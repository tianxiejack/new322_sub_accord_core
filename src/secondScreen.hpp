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
	cv::Rect m_rc;
public:
	CSecondScreen(const cv::Rect& rc, int fps, int fontSize = 45, const char* fontFile = NULL):
		CSecondScreenBase(rc, fps, true, fontSize, fontFile),m_rc(rc){};
	virtual ~CSecondScreen(){};
	void disp_fps(){
	    static GLint frames = 0;
	    static GLint t0 = 0;
	    static char  fps_str[20] = {'\0'};
	    GLint t = glutGet(GLUT_ELAPSED_TIME);
	    sprintf(fps_str, "%6.1f FPS\n", 0.0);
	    if (t - t0 >= 200) {
	        GLfloat seconds = (t - t0) / 1000.0;
	        GLfloat fps = frames / seconds;
	        sprintf(fps_str, "%6.1f FPS\n", fps);
	        //printf("%6.1f FPS\n", fps);
	        t0 = t;
	        frames = 0;
	    }
	    glColor3f(1.0, 1.0, 1.0);
	    glRasterPos2f(0.5, 0.5);
	    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char *)fps_str);
	    frames++;
	}
	virtual void OnRender(int stepIdx, int stepSub, int context)
	{
		if(stepIdx == RENDER_HOOK_RUN_SWAP){
			glUseProgram(0);
			//M3DVector4f vcolor = {1.0,1.0,1.0,1.0};
			//cr_osd::glShaderManager.UseStockShader(GLT_SHADER_IDENTITY, vcolor);
			//glClear(GL_COLOR_BUFFER_BIT);
			glViewport(0, 0, m_rc.width, m_rc.height);
			glRasterPos2f(0.0, 0.0);
			glColor4f(0.0, 1.0, 1.0, 1.0f);
			char  str[20] = "0123456789";
			glutBitmapString(GLUT_BITMAP_8_BY_13, (unsigned char *)str);
		}
	}
};

#endif /* SECONDSCREEN_HPP_ */
