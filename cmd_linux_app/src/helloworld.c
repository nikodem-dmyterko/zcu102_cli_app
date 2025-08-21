#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <gst/gst.h>
#include <glib.h>

#include "cmd_helper.h"
#include "filter.h"
#include "vgst_lib.h"
#include "video_cfg.h"
#include "vgst_sdxfilter2d.h"
#include "vgst_utils.h"   // init_struct_params, vgst_* API

struct maincontroller mc;
struct filter_tbl ft;

unsigned int *g_flags;
vgst_enc_params         enc_param;
vgst_sdx_filter_params  filter_param;
vgst_ip_params          input_param;
vgst_op_params          output_param;
vgst_cmn_params         cmn_param;

static GMainLoop *g_loop = NULL;

static void sigint_handler(int s) {
    (void)s;
    if (g_loop) g_main_loop_quit(g_loop);
}

struct sink_map {
    const char *name;
    VGST_SINK_TYPE type;
};

static const struct sink_map sink_tbl[] = {
    {"stream",  STREAM},
    {"record",  RECORD},
    {"display", DISPLAY},
    {"split",   SPLIT_SCREEN},
};

static void print_usage(const char *prog)
{
    printf("Usage: %s [--source h|NAME] [--sink h|NAME]\n"
           "           [--filter2d NAME|COEFF] [--accel sw|hw]\n"
           "           [--pipeline SRC SINK passthrough|processing]\n"
           "           [--help]\n\n"
           "Example: %s --pipeline tpg display processing --filter2d blur --accel hw\n",
           prog, prog);
}

static void list_sources(void)
{
    size_t cnt = vlib_video_src_cnt_get();
    printf("Available sources (%zu):\n", cnt);
    for (size_t i = 0; i < cnt; ++i) {
        const struct vlib_vdev *v = vlib_video_src_get(i);
        enum vlib_vsrc_class cls = vlib_video_src_get_class(v);
        const char *name = vsrc_get_short_name(cls);
        if (!name)
            name = vlib_video_src_get_display_text(v);
        printf("  %s\n", name);
    }
}

static void list_sinks(void)
{
    printf("Available sinks (%zu):\n", sizeof(sink_tbl)/sizeof(sink_tbl[0]));
    for (size_t i = 0; i < sizeof(sink_tbl)/sizeof(sink_tbl[0]); ++i)
        printf("  %s\n", sink_tbl[i].name);
}

static void list_filter2d(void)
{
    printf("Available 2D filter presets (%d):\n", FILTER2D_PRESET_CNT);
    for (int i = 0; i < FILTER2D_PRESET_CNT; ++i)
        printf("  %s\n", filter2d_get_preset_name(i));
}

static int source_from_name(const char *name)
{
    size_t cnt = vlib_video_src_cnt_get();
    for (size_t i = 0; i < cnt; ++i) {
        const struct vlib_vdev *v = vlib_video_src_get(i);
        enum vlib_vsrc_class cls = vlib_video_src_get_class(v);
        const char *sn = vsrc_get_short_name(cls);
        if (sn && !strcasecmp(name, sn))
            return i;
        const char *dn = vlib_video_src_get_display_text(v);
        if (dn && !strcasecmp(name, dn))
            return i;
    }
    return -1;
}

