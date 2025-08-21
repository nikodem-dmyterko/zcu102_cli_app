/******************************************************************************
 * (c) Copyright 2017-2018 Xilinx, Inc. All rights reserved.
 *
 * This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and international copyright and other
 * intellectual property laws.
 *
 * DISCLAIMER
 * This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as otherwise provided in a valid
 * license issued to you by Xilinx, and to the maximum extent permitted by
 * applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
 * FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
 * MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
 * and (2) Xilinx shall not be liable (whether in contract or tort, including
 * negligence, or under any other theory of liability) for any loss or damage
 * of any kind or nature related to, arising under or in connection with these
 * materials, including for any direct, or any indirect, special, incidental,
 * or consequential loss or damage (including loss of data, profits, goodwill,
 * or any type of loss or damage suffered as a result of any action brought by
 * a third party) even if such damage or loss was reasonably foreseeable or
 * Xilinx had been advised of the possibility of the same.
 *
 * CRITICAL APPLICATIONS
 * Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe performance, such as life-support or
 * safety devices or systems, Class III medical devices, nuclear facilities,
 * applications related to the deployment of airbags, or any other applications
 * that could lead to death, personal injury, or severe property or
 * environmental damage (individually and collectively, "Critical
 * Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical Applications, subject only to applicable laws
 * and regulations governing limitations on product liability.
 *
 * THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.
 *******************************************************************************/

#include "vgst_lib.h"
#include "vgst_utils.h"

GST_DEBUG_CATEGORY (vgst_lib);
#define GST_CAT_DEFAULT vgst_lib

static struct filter_tbl *sdx_ft;  /* filter table */

const gchar *
vgst_error_to_string (VGST_ERROR_LOG error_code, gint index) {
    return error_to_string (error_code, index);
}

gint
check_limitation (vgst_ip_params *ip_param, vgst_enc_params *enc_param, guint num_src, guint display_rate) {
    if (ip_param->width == MAX_WIDTH && ip_param->height == MAX_HEIGHT && num_src == 1 && display_rate == MAX_SUPPORTED_FRAME_RATE) {
      /* 1 4kp60 pipeline limitations */
      if ((enc_param->b_frame > 0) || !enc_param->enable_l2Cache || (enc_param->gop_mode == LOW_DELAY_B)) {
        GST_ERROR ("1-4kp60 Pipeline limitations");
        return VGST_ERROR_4KP60_PARAM_NOT_SUPPORTED;
      }
    }
    if ((ip_param->width == MAX_WIDTH) && (ip_param->height == MAX_HEIGHT) && (num_src == 2) && (display_rate == MAX_SUPPORTED_FRAME_RATE/2)) {
      /* 2 4kp30 pipeline limitations */
      if ((enc_param->b_frame > 0) || !enc_param->enable_l2Cache || (enc_param->gop_mode == LOW_DELAY_B) || (enc_param->latency_mode == SUB_FRAME_LATENCY) ||\
          (enc_param->enc_type == AVC && enc_param->bitrate > MAX_H264_BITRATE/2) || \
          (enc_param->enc_type == HEVC && enc_param->bitrate > MAX_H265_BITRATE/2)) {
        GST_ERROR ("2-4kp30 Pipeline limitations");
        return VGST_ERROR_2_4KP30_PARAM_NOT_SUPPORTED;
      }
    }
    if ((ip_param->width == MAX_WIDTH/2) && (ip_param->height == MAX_HEIGHT/2) && (num_src == 4) && (display_rate == MAX_SUPPORTED_FRAME_RATE)) {
      /* 4 1080p60 pipeline limitations */
      if ((enc_param->b_frame > 0) || !enc_param->enable_l2Cache || (enc_param->gop_mode == LOW_DELAY_B) || (enc_param->latency_mode == SUB_FRAME_LATENCY) ||\
          (enc_param->enc_type == AVC && enc_param->bitrate > MAX_H264_BITRATE/4) || \
          (enc_param->enc_type == HEVC && enc_param->bitrate > MAX_H265_BITRATE/4)) {
        GST_ERROR ("4-1080p60 Pipeline limitations");
        return VGST_ERROR_4_1080P60_PARAM_NOT_SUPPORTED;
      }
    }
    return VGST_SUCCESS;
}

