#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gst/gst.h>
#include <glib.h>

#include "cmd_helper.h"
#include "filter.h"
#include "vgst_lib.h"
#include "video_cfg.h"
#include "vgst_utils.h"   // dla init_struct_params, bus_callback, vgst_* API

// ---- Dane / struktury jak u Ciebie ----
struct maincontroller mc;
struct filter_tbl ft;
extern vgst_application app;
#define MAX_MODES 3

unsigned int *g_flags;
vgst_enc_params         enc_param;
vgst_sdx_filter_params  filter_param;
vgst_ip_params          input_param;
vgst_op_params          output_param;
vgst_cmn_params         cmn_param;

static struct option opts[] = {
    { "drm-module",        required_argument, NULL, 'd' },
    { "help",              no_argument,       NULL, 'h' },
    { "partial-reconfig",  no_argument,       NULL, 'p' },
    { "resolution",        required_argument, NULL, 'r' },
    { NULL, 0, NULL, 0 }
};

static GMainLoop *g_loop = NULL;

static void sigint_handler(int s) {
    (void)s;
    if (g_loop) g_main_loop_quit(g_loop);
}

int main(int argc, char **argv)
{
    // --- GStreamer + logi + sygnały ---
    gst_init(&argc, &argv);
    setenv("GST_DEBUG", "3", 0);
    setenv("GST_DEBUG_NO_COLOR", "1", 0);

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    struct sigaction sa = {0};
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    printf("CLI start\n");

    // --- Konfiguracja wyjścia / parse args ---
    int ret, i, c;
    int best_mode = 1;
    const int width[MAX_MODES]  = {3840, 1920, 1280};
    const int height[MAX_MODES] = {2160, 1080, 720};

    struct vlib_config_data cfg;
    struct vlib_config      config;   // do przekazania vsrc/mode/type

    memset(&cfg, 0, sizeof(cfg));
    memset(&config, 0, sizeof(config));

    cfg.width_out = width[0];
    cfg.height_out = height[0];
    g_flags = &(cfg.flags);
    cfg.flags |= VLIB_CFG_FLAG_FILE_ENABLE; /* Enable file source support */

    while ((c = getopt_long(argc, argv, "d:hpr:", opts, NULL)) != -1) {
        switch (c) {
        case 'd':
            sscanf(optarg, "%u", &cfg.display_id);
            break;
        case 'h':
            printf("Usage: %s [options]\n", argv[0]);
            printf("-d, --drm-module name   DRM module index (0/1)\n");
            printf("-p, --partial-reconfig  Enable PR\n");
            printf("-r, --resolution WxH    3840x2160 | 1920x1080 | 1280x720\n");
            return 0;
        case 'p':
            cfg.flags |= VLIB_CFG_FLAG_PR_ENABLE;
            break;
        case 'r':
            ret = sscanf(optarg, "%ux%u", &cfg.width_out, &cfg.height_out);
            if (ret != 2) {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
                return 1;
            }
            best_mode = 0;
            break;
        default:
            fprintf(stderr, "Invalid option -%c\n", c);
            return 1;
        }
    }

    // --- Dobór trybu KMS ---
    for (i = 0; i < MAX_MODES; i++) {
        if (best_mode) {
            size_t vr;
            ret = vlib_drm_try_mode(cfg.display_id, width[i], height[i], &vr);
            if (ret == VLIB_SUCCESS) {
                cfg.width_out = width[i];
                cfg.height_out = height[i];
                cfg.fps.numerator = vr;
                cfg.fps.denominator = 1;
                break;
            }
        } else {
            if (cfg.width_out == width[i] && cfg.height_out == height[i])
                break;
        }
    }
    if (i == MAX_MODES) {
        fprintf(stderr, "Only supported: 720p, 1080p, 2160p\n");
        return 1;
    }

    // --- Enumeracja źródeł / media graph ---
    ret = vgst_video_src_init(&cfg);
    if (ret) {
        fprintf(stderr, "ERROR: vgst_video_src_init failed: %s\n", vlib_errstr);
        return ret;
    }

    // Input = Output jeśli brak
    if (!cfg.width_in) {
        cfg.width_in  = cfg.width_out;
        cfg.height_in = cfg.height_out;
    }

    // --- Inic biblioteki / fill domyślne ---
    filter_init(&ft); // rejestracja typów filtrów (woła gst_init wewnątrz, nie szkodzi)

    vgst_init_base(&cfg, &enc_param, &input_param, &output_param, &cmn_param, &filter_param, &ft);
    init_struct_params(&enc_param, &input_param, &output_param, &cmn_param, &filter_param);

    // Bezpieczne doprecyzowanie wartości na start (zostajemy przy 1 strumieniu)
    cmn_param.num_src       = 1;
    input_param.filter_type = VCU;  // unikamy SDX bez nazwy filtra
    // jeśli wiesz jaki format – ustaw przyjaźniejszy string:
    input_param.format_str  = "NV12";        // albo "YUY2" gdy wejście YUYV

    // --- wybór źródła jak w GUI (preferuj TPG) ---
    config.type = 0;
    config.mode = 0;
    for (config.vsrc = 0; config.vsrc < vlib_video_src_cnt_get(); config.vsrc++) {
        const struct vlib_vdev *vsrc = vlib_video_src_get(config.vsrc);
        if (vlib_video_src_get_class(vsrc) == VLIB_VCLASS_TPG) break;
    }
    if (config.vsrc == vlib_video_src_cnt_get()) {
        config.vsrc = 0; // fallback: pierwsze dostępne
    }

    printf("video sources found: %zu, using index %u\n",
           vlib_video_src_cnt_get(), config.vsrc);

    // --- init controllera i start wybranego źródła ---
    cmd_init(&mc, config, &ft);
    setVideo(&mc, 1);   // NIE na sztywno „1” – używamy wykrytego

    // --- Ustaw PLAYING + bus watch (jeśli biblioteka sama nie robi) ---
    for (unsigned s = 0; s < cmn_param.num_src; ++s) {
        if (app.playback[s].pipeline) {
            gst_element_set_state(app.playback[s].pipeline, GST_STATE_PLAYING);

            GstBus *bus = gst_element_get_bus(app.playback[s].pipeline);
            if (bus) {
                gst_bus_add_watch(bus, (GstBusFunc)bus_callback, &app.playback[s]);
                gst_object_unref(bus);
            }
        }
    }

    // --- Pętla GLib (zastępuje event loop Qt) ---
    g_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(g_loop);

    // --- Sprzątanie po Ctrl+C / EOS ---
    for (unsigned s = 0; s < cmn_param.num_src; ++s) {
        if (app.playback[s].pipeline) {
            gst_element_set_state(app.playback[s].pipeline, GST_STATE_NULL);
            gst_object_unref(app.playback[s].pipeline);
            app.playback[s].pipeline = NULL;
        }
    }
    g_main_loop_unref(g_loop);
    g_loop = NULL;

    return 0;
}
