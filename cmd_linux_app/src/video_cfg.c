#include <string.h>
#include <strings.h>  // dla strcasecmp
#include <video_cfg.h>

struct v_source {
    video_src src;
    const char *short_name;
    bool has_panel;
    const struct vlib_vdev *vd;
};

struct v_filter {
    const char *name;
    const char *short_name;
    bool has_panel;
};

struct v_sink {
    const char *name;
    const char *short_name;
};

static struct v_playmode playmode_tbl[] = {
    {"Manual", "Manual", true},
    {"Demo", "Demo", true},
};

static struct v_source vsrc_tbl[] = {
    {VLIB_VCLASS_VIVID, "TPG (SW)", false, NULL},
    {VLIB_VCLASS_UVC, "USB", false, NULL},
    {VLIB_VCLASS_TPG, "TPG (PL)", true, NULL},
    {VLIB_VCLASS_HDMII, "HDMI", false, NULL},
    {VLIB_VCLASS_CSI, "CSI", true, NULL},
    {VLIB_VCLASS_FILE, "FILE", true, NULL},
};

static struct v_filter filter_tbl[] = {
    {"2D Filter", "2D Filter", true},
    {"Simple Posterize", "Simple Posterize", false},
    {"Optical Flow", "Optical Flow", false},
};

static struct v_sink vsnk_tbl[] = {
    {"HDMI", "HDMI"},
    {"FILE", "FILE"},
};

static struct v_source* get_vsrc_by_id(video_src id) {
    for (unsigned int i = 0; i < ARRAY_SIZE(vsrc_tbl); i++) {
        if (id == vsrc_tbl[i].src) {
            return &vsrc_tbl[i];
        }
    }
    return NULL;
}

bool vsrc_get_has_panel(video_src id) {
    struct v_source *vs = get_vsrc_by_id(id);
    return vs ? vs->has_panel : false;
}

const char* vsrc_get_short_name(video_src id) {
    struct v_source *vs = get_vsrc_by_id(id);
    return vs ? vs->short_name : NULL;
}

struct v_filter* get_vfilter_by_name(const char *name) {
    for (unsigned int i = 0; i < ARRAY_SIZE(filter_tbl); i++) {
        if (strcasecmp(name, filter_tbl[i].name) == 0) {
            return &filter_tbl[i];
        }
    }
    return NULL;
}

bool vfilter_get_has_panel(const char *name) {
    struct v_filter *vf = get_vfilter_by_name(name);
    return vf ? vf->has_panel : false;
}

const char* vfilter_get_short_name(const char *name) {
    struct v_filter *vf = get_vfilter_by_name(name);
    return vf ? vf->short_name : NULL;
}

struct v_playmode* get_vplaymode_by_name(const char *name) {
    for (unsigned int i = 0; i < ARRAY_SIZE(playmode_tbl); i++) {
        if (strcasecmp(name, playmode_tbl[i].name) == 0) {
            return &playmode_tbl[i];
        }
    }
    return NULL;
}

bool vplaymode_get_has_panel(const char *name) {
    struct v_playmode *vp = get_vplaymode_by_name(name);
    return vp ? vp->has_panel : false;
}

const char* vplaymode_get_short_name(const char *name) {
    struct v_playmode *vp = get_vplaymode_by_name(name);
    return vp ? vp->short_name : NULL;
}

const struct vlib_vdev *vsrc_get_vd(video_src id) {
    struct v_source *vs = get_vsrc_by_id(id);
    return vs ? vs->vd : NULL;
}

void vsrc_set_vd(video_src vsrc, const struct vlib_vdev *vd) {
    struct v_source *vs = get_vsrc_by_id(vsrc);
    if (!vs) return;
    vs->vd = vd;
}

size_t vsrc_count(void) {
    return ARRAY_SIZE(vsrc_tbl);
}

const char *vsrc_name(size_t index) {
    if (index >= ARRAY_SIZE(vsrc_tbl)) return NULL;
    return vsrc_tbl[index].short_name;
}

int vsrc_index_by_name(const char *name) {
    for (size_t i = 0; i < ARRAY_SIZE(vsrc_tbl); ++i) {
        if (strcasecmp(name, vsrc_tbl[i].short_name) == 0)
            return (int)i;
    }
    return -1;
}

size_t vsnk_count(void) {
    return ARRAY_SIZE(vsnk_tbl);
}

const char *vsnk_name(size_t index) {
    if (index >= ARRAY_SIZE(vsnk_tbl)) return NULL;
    return vsnk_tbl[index].short_name;
}

int vsnk_index_by_name(const char *name) {
    for (size_t i = 0; i < ARRAY_SIZE(vsnk_tbl); ++i) {
        if (strcasecmp(name, vsnk_tbl[i].short_name) == 0)
            return (int)i;
    }
    return -1;
}
