/*
 * main.h
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */

#ifndef MAIN_H
#define MAIN_H

#include <linux/videodev2.h>

#define SYS_CHN_CNT	(4)
#define SYS_CHN_WIDTH(chn)		tWidth[chn]
#define SYS_CHN_HEIGHT(chn)		tHeight[chn]
#define SYS_CHN_FPS(chn)		tFPS[chn]
#define SYS_CHN_FMT(chn)		tFormat[chn]

#define SYS_DIS0_FPS	(60)
#define SYS_DIS0_X		(0)
#define SYS_DIS0_Y		(0)
#define SYS_DIS0_WIDTH	(1920)
#define SYS_DIS0_HEIGHT	(1080)

#define SYS_DIS1
#ifdef SYS_DIS1
#	define SYS_DIS1_FPS		(60)
#	define SYS_DIS1_X		(SYS_DIS0_WIDTH)
#	define SYS_DIS1_Y		(0)
#	define SYS_DIS1_WIDTH	(1440)
#	define SYS_DIS1_HEIGHT	(900)
#endif

static int tWidth[SYS_CHN_CNT] = {1920, 1920, 1920, 1920};
static int tHeight[SYS_CHN_CNT] = {1080, 1080, 1080, 1080};
static int tFPS[SYS_CHN_CNT] = {30, 30, 30, 30};
static int tFormat[SYS_CHN_CNT] = {V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YUYV};

#define M_MAIN main_core
int main_cap(int argc, char **argv);
int main_core(int argc, char **argv);
int main_core_file(int argc, char **argv);
int main_glu(int argc, char **argv);

#endif /* MAIN_H_ */
