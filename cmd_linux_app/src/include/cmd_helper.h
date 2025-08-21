#ifndef CMD_HELPER_H
#define CMD_HELPER_H

void cmd_print_help(const char *prog);
void cmd_print_sources(void);
void cmd_print_sinks(void);
int  cmd_select_source(const char *name);
int  cmd_select_sink(const char *name);
int  cmd_set_filter2d(const char *spec);
void cmd_set_accel(const char *mode);
int  cmd_create_pipeline(const char *src, const char *sink, const char *mode);

#endif /* CMD_HELPER_H */
