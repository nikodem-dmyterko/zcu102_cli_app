#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "cmd_helper.h"
#include "video_cfg.h"

int main(int argc, char **argv) {
    video_cfg_init();

    static struct option opts[] = {
        {"source",    required_argument, 0, 's'},
        {"sink",      required_argument, 0, 'k'},
        {"filter2d",  required_argument, 0, 'f'},
        {"accel",     required_argument, 0, 'a'},
        {"pipeline",  no_argument,       0, 'p'},
        {"help",      no_argument,       0, 'h'},
        {0,0,0,0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "", opts, NULL)) != -1) {
        switch (c) {
        case 's':
            if (strcmp(optarg, "h") == 0) {
                cmd_print_sources();
            } else if (cmd_select_source(optarg) != 0) {
                fprintf(stderr, "Unknown source %s\n", optarg);
            }
            break;
        case 'k':
            if (strcmp(optarg, "h") == 0) {
                cmd_print_sinks();
            } else if (cmd_select_sink(optarg) != 0) {
                fprintf(stderr, "Unknown sink %s\n", optarg);
            }
            break;
        case 'f':
            if (cmd_set_filter2d(optarg) != 0) {
                fprintf(stderr, "Invalid filter specification\n");
            }
            break;
        case 'a':
            cmd_set_accel(optarg);
            break;
        case 'p':
            if (optind + 2 >= argc) {
                cmd_print_help(argv[0]);
                return 1;
            }
            {
                const char *src = argv[optind++];
                const char *sink = argv[optind++];
                const char *mode = argv[optind++];
                if (cmd_create_pipeline(src, sink, mode) != 0) {
                    fprintf(stderr, "Failed to create pipeline\n");
                    return 1;
                }
            }
            break;
        case 'h':
        default:
            cmd_print_help(argv[0]);
            return 0;
        }
    }

    return 0;
}