gint
vgst_config_options (vgst_enc_params *enc_param, vgst_ip_params *ip_param, vgst_op_params *op_param,
                     vgst_cmn_params *cmn_param, vgst_sdx_filter_params *filter_param) {
    struct stat file_stat;
    guint i,num_src = cmn_param->num_src;
    gint ret;
    struct vlib_config_data config;

    /* initialize GStreamer */
    gst_init (NULL, NULL);

    GST_DEBUG_CATEGORY_INIT (vgst_lib, "vgst-lib", 0, "vgst lib");
    if (num_src > MAX_SRC_NUM) {
      GST_ERROR ("Source count is invalid");
      return VGST_ERROR_SRC_COUNT_INVALID;
    }
    if ((DP == cmn_param->driver_type || SDI_Tx == cmn_param->driver_type) && num_src > 1) {
      GST_ERROR ("Multi stream is not supported on DP or SDI");
      return VGST_ERROR_MULTI_STREAM_FAIL;
    }
    if ((DP == cmn_param->driver_type || SDI_Tx == cmn_param->driver_type) && cmn_param->sink_type == SPLIT_SCREEN) {
      GST_ERROR ("Split screen is not supported on DP or SDI");
      return VGST_ERROR_SPLIT_SCREEN_FAIL;
    }
    if (DP != cmn_param->driver_type && HDMI_Tx != cmn_param->driver_type && SDI_Tx != cmn_param->driver_type) {
      GST_ERROR ("Driver type is invalid");
      return VGST_ERROR_DRIVER_TYPE_MISMATCH;
    }

    for (i =0; i< num_src; i++) {
      if (LIVE_SRC != ip_param[i].src_type && FILE_SRC != ip_param[i].src_type && STREAMING_SRC != ip_param[i].src_type) {
        GST_ERROR ("Source type is not supported");
        return VGST_ERROR_SRC_TYPE_NOT_SUPPORTED;
      }
      if ((ip_param[i].filter_type != SDX_FILTER) && ip_param[i].format != NV12) {
        GST_ERROR ("Input format is not supported");
        return VGST_ERROR_FORMAT_NOT_SUPPORTED;
      }
      if ((ip_param[i].filter_type != SDX_FILTER) && FILE_SRC == ip_param[i].src_type && (!ip_param[i].uri || (stat(ip_param[i].uri +strlen("file:"), &file_stat) < 0))) {
        GST_ERROR ("Input file does not exist");
        return VGST_ERROR_FILE_IO;
      }
      if ((ip_param[i].filter_type == SDX_FILTER) && FILE_SRC == ip_param[i].src_type && (stat(ip_param[i].uri, &file_stat) < 0)) {
        GST_ERROR ("Input file does not exist");
        return VGST_ERROR_FILE_IO;
      }
      if (FILE_SRC == ip_param[i].src_type && num_src > 1) {
        GST_ERROR ("File playback in multi stream not supported");
        return VGST_ERROR_FILE_IN_MULTISTREAM_NOT_SUPPORTED;
      }
      if (ip_param[i].width > MAX_WIDTH || ip_param[i].height > MAX_HEIGHT) {
        GST_ERROR ("Resolution WxH not supported");
        return VGST_ERROR_RESOLUTION_NOT_SUPPORTED;
      }
      if ((ip_param[i].filter_type != SDX_FILTER) && LIVE_SRC == ip_param[i].src_type && ip_param[i].device_type == TPG_1 && (ip_param[i].width != MAX_WIDTH || ip_param[i].height != (guint)MAX_HEIGHT)) {
        GST_ERROR ("TPG other than 4k resolution not supported");
        return VGST_ERROR_TPG_IN_1080P_NOT_SUPPORTED;
      }
      if ((ip_param[i].filter_type != SDX_FILTER) && LIVE_SRC == ip_param[i].src_type && (ip_param[i].device_type != TPG_1 && ip_param[i].device_type != HDMI_1 \
          && ip_param[i].device_type != CSI && ip_param[i].device_type != SDI && ip_param[i].device_type != HDMI_2 \
          && ip_param[i].device_type != HDMI_3)) {
        GST_ERROR ("Device type is invalid");
        return VGST_ERROR_DEVICE_TYPE_INVALID;
      }
      if ((cmn_param->sink_type  == STREAM || cmn_param->sink_type  == RECORD) && ip_param[i].raw == TRUE) {
        GST_WARNING ("Oops!! raw flag set wrong");
        ip_param[i].raw = FALSE;
      }
      if ((FILE_SRC == ip_param[i].src_type || STREAMING_SRC == ip_param[i].src_type) && (cmn_param->sink_type  == RECORD || cmn_param->sink_type  == STREAM)) {
        GST_ERROR ("For file source, sink type should be only display");
        return VGST_ERROR_INPUT_OPTIONS_INVALID;
      }
      if ((cmn_param->sink_type  == RECORD) && (enc_param[i].latency_mode == SUB_FRAME_LATENCY)) {
        GST_ERROR ("Sub frame latency is not supported in record option");
        return VGST_ERROR_SUB_FRAME_ON_RECORD_NOT_SUPPORTED;
      }
      if ((ip_param[i].filter_type != SDX_FILTER) && LIVE_SRC == ip_param[i].src_type && (FALSE == ip_param[i].raw)) {
        ret = check_limitation (&ip_param[i], &enc_param[i], num_src, cmn_param->frame_rate);
        if (ret) {
          GST_ERROR ("pipeline failed to start due to parameter limitations");
          return ret;
        }
        if (NORMAL_LATENCY != enc_param[i].latency_mode && SUB_FRAME_LATENCY != enc_param[i].latency_mode) {
          GST_ERROR ("low_latency mode not supported");
          return VGST_ERROR_LOW_LATENCY_MODE_NOT_SUPPORTED;
        }
        if ((LOW_LATENCY == enc_param[i].rc_mode || SUB_FRAME_LATENCY == enc_param[i].latency_mode) && enc_param[i].b_frame) {
          GST_ERROR ("b-frame in low_latency or sub_frame latency mode not supported");
          return VGST_ERROR_B_FRAME_LOW_LATENCY_MODE_NOT_SUPPORTED;
        }
        if (enc_param[i].gop_mode != BASIC && LOW_DELAY_P != enc_param[i].gop_mode && LOW_DELAY_B != enc_param[i].gop_mode) {
          GST_ERROR ("gop-mode not supported");
          return VGST_ERROR_GOP_MODE_NOT_SUPPORTED;
        }
        if (enc_param[i].b_frame < MIN_B_FRAME || enc_param[i].b_frame > MAX_B_FRAME) {
          GST_ERROR ("b-frames should be in the range of 0-4");
          return VGST_ERROR_B_FRAME_RANGE_MISMATCH;
        }
        if (0 != enc_param[i].gop_len % (enc_param[i].b_frame+1)) {
          GST_ERROR ("GoP length should be multiple of b-frames+1 multiple");
          return VGST_ERROR_GOP_NOT_SUPPORTED;
        }
        if (enc_param[i].gop_len < MIN_GOP_LEN || enc_param[i].gop_len > MAX_GOP_LEN) {
          GST_ERROR ("GoP length should be 1-1000");
          return VGST_ERROR_GOP_LENGTH_RANGE_MISMATCH;
        }
        if (AVC != enc_param[i].enc_type && HEVC != enc_param[i].enc_type) {
          GST_ERROR ("Encoder name is incorrect");
          return VGST_ERROR_ENCODER_TYPE_NOT_SUPPORTED;
        }
        if ((HEVC == enc_param[i].enc_type) && (ip_param->width == MAX_WIDTH) && (ip_param->height == MAX_HEIGHT) && \
            (enc_param[i].slice > MAX_H265_4KP_SLICE_CNT || enc_param[i].slice < MIN_SLICE_VALUE)) {
          GST_ERROR ("Slice range for H265 in 4kp should be : %d-%d", MIN_SLICE_VALUE, MAX_H265_4KP_SLICE_CNT);
          return VGST_ERROR_SLICE_RANGE_MISMATCH;
        }
        if ((HEVC == enc_param[i].enc_type) && (ip_param->width == MAX_WIDTH/2) && (ip_param->height == MAX_HEIGHT/2) && \
            (enc_param[i].slice > MAX_H265_1080P_SLICE_CNT || enc_param[i].slice < MIN_SLICE_VALUE)) {
          GST_ERROR ("Slice range for H265 in 1080p should be : %d-%d", MIN_SLICE_VALUE, MAX_H265_1080P_SLICE_CNT);
          return VGST_ERROR_SLICE_RANGE_MISMATCH;
        }
        if ((AVC == enc_param[i].enc_type) && (ip_param->width == MAX_WIDTH) && (ip_param->height == MAX_HEIGHT) && \
            (enc_param[i].slice > MAX_H264_4KP_SLICE_CNT || enc_param[i].slice < MIN_SLICE_VALUE)) {
          GST_ERROR ("Slice range for H264 in 4kp should be : %d-%d", MIN_SLICE_VALUE, MAX_H264_4KP_SLICE_CNT);
          return VGST_ERROR_SLICE_RANGE_MISMATCH;
        }
        if ((AVC == enc_param[i].enc_type) && (ip_param->width == MAX_WIDTH/2) && (ip_param->height == MAX_HEIGHT/2) && \
            (enc_param[i].slice > MAX_H264_1080P_SLICE_CNT || enc_param[i].slice < MIN_SLICE_VALUE)) {
          GST_ERROR ("Slice range for H264 in 1080p should be : %d-%d", MIN_SLICE_VALUE, MAX_H264_1080P_SLICE_CNT);
          return VGST_ERROR_SLICE_RANGE_MISMATCH;
        }
        if ((enc_param[i].profile != BASELINE_PROFILE && enc_param[i].profile != MAIN_PROFILE && enc_param[i].profile != HIGH_PROFILE) &&
             (AVC == enc_param[i].enc_type)) {
          GST_ERROR ("Profile can be either baseline, main or high for H264");
          return VGST_ERROR_PROFILE_NOT_SUPPORTED;
        }
        if ((enc_param[i].profile != MAIN_PROFILE) && (HEVC == enc_param[i].enc_type)) {
          GST_ERROR ("Only main profile supported for H265");
          return VGST_ERROR_PROFILE_NOT_SUPPORTED;
        }
        if ((HEVC == enc_param[i].enc_type) && (enc_param[i].bitrate > MAX_H265_BITRATE)) {
          GST_ERROR ("max bitrate supported for H265 is [%d]Mbps", MAX_H265_BITRATE);
          return VGST_ERROR_BITRATE_NOT_SUPPORTED;
        }
        if ((AVC == enc_param[i].enc_type) && (enc_param[i].bitrate > MAX_H264_BITRATE)) {
          GST_ERROR ("max bitrate supported for H264 is [%d]Mbps", MAX_H264_BITRATE);
          return VGST_ERROR_BITRATE_NOT_SUPPORTED;
        }
        if ((enc_param[i].qp_mode != UNIFORM && enc_param[i].qp_mode != AUTO)) {
          GST_ERROR ("Qp mode can be either Uniform or Auto");
          return VGST_ERROR_QPMODE_NOT_SUPPORTED;
        }
        if ((enc_param[i].rc_mode != VBR && enc_param[i].rc_mode != CBR && enc_param[i].rc_mode != LOW_LATENCY)) {
          GST_ERROR ("Rate control mode can be either VBR or CBR or Low Latency");
          return VGST_ERROR_RCMODE_NOT_SUPPORTED;
        }
        if (cmn_param->sink_type == STREAM) {
          if (op_param[i].port_num < MIN_PORT_NUMBER || op_param[i].port_num > MAX_PORT_NUMBER) {
            GST_ERROR ("Port number should be in the range of [%d]-[%d]",MIN_PORT_NUMBER, MAX_PORT_NUMBER);
            return VGST_ERROR_PORT_NUM_RANGE_MISMATCH;
          }
        }
      }
      if (ip_param[i].filter_type != SDX_FILTER && config.width_in == 0) {
        #ifdef BASE_TRD
        config.fps.numerator = cmn_param->frame_rate;
        #else
        config.fps = cmn_param->frame_rate;
        #endif
        config.width_in = ip_param[i].width;
        config.height_in = ip_param[i].height;
        if (LIVE_SRC == ip_param[i].src_type && (ret = vlib_src_config(ip_param[i].device_type, &config))) {
          GST_ERROR ("vlib source config failure %d ",ret);
          return ret;
        }
      }
    }
    init_struct_params (enc_param, ip_param, op_param, cmn_param, filter_param);
    for (i =0; i< num_src; i++) {
      vgst_print_params ();
    }
    return VGST_SUCCESS;
}


