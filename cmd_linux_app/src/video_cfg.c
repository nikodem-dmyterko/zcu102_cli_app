#include <stdio.h>
#include <string.h>

#include "video_cfg.h"
#include <filter.h>
#include <vgst_sdxfilter2d.h>
#include <video.h>

static vgst_enc_params enc_param;
static vgst_sdx_filter_params filter_param;
static vgst_ip_params input_param;
static vgst_op_params output_param;
static vgst_cmn_params cmn_param;
static struct filter_tbl ft;
static struct vlib_config_data vlib_cfg;

void video_cfg_init(void) {
    memset(&vlib_cfg, 0, sizeof(vlib_cfg));

    init_struct_params(&enc_param, &input_param, &output_param, &cmn_param,
                       &filter_param);

    filter_init(&ft);
    filter_param.filter_name = SDX_FILTER2D_PLUGIN;
    filter_param.filter_mode = GST_FILTER_MODE_HW;

    cmn_param.num_src = 1;
    cmn_param.sink_type = DISPLAY;
    cmn_param.driver_type = DP;

    vgst_init();
    vlib_video_src_init(&vlib_cfg);
}

void video_cfg_list_sources(void) {
    size_t count = vlib_video_src_cnt_get();
    printf("source count: %zu\n", count);
    for (size_t i = 0; i < count; ++i) {
        const char *name = vgst_get_srctype(i);
        if (name) {
            printf("%s\n", name);
        }
    }
}

void video_cfg_list_sinks(void) {
    unsigned int ids[] = {DP, HDMI_Tx};
    size_t cnt = 0;
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i) {
        const char *n = vgst_get_sinkname(ids[i]);
        if (n) {
            printf("%s\n", n);
            cnt++;
        }
    }
    printf("sink count: %zu\n", cnt);
}

int video_cfg_set_source(const char *name) {
    size_t count = vlib_video_src_cnt_get();
    for (size_t i = 0; i < count; ++i) {
        const char *n = vgst_get_srctype(i);
        if (n && strcmp(n, name) == 0) {
            input_param.device_type = i;
            return 0;
        }
    }
    return -1;
}

int video_cfg_set_sink(const char *name) {
    unsigned int ids[] = {DP, HDMI_Tx};
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i) {
        const char *n = vgst_get_sinkname(ids[i]);
        if (n && strcmp(n, name) == 0) {
            cmn_param.driver_type = ids[i];
            return 0;
        }
    }
    return -1;
}

int video_cfg_set_filter(const char *name, const short coeff[3][3]) {
    filter_param.filter_name = (char *)name;
    if (is_sdx_plugin_present(&ft, SDX_FILTER2D_PLUGIN)) {
        filter2d_set_coeff(filter_type_get_obj(&ft, 0), coeff);
    }
    return 0;
}

void video_cfg_set_accel(int hw) {
    filter_param.filter_mode = hw ? GST_FILTER_MODE_HW : GST_FILTER_MODE_SW;
}

int video_cfg_create_pipeline(const char *mode) {
    vlib_cfg.display_id = cmn_param.driver_type;
    vlib_init_gst(&vlib_cfg);

    if (mode && strcmp(mode, "passthrough") == 0) {
        input_param.filter_type = VCU;
    } else {
        input_param.filter_type = SDX_FILTER;
    }
    int ret = vgst_config_options(&enc_param, &input_param, &output_param,
                                  &cmn_param, &filter_param);
    if (ret != VGST_SUCCESS) {
        return ret;
    }
    ret = vgst_create_pipeline();
    if (ret != VGST_SUCCESS) {
        return ret;
    }
    return vgst_run_pipeline();
}

void video_cfg_cleanup(void) {
    vlib_uninit_gst();
    vgst_uninit();
}