#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmd_helper.h"
#include "video_cfg.h"

void cmd_print_help(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("  --source h             list sources\n");
    printf("  --source NAME          select source\n");
    printf("  --sink h               list sinks\n");
    printf("  --sink NAME            select sink\n");
    printf("  --filter2d NAME/012345678  set 3x3 filter coefficients\n");
    printf("  --accel sw|hw          choose software or hardware filter\n");
    printf("  --pipeline SRC SINK MODE  create pipeline (mode: passthrough or processing)\n");
    printf("  --help                 show this message\n");
}

void cmd_print_sources(void) {
    video_cfg_list_sources();
}

void cmd_print_sinks(void) {
    video_cfg_list_sinks();
}

int cmd_select_source(const char *name) {
    return video_cfg_set_source(name);
}

int cmd_select_sink(const char *name) {
    return video_cfg_set_sink(name);
}

int cmd_set_filter2d(const char *spec) {
    char *slash = strchr(spec, '/');
    if (!slash) {
        return -1;
    }
    size_t name_len = (size_t)(slash - spec);
    if (name_len == 0 || name_len >= 64) {
        return -1;
    }
    char name[64];
    memcpy(name, spec, name_len);
    name[name_len] = '\0';
    const char *digits = slash + 1;
    if (strlen(digits) != 9) {
        return -1;
    }
    short coeff[3][3];
    for (int i = 0; i < 9; ++i) {
        if (digits[i] < '0' || digits[i] > '9') {
            return -1;
        }
        coeff[i / 3][i % 3] = (short)(digits[i] - '0');
    }
    return video_cfg_set_filter(name, coeff);
}

void cmd_set_accel(const char *mode) {
    if (mode && strcmp(mode, "sw") == 0) {
        video_cfg_set_accel(0);
    } else {
        video_cfg_set_accel(1);
    }
}

int cmd_create_pipeline(const char *src, const char *sink, const char *mode) {
    if (cmd_select_source(src) != 0) {
        fprintf(stderr, "Unknown source %s\n", src);
        return -1;
    }
    if (cmd_select_sink(sink) != 0) {
        fprintf(stderr, "Unknown sink %s\n", sink);
        return -1;
    }
    return video_cfg_create_pipeline(mode);
}