gint
vgst_start_pipeline (void) {
    VGST_ERROR_LOG ret;
    // create a pipeline
    if ((ret = vgst_create_pipeline ())) {
      GST_ERROR ("pipeline creation failed !!!");
      return ret;
    }

    GST_DEBUG ("starting playback/capture");

    // run the pipeline
    if ((ret = vgst_run_pipeline ())) {
      GST_ERROR ("pipeline start failed !!!");
      return ret;
    }
    return ret;
}


void
vgst_get_fps (guint index, guint *fps) {
    get_fps (index, fps);
}


guint
vgst_get_bitrate (int index) {
    return get_bitrate (index);
}


gint
vgst_stop_pipeline () {
    /* pipeline clean up */
    GST_DEBUG ("cleaning up the pipeline");
    return stop_pipeline ();
}


gint
vgst_poll_event (int *arg, int index) {
    return poll_event (arg, index);
}

gint vgst_init(void) {
    return vlib_src_init();
}

gint vgst_uninit(void) {
    return vlib_src_uninit();
}

#ifdef BASE_TRD
gint
vgst_video_src_init (struct vlib_config_data * cfg)
{
  return vlib_video_src_init (cfg);
}

gint
vgst_init_base (struct vlib_config_data * cfg, vgst_enc_params * enc_param,
    vgst_ip_params * input_param, vgst_op_params * output_param,
    vgst_cmn_params * cmn_param, vgst_sdx_filter_params * filter_param, struct filter_tbl *ft)
{
  int ret = 0;
  unsigned int in_fourcc = cfg->fmt_in ? cfg->fmt_in : INPUT_PIX_FMT;

  ret = vlib_platform_setup (cfg);
  if (ret) {
    return ret;
  }
  ret = vlib_init_gst (cfg);
  if (ret) {
    return ret;
  }
  sdx_ft = ft;

  /* Initialize gst_lib structs */
  cmn_param->num_src = 1;
  cmn_param->sink_type = DISPLAY;
  cmn_param->frame_rate = vlib_get_fps ();
  cmn_param->driver_type = cfg->display_id;
  cmn_param->plane_id = vlib_get_active_plane_id ();

  input_param->height = vlib_get_active_height ();
  input_param->width = vlib_get_active_width ();
  input_param->filter_type = SDX_FILTER;

  input_param->format_str = (char *) &(in_fourcc);
  /* Convert YUYV to YUY2 */
  if (in_fourcc == v4l2_fourcc ('Y', 'U', 'Y', 'V')) {
    input_param->format_str = "YUY2";
  }

  return ret;
}

