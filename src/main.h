/*
 * main.h
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */

#ifndef MAIN_H
#define MAIN_H

#define SYS_CHN_CNT	(2)
#define SYS_CHN_WIDTH(chn)		tWidth[chn]
#define SYS_CHN_HEIGHT(chn)		tHeight[chn]
#define SYS_CHN_FPS(chn)		tFPS[chn]
#define DIS_FPS		30
static int tWidth[SYS_CHN_CNT] = {1920, 1920};
static int tHeight[SYS_CHN_CNT] = {1080, 1080};
static int tFPS[SYS_CHN_CNT] = {30, 60};

#define M_MAIN main_core
int main_cap(int argc, char **argv);
int main_core(int argc, char **argv);
int main_core_file(int argc, char **argv);

#endif /* MAIN_H_ */
