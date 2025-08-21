

/* vlib/vgst includes */
#include <vcap_csi.h>
#include <vcap_tpg.h>
#include <vgst_lib.h>

#include "cmd_helper.h"
#include "video_cfg.h"
#include "perfapm-client.h"

extern unsigned int *g_flags;
extern vgst_enc_params		enc_param;
extern vgst_sdx_filter_params	filter_param;
extern vgst_ip_params		input_param;
extern vgst_op_params		output_param;
extern vgst_cmn_params		cmn_param;

static bool demo_src = true;
static bool self_initialized = false;

/**
 * @brief Initializes the maincontroller structure.
 *
 * Sets the video configuration, filter table pointer, video control state, and flags.
 * Also initializes performance measurement.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param cfg Video configuration.
 * @param ft Pointer to the filter table.
 */
void cmd_init(struct maincontroller *mc, struct vlib_config cfg, struct filter_tbl *ft) {
    mc->config = cfg;
    mc->ft = ft;
    mc->videoCtrl = VIDEO_CTRL_OFF;
    mc->demo_src = true;
    mc->self_initialized = true;
    int perfInitStatus = EXIT_FAILURE;
    perfInitStatus = perfoacl_init_all(PERF_LINUX_OS, PERF_SAMPLE_INTERVAL_COUNTER);
    if(perfInitStatus){
        return;
    }
    else{
        printf("Perf Init Status: %d", perfInitStatus);
    }
    mc->showMemoryThroughput = 0;
}

/**
 * @brief Turns video playback on or off.
 *
 * If play == 0, stops the video pipeline.
 * If play != 0, applies the current video configuration.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param play Video playback flag (0 = OFF, !=0 = ON).
 */