gint
vgst_stop_pipeline_base (void)
{
  int ret = 0;
  vgst_stop_pipeline ();
  ret = vlib_pipeline_stop_gst ();
  return ret;
}

gint
vgst_change_mode (struct vlib_config * config, unsigned int flags,
    vgst_enc_params * enc_param, vgst_ip_params * input_param,
    vgst_op_params * output_param, vgst_cmn_params * cmn_param,
    vgst_sdx_filter_params * filter_param)
{
  int ret = VLIB_SUCCESS;
  int vsrc_class;
  static int stop_flag = 0;
  struct filter_s *fs = NULL;

  /* xlnxvideosrc calls vlib_change_mode_gst internally.
   * Only call this function if v4l2src is used.
   */
  if (!g_strcmp0 (V4L2_SRC_NAME, OPENSOURCE_V4L2_SRC_NAME)) {
    ret = vlib_change_mode_gst (config);
    if (ret) {
      GST_ERROR ("vlib change_mode failed");
      return ret;
    }
  }

  if (flags && (flags & VLIB_CFG_FLAG_MEDIA_EXIT)) {
    return ret;
  }

  if (stop_flag) {
    vgst_stop_pipeline ();
  }

  if (stop_flag == 0) {
    stop_flag = 1;
  }

  vsrc_class = vlib_video_src_get_class (vlib_video_src_get (config->vsrc));

  /* Select between v4l2src and filesrc */
  input_param->device_type = config->vsrc;
  input_param->src = V4L2_SRC_NAME;
  input_param->src_type = LIVE_SRC;
  if ((vsrc_class == VLIB_VCLASS_FILE)
      && (config->vsrc == (vlib_video_src_cnt_get () - 1))) {
    if (input_param->uri == NULL) {
      create_err_msg("No video file selected. Use file browser to select "
                     "input file.", 0);
      return VLIB_ERROR_FILE_IO;
    }
    input_param->src = FILE_SRC_NAME;
    input_param->src_type = FILE_SRC;
  }

  /* Select between passthrough and sdxfilter */
  input_param->raw = TRUE;
  input_param->io_mode = VGST_V4L2_IO_MODE_DMABUF_EXPORT;
  if (config && config->type > 0) {
    input_param->raw = FALSE;
    fs = filter_type_get_obj (sdx_ft, config->type - 1);
    filter_param->filter_name = strdup (fs->dt_comp_string);
    if (fs && config->mode >= filter_type_get_num_modes (fs)) {
      GST_ERROR ("invalid filter mode '%zu' for filter '%s'\n",
          config->mode, filter_type_get_display_text (fs));
      config->mode = 0;
    }
    if (strcmp (filter_type_get_mode (fs, config->mode), "HW") == 0) {
      filter_param->filter_mode = GST_FILTER_MODE_HW;
    } else {
      filter_param->filter_mode = GST_FILTER_MODE_SW;
      input_param->io_mode = VGST_V4L2_IO_MODE_MMAP;
    }
    if (vsrc_class == VLIB_VCLASS_FILE ||
        vsrc_class == VLIB_VCLASS_UVC || vsrc_class == VLIB_VCLASS_VIVID) {
      input_param->io_mode = VGST_V4L2_IO_MODE_MMAP;
    }
  }

  /* Apply all config parameters */
  ret =
      vgst_config_options (enc_param, input_param, output_param, cmn_param,
      filter_param);
  if (ret != VGST_SUCCESS) {
    fprintf (stderr, "ERROR: vgst_config_options failed\n");
  }

  ret = vgst_start_pipeline ();

  if (config && config->type > 0)
    free (filter_param->filter_name);

  return ret;
}

