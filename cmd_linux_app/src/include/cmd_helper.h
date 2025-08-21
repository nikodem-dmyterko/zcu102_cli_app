#ifndef CMD_HELPER_H
#define CMD_HELPER_H

#include <stdbool.h>
#include <pthread.h>
#include "video_cfg.h"

struct filter_tbl;

struct maincontroller {
    struct vlib_config config;
    struct filter_tbl *ft;
    int videoCtrl;
    bool demo_src;
    bool self_initialized;
    int showMemoryThroughput;

	int demo_filter_loop_count;
	int demo_src_loop_count;
	int demo_perset_loop_count;
    int demo_sequence[9][3];
	int demoSequenceLength;

    pthread_t fps_thread;
    pthread_t demo_thread;
};

enum demoSequenceParams{
	SOURCE_TYPE,
	FILTER_TYPE,
	FILTER_MODE
};


enum MemData
{
	videoSrc,
	filter,
	videoPort,
	NMemData
};

void cmd_init(struct maincontroller *mc, struct vlib_config cfg, struct filter_tbl *ft);
void setVideo(struct maincontroller *mc, int play);
void setFilterMode(struct maincontroller *mc, int filter, int isPlaying);
void setFilterType(struct maincontroller *mc, int filter, int isPlaying);
void setInput(struct maincontroller *mc, int input, int isPlaying, const char *defaultfilename, bool hasFilePath);

void setTPGPattern(int input);
void setPreset(struct maincontroller *mc, int input);

// TPG functions
void setBoxSize(int size);
void setBoxColor(int color);
void setBoxSpeed(int speed);
void setCrossRows(int rows);
void setCrossColumns(int columns);
void setZoneH(int h);
void setZoneHDelta(int h);
void setZoneV(int v);
void setZoneVDelta(int v);

// CSI functions
void csiredgamma(int redg);
void csigreengamma(int greeng);
void csibluegamma(int blueg);
void csicontrast(int contrast);
void csibrightness(int brightness);
void csiredgain(int redgain);
void csigreengain(int greengain);
void csibluegain(int bluegain);
void csiexposure(int exposure);
void csiimxgain(int imxgain);
void setTestPattern(int testpattern);
void setVerticalFlip(int flip);

//
void filterCoeff(struct maincontroller *mc, int c00,int c01,int c02,int c10,int c11,int c12,int c20,int c21,int c22);
void closeall();
void videoSrcLoop(struct maincontroller *mc);
//void demoSequence(QVariantList seqList);
//void updateDemoTimer(int timerval);
//void updateThroughput();

#endif /* CMD_HELPER_H */