void setVideo(struct maincontroller *mc, int play){
    if(play == 0){
        mc->videoCtrl = VIDEO_CTRL_OFF;
        int err = vgst_stop_pipeline();
        if (err != VLIB_SUCCESS){
            printf("ERROR play == 0: %d", err);
        }
    }
    else{
        mc->videoCtrl = VIDEO_CTRL_ON;
        int err = vgst_change_mode(&mc->config, *g_flags, &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
        if (err != VLIB_SUCCESS){
            printf("ERROR play != 0: %d", err);
        }
    }
}

/**
 * @brief Sets the video filter mode.
 *
 * Disables video, sets the filter mode, and re-enables video.
 * If playback is active, updates the video pipeline mode.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param filter New filter mode.
 * @param isPlaying Flag indicating whether playback is active (0 = OFF, !=0 = ON).
 */
void setFilterMode(struct maincontroller *mc, int filter, int isPlaying){
    mc->videoCtrl = VIDEO_CTRL_OFF;
    mc->config.mode = filter;
    mc->videoCtrl = VIDEO_CTRL_ON;

    if (isPlaying){
        int err = vgst_change_mode(&mc->config, *g_flags, &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
    }
}

/**
 * @brief Sets the video filter type.
 *
 * Disables video, sets the filter type, and re-enables video.
 * If playback is active, updates the video pipeline mode.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param filter New filter type.
 * @param isPlaying Flag indicating whether playback is active (0 = OFF, !=0 = ON).
 */
void setFilterType(struct maincontroller *mc, int filter, int isPlaying){
    mc->videoCtrl = VIDEO_CTRL_OFF;
    mc->config.type = filter;
    mc->videoCtrl = VIDEO_CTRL_ON;
    if(isPlaying){
        int err = vgst_change_mode(&mc->config, *g_flags, &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
    }
}

/**
 * @brief Sets the video input source.
 *
 * Disables video, updates the input source, and re-enables video.
 * If playback is active, applies the new configuration.
 * Restores previous input if change fails.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param input Input source ID.
 * @param isPlaying Flag indicating whether playback is active.
 * @param defaultfilename Default file name (unused in this function).
 * @param hasFilePath Flag indicating whether a file path exists.
 */
void setInput(struct maincontroller *mc, int input, int isPlaying, const char *defaultfilename, bool hasFilePath){
    mc->videoCtrl = VIDEO_CTRL_OFF;
    size_t prevVsrc = mc->config.vsrc;
    mc->config.vsrc = vlib_video_src_get_index(vsrc_get_vd((video_src)input));
    mc->videoCtrl = VIDEO_CTRL_ON;
    int ret = 0;
    if(isPlaying){
        ret = vgst_change_mode(&mc->config, *g_flags, &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
    }
    if(input == VLIB_VCLASS_FILE && !hasFilePath){
        // Do nothing if file path is missing
    }
    else if (ret != VLIB_SUCCESS) {
        mc->config.vsrc = prevVsrc;
        vgst_change_mode(&mc->config, *g_flags, &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
    }
}

/**
 * @brief Sets the Test Pattern Generator (TPG) background pattern.
 *
 * @param input Pattern index (0 is ignored).
 */
void setTPGPattern(int input){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_bg_pattern(vd, input+1);
}

/**
 * @brief Sets the filter preset coefficients.
 *
 * Updates the 2D filter coefficients based on a preset index.
 * If input equals the max preset count, it applies current coefficients.
 *
 * @param mc Pointer to the maincontroller structure.
 * @param input Preset index.
 */
void setPreset(struct maincontroller *mc, int input){
    UNUSED(input);
    if (is_sdx_plugin_present(mc->ft, SDX_FILTER2D_PLUGIN)) {
        if(input < FILTER2D_PRESET_CNT){
            filter2d_set_preset_coeff(filter_type_get_obj(mc->ft, mc->config.type), (filter2d_preset) input);
        }
        coeff_t *coeff1 = filter2d_get_coeff((filter_type_get_obj(mc->ft, mc->config.type)));
        if(input == FILTER2D_PRESET_CNT){
            filter2d_set_coeff(filter_type_get_obj(mc->ft, mc->config.type), *coeff1);
        }
    }
}



void setBoxSize(int size){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_box_size(vd, size);
}

void setBoxColor(int color){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_box_color(vd, color);
}

void setBoxSpeed(int speed){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_box_speed(vd, speed);
}

void setCrossRows(int rows){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_cross_hair_num_rows(vd, rows);
}

void setCrossColumns(int columns){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_cross_hair_num_columns(vd, columns);
}

void setZoneH(int h){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_zplate_hor_cntl_start(vd, h);
}

void setZoneHDelta(int h){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_zplate_hor_cntl_delta(vd, h);
}

void setZoneV(int v){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_zplate_ver_cntl_start(vd, v);
}

void setZoneVDelta(int v){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_TPG);
    tpg_set_zplate_hor_cntl_delta(vd, v);
}

// CSI Functions
void csiredgamma(int redg){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    gamma_set_red_correction(vd, redg);
}

void csigreengamma(int greeng){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    gamma_set_green_correction(vd, greeng);
}

void csibluegamma(int blueg){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    gamma_set_blue_correction(vd, blueg);
}

void csicontrast(int contrast){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    csc_set_contrast(vd, contrast);
}

void csibrightness(int brightness){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    csc_set_brightness(vd, brightness);
}

void csiredgain(int redgain){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    csc_set_red_gain(vd, redgain);
}

void csigreengain(int greengain){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    csc_set_green_gain(vd, greengain);
}

void csibluegain(int bluegain){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    csc_set_blue_gain(vd, bluegain);
}

void csiexposure(int exposure){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    imx274_set_exposure(vd, exposure);
}

void csiimxgain(int imxgain){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    imx274_set_gain(vd, imxgain);
}

void setTestPattern(int testpattern){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    imx274_set_test_pattern(vd, testpattern);
}

void setVerticalFlip(int flip){
    const struct vlib_vdev *vd = vsrc_get_vd(VLIB_VCLASS_CSI);
    imx274_set_vertical_flip(vd, flip);
}





void filterCoeff(struct maincontroller *mc, int c00,int c01,int c02,int c10,int c11,int c12,int c20,int c21,int c22){
	short coeff[3][3];
	coeff[0][0] = (short int) c00;
	coeff[0][1] = (short int) c01;
	coeff[0][2] = (short int) c02;
	coeff[1][0] = (short int) c10;
	coeff[1][1] = (short int) c11;
	coeff[1][2] = (short int) c12;
	coeff[2][0] = (short int) c20;
	coeff[2][1] = (short int) c21;
	coeff[2][2] = (short int) c22;
	UNUSED(coeff);
	if (is_sdx_plugin_present (mc->ft, SDX_FILTER2D_PLUGIN)) {
		filter2d_set_coeff(filter_type_get_obj(mc->ft, mc->config.type), coeff);
	}
}


void closeall(){
	perfoacl_deinit_all(PERF_LINUX_OS, PERF_SAMPLE_INTERVAL_COUNTER);
//	demo_timer->stop();
//	fps_timer->stop();
	vgst_stop_pipeline_base();
	vgst_uninit();
}


void videoSrcLoop(struct maincontroller *mc){
    int count = 0;
    int arySize = mc->demoSequenceLength;
    if (arySize <= 0) return;

    while (1) {
        if (mc->demo_src_loop_count >= arySize) {
            mc->demo_src_loop_count = 0;
        }

        int src_idx = mc->demo_sequence[mc->demo_src_loop_count][SOURCE_TYPE];
        const struct vlib_vdev *vsrc = vsrc_get_vd((video_src)src_idx);

        if (vsrc) {
            demo_src = true;

            struct vlib_config d_config = {0};   // <- zero-init!
            d_config.vsrc = vlib_video_src_get_index(vsrc);

            if (mc->demo_sequence[mc->demo_src_loop_count][FILTER_TYPE] != -1) {
                d_config.type = mc->demo_sequence[mc->demo_src_loop_count][FILTER_TYPE];
                d_config.mode = mc->demo_sequence[mc->demo_src_loop_count][FILTER_MODE];
            }

            int ret = vgst_change_mode(&d_config, *g_flags,
                                       &enc_param, &input_param, &output_param, &cmn_param, &filter_param);
            if (ret != VLIB_SUCCESS) {
                mc->demo_src_loop_count++;
                count++;
                if (count >= arySize) {
                    demo_src = false;
                    break;
                }
                continue;
            }

            count = 0;
            mc->config.vsrc = d_config.vsrc;

            if (mc->demo_sequence[mc->demo_src_loop_count][FILTER_TYPE] != -1) {
                mc->config.type = d_config.type;
                mc->config.mode = d_config.mode;
            }
        } else {
            mc->demo_src_loop_count++;
            count++;
            if (count >= arySize) {
                mc->demo_src_loop_count = 0;
                if (!demo_src) {
                    count = 0;
                }
                break;
            } else {
                continue;
            }
        }

        mc->demo_src_loop_count++;
        break;   // jeden „krok”; resztę zrobi timer
    }
}



//void demoSequence(QVariantList seqList){
//	if (mc->self_initialized && mc->demo_timer->isActive()){
//		demo_src = false;
//	}
//	int i = 0;
//	for(; i < seqList.count(); i++){
//		QVariantMap row = seqList.at(i).toMap();
//		demo_sequence[i][SOURCE_TYPE] = row["source"].toInt();
//		demo_sequence[i][FILTER_TYPE] = row["filterType"].toInt();
//		demo_sequence[i][FILTER_MODE] = row["filterMode"].toInt();
//	}
//	demoSequenceLength = i;
//}
//void updateDemoTimer(int timerval){
//	demoTimerInterval = timerval * SECONDS_TO_MILLISEC;
//}


//void updateThroughput(){
//	double data[NMemData];
//
//	Perf_OA_Payload *openamp_payload = NULL;
//	openamp_payload = perfoacl_get_rd_wr_cnt(MAX_PERF_SLOTS, PERF_SAMPLE_INTERVAL_COUNTER);
//	data[videoSrc] = (float) ((openamp_payload->readcnt[6] + openamp_payload->readcnt[7]) * 8 / 1000000000.0);
//	data[filter] = (float) ((openamp_payload->readcnt[8] + openamp_payload->readcnt[9]) * 8 / 1000000000.0);
//	data[videoPort] = (float) ((openamp_payload->readcnt[4] + openamp_payload->readcnt[5]) * 8 / 1000000000.0);
//
//}