int
vgst_set_event_log (int state)
{
  return 0;
}

int
vgst_get_active_height (void)
{
  return get_active_height ();
}

int
vgst_get_active_width (void)
{
  return get_active_width ();
}

float
vgst_get_event_cnt (pipeline_event event)
{
  if (event == DISPLAY_EVENT) {
    unsigned int getFpsArr[2] = {0};
    vgst_get_fps(0, &getFpsArr[0]);
    return ((float) getFpsArr[0]);
  }
  return VLIB_ERROR_OTHER;
}

const char *
vgst_error_name (vlib_error error_code)
{
  switch ((int)error_code) {
    case VLIB_ERROR_INTERNAL:
      return "VLIB Internal Error";
    case VLIB_ERROR_INVALID_PARAM:
      return "VLIB Invalid Parameter Error";
    case VLIB_ERROR_INIT:
      return "VLIB Source Init Error";
    case VLIB_ERROR_DEINIT:
      return "VLIB Source Un-init Error";
    case VLIB_ERROR_SRC_CONFIG:
      return "VLIB Source Config Error";
    case VLIB_ERROR_HDMIRX_INVALID_STATE:
      return "VLIB HDMI-RX Invalid State \nCheck Link/Resolution";
    case VLIB_ERROR_HDMIRX_INVALID_RES:
      return "VLIB HDMI-RX Invalid Resolution \nSupported Input Resolutions are 1080p and 4K";
    case VLIB_ERROR_HDMIRX_INVALID_FPS:
      return "VLIB HDMI-RX Invalid FPS \nSupported Max Input Frame Rate is 30fps";
    case VLIB_ERROR_HDMIRX_INVALID_SEQUENCE:
      return "VLIB HDMI-RX Invalid Sequence \nDerived HDMI can't run independently \n\
              HDMI source should run in parallel to derived HDMI";
    case VLIB_ERROR_OTHER:
      return "VLIB Other Error";
    case VLIB_ERROR_SET_FPS :
      return "VLIB TPG set fps failed";
    case VLIB_ERROR_MIPI_CONFIG_FAILED :
      return "VLIB MIPI Invalid State \n Check MIPI Sensor Connection";
    case VLIB_ERROR_MIPI_NOT_CONNECTED :
      return "VLIB MIPI Not Connected";
    case VLIB_ERROR_INVALID_STATE:
      return "VLIB  Source is in invalid state \nCheck Link/Resolution";
    case VLIB_ERROR_INVALID_RESOLUTION:
      return "VLIB set and get resolution mismatch";
    case VLIB_ERROR_SET_FORMAT_FAILED:
      return "VLIB unable to set format";
    case VLIB_ERROR_GET_FORMAT_FAILED:
      return "VLIB unable to get format";
    case VLIB_SUCCESS:
      return "VLIB Success";
    default:
      return "VLIB Unknown Error";
  }
}

static xlnx_vsrc_name vsrc_name[] =
{
  {VLIB_VCLASS_VIVID, "vivid"},
  {VLIB_VCLASS_UVC, "usbcam"},
  {VLIB_VCLASS_TPG, "tpg"},
  {VLIB_VCLASS_HDMII, "hdmi"},
  {VLIB_VCLASS_CSI, "mipi"},
};

const char *
vgst_get_srctype(size_t id)
{
  int vsrc_class = vlib_video_src_get_class (vlib_video_src_get (id));

  for (unsigned int i = 0; i < ARRAY_SIZE(vsrc_name); i++) {
    if (vsrc_class == vsrc_name[i].vsrc_class) {
      return vsrc_name[i].src_type;
    }
  }

  return NULL;
}

static xlnx_vsink_name vsink_name[] =
{
  {DP, "dp"},
  {HDMI_Tx, "hdmi"},
};

const char *
vgst_get_sinkname(unsigned int display_id)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(vsink_name); i++) {
    if (display_id == vsink_name[i].sink_val) {
      return vsink_name[i].sink_name;
    }
  }

  return NULL;
}
#endif