static int sink_from_name(const char *name, VGST_SINK_TYPE *out)
{
    for (size_t i = 0; i < sizeof(sink_tbl)/sizeof(sink_tbl[0]); ++i)
        if (!strcasecmp(name, sink_tbl[i].name)) {
            *out = sink_tbl[i].type;
            return 0;
        }
    return -1;
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

    vgst_init();

    struct vlib_config_data cfg = {0};
    struct vlib_config      config = {0};

    cfg.width_out = 1280;
    cfg.height_out = 720;
    g_flags = &cfg.flags;
    cfg.flags |= VLIB_CFG_FLAG_FILE_ENABLE;

    if (vgst_video_src_init(&cfg)) {
        fprintf(stderr, "ERROR: vgst_video_src_init failed: %s\n", vlib_errstr);
        return 1;
    }

    if (!cfg.width_in) {
        cfg.width_in  = cfg.width_out;
        cfg.height_in = cfg.height_out;
    }

    filter_init(&ft);
    vgst_init_base(&cfg, &enc_param, &input_param, &output_param,
                   &cmn_param, &filter_param, &ft);
    init_struct_params(&enc_param, &input_param, &output_param,
                      &cmn_param, &filter_param);

    cmn_param.num_src       = 1;
    input_param.filter_type = VCU;
    input_param.format_str  = "NV12";

    config.type = 0;
    config.mode = 0;

    bool list_src = false, list_snk = false, list_f2d = false;
    const char *src_name = NULL, *sink_name = NULL;
    const char *filter_arg = NULL, *accel_arg = NULL;
    const char *pipe_src = NULL, *pipe_sink = NULL, *pipe_mode = NULL;
    bool run_pipeline = false;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--source")) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            if (argv[i + 1][0] == 'h') { list_src = true; i++; }
            else src_name = argv[++i];
        } else if (!strcmp(argv[i], "--sink")) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            if (argv[i + 1][0] == 'h') { list_snk = true; i++; }
            else sink_name = argv[++i];
        } else if (!strcmp(argv[i], "--filter2d")) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            if (argv[i + 1][0] == 'h') { list_f2d = true; i++; }
            else filter_arg = argv[++i];
        } else if (!strcmp(argv[i], "--accel")) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            accel_arg = argv[++i];
        } else if (!strcmp(argv[i], "--pipeline")) {
            run_pipeline = true;
            int rem = argc - (i + 1);
            if (rem >= 3 && argv[i+1][0] != '-' && argv[i+2][0] != '-' && argv[i+3][0] != '-') {
                pipe_src  = argv[++i];
                pipe_sink = argv[++i];
                pipe_mode = argv[++i];
            } else if (rem >= 1) {
                pipe_mode = argv[++i];
            } else {
                print_usage(argv[0]);
                closeall();
                return 1;
            }
        } else if (!strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            closeall();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            closeall();
            return 1;
        }
    }

    if (list_src) { list_sources(); closeall(); return 0; }
    if (list_snk) { list_sinks(); closeall(); return 0; }
    if (list_f2d) { list_filter2d(); closeall(); return 0; }
    if (!run_pipeline) {
        print_usage(argv[0]);
        closeall();
        return 0;
    }

    if (!pipe_src) pipe_src = src_name;
    if (!pipe_sink) pipe_sink = sink_name;
    if (!pipe_src || !pipe_sink || !pipe_mode) {
        fprintf(stderr, "source/sink/mode not specified\n");
        print_usage(argv[0]);
        closeall();
        return 1;
    }

    int src_idx = source_from_name(pipe_src);
    if (src_idx < 0) {
        fprintf(stderr, "Unknown source '%s'\n", pipe_src);
        closeall();
        return 1;
    }
    config.vsrc = (size_t)src_idx;
    VGST_SINK_TYPE st;
    if (sink_from_name(pipe_sink, &st) < 0) {
        fprintf(stderr, "Unknown sink '%s'\n", pipe_sink);
        closeall();
        return 1;
    }
    cmn_param.sink_type = st;
    if (!strcasecmp(pipe_mode, "processing"))
        config.mode = 1;
    else
        config.mode = 0;

    if (accel_arg) {
        if (!strcasecmp(accel_arg, "sw"))
            input_param.filter_type = SDX_FILTER;
        else
            input_param.filter_type = VCU;
    }

    enum { F_NONE, F_PRESET, F_COEFF } fkind = F_NONE;
    int preset_idx = 0;
    short coeffs[9];
    if (filter_arg) {
        if (strlen(filter_arg) == 9 && strspn(filter_arg, "0123456789") == 9) {
            for (int i = 0; i < 9; ++i)
                coeffs[i] = filter_arg[i] - '0';
            fkind = F_COEFF;
        } else {
            for (int i = 0; i < FILTER2D_PRESET_CNT; ++i) {
                const char *nm = filter2d_get_preset_name(i);
                if (!strcasecmp(filter_arg, nm)) { preset_idx = i; fkind = F_PRESET; break; }
            }
            if (fkind == F_NONE) {
                fprintf(stderr, "Unknown filter preset '%s'\n", filter_arg);
                closeall();
                return 1;
            }
        }
    }

    cmd_init(&mc, config, &ft);
    if (fkind == F_PRESET)
        setPreset(&mc, preset_idx);
    else if (fkind == F_COEFF)
        filterCoeff(&mc,
            coeffs[0], coeffs[1], coeffs[2],
            coeffs[3], coeffs[4], coeffs[5],
            coeffs[6], coeffs[7], coeffs[8]);
    setVideo(&mc, 1);

    g_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(g_loop);
    g_main_loop_unref(g_loop);
    g_loop = NULL;

    closeall();

    return 0;
}
