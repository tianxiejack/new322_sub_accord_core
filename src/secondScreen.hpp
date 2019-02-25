/*
 * secondScreen.hpp
 *
 *  Created on: Feb 22, 2019
 *      Author: ubuntu
 */

#ifndef SECONDSCREEN_HPP_
#define SECONDSCREEN_HPP_

#include "crcoreSecondScreen.hpp"

class CT
{
public:
	CT(){OSA_printf("%s %d %s", __FILE__, __LINE__, __func__);};
	virtual ~CT(){OSA_printf("%s %d %s", __FILE__, __LINE__, __func__);};
};
class CSecondScreen : public CSecondScreenBase
{
	CT m_ct;
public:
	CSecondScreen(const cv::Rect& rc, int fps, int fontSize = 45, const char* fontFile = NULL):
		CSecondScreenBase(rc, fps, true, fontSize, fontFile){};
	virtual ~CSecondScreen(){};
	virtual void OnRender(int stepIdx, int stepSub, int context)
	{
		if(stepIdx == RENDER_HOOK_RUN_SWAP){
			//glUseProgram(0);
			//glClear(GL_COLOR_BUFFER_BIT);
			glColor3f(1.0, 0.0, 0.0);
			glViewport(0, 0, 500, 500);
			glRasterPos2f(200, 200);
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '0');
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '1');
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '2');
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '3');
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '4');
		}
	}
};

#endif /* SECONDSCREEN_HPP_ */
