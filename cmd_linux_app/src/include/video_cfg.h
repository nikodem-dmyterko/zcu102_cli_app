#ifndef VIDEO_CFG_H
#define VIDEO_CFG_H

#include <vgst_lib.h>
#include <vgst_utils.h>

void video_cfg_init(void);
void video_cfg_list_sources(void);
void video_cfg_list_sinks(void);
int  video_cfg_set_source(const char *name);
int  video_cfg_set_sink(const char *name);
int  video_cfg_set_filter(const char *name, const short coeff[3][3]);
void video_cfg_set_accel(int hw);
int  video_cfg_create_pipeline(const char *mode);
void video_cfg_cleanup(void);

#endif /* VIDEO_CFG_H */
