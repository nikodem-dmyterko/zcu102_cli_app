#ifndef VIDEO_CFG_H
#define VIDEO_CFG_H

#include <stddef.h>
#include <stdbool.h>
#include <video.h>

typedef enum vlib_vsrc_class video_src;

struct v_playmode {
    const char *name;
    const char *short_name;
    bool has_panel;
};

bool vsrc_get_has_panel(video_src id);
const char* vsrc_get_short_name(video_src id);
const struct vlib_vdev *vsrc_get_vd(video_src id);
void vsrc_set_vd(video_src vsrc, const struct vlib_vdev *vd);
bool vfilter_get_has_panel(const char *name);
const char* vfilter_get_short_name(const char *name);
bool vplaymode_get_has_panel(const char *name);
const char* vplaymode_get_short_name(const char *name);

#endif /* VIDEO_CFG_H */
