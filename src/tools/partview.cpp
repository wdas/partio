/*
PARTIO SOFTWARE
Copyright 2010 Disney Enterprises, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
Studios" or the names of its contributors may NOT be used to
endorse or promote products derived from this software without
specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/


#include <Partio.h>
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <sys/stat.h>
#if defined(__DARWIN__) || defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <float.h>
#include "Camera.h"


using namespace Partio;
using namespace std;

ParticlesData* particles=0;
Camera camera;
ParticleAttribute positionAttr;
ParticleAttribute colorAttr;
ParticleAttribute alphaAttr;
double fov=60;
double pointSize = 1.5;
double brightness = 0.0;
bool useColor = true;
bool useAlpha = true;
int numPoints= 0;
string  particleFile = "";
bool sourceChanged = false;
int frameNumberOGL = 0;

void restorePerspectiveProjection() {

	glMatrixMode(GL_PROJECTION);
	// restore previous projection matrix
	glPopMatrix();

	// get back to modelview mode
	glMatrixMode(GL_MODELVIEW);
}

void setOrthographicProjection() {

	// switch to projection mode
	glMatrixMode(GL_PROJECTION);

	// save previous matrix which contains the
	//settings for the perspective projection
	glPushMatrix();

	// reset matrix
	glLoadIdentity();

	// set a 2D orthographic projection
	gluOrtho2D(0, 1024, 768, 0);

	// switch back to modelview mode
	glMatrixMode(GL_MODELVIEW);
}


// default font
void *font = GLUT_STROKE_ROMAN;

void renderBitmapString(
		float x,
		float y,
		float z,
		void *font,
		char *string) {

	char *c;
	glRasterPos3f(x, y,z);
	for (c=string; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);
	}
}


static void render()
{
	static bool inited=false;
    if(!inited || sourceChanged)
	{
        inited=true;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_DEPTH);
        //glDepthMask(0); // turns the particles inside out
        glPointSize(3);

        Vec3 bmin(FLT_MAX,FLT_MAX,FLT_MAX),bmax(-FLT_MAX,-FLT_MAX,-FLT_MAX);

        if(!particles->attributeInfo("position",positionAttr))
		{
            std::cerr<<"Failed to find position attribute "<<std::endl;
            exit(1);
        }
		else
		{
            for(int i=0;i<particles->numParticles();i++)
			{
                const float* pos=particles->data<float>(positionAttr,i);
                bmin=bmin.min(Vec3(pos));
                bmax=bmax.max(Vec3(pos));
            }
            std::cout<<"count  "<<particles->numParticles()<<std::endl;
            std::cout<<"boxmin "<<bmin<<std::endl;
            std::cout<<"boxmax "<<bmax<<std::endl;

			if (!sourceChanged)
			{
				camera.fit(fov,bmin,bmax);
			}
        }
        if (!particles->attributeInfo("rgbPP", colorAttr) &
			!particles->attributeInfo("rgb", colorAttr) &
			!particles->attributeInfo("color", colorAttr) &
			!particles->attributeInfo("pointColor", colorAttr))
		{
			std::cerr<<"Failed to find color attribute "<<std::endl;
		}
		if (!particles->attributeInfo("opacity", colorAttr) &
			!particles->attributeInfo("opacityPP", colorAttr) &
			!particles->attributeInfo("alpha", colorAttr) &
			!particles->attributeInfo("alphaPP", colorAttr) &
			!particles->attributeInfo("pointOpacity", colorAttr))
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		{
			std::cerr<<"Failed to find opacity/alpha attribute "<<std::endl;
			glDisable(GL_BLEND);
		}
		sourceChanged=false;
    }

    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    int width=glutGet(GLUT_WINDOW_WIDTH);
    int height=glutGet(GLUT_WINDOW_HEIGHT);
    gluPerspective(fov,(float)width/height,0.1,100000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    camera.look();

    glBegin(GL_LINES);
    glColor3f(1,0,0);glVertex3f(0,0,0);glVertex3f(1,0,0);
    glColor3f(0,1,0);glVertex3f(0,0,0);glVertex3f(0,1,0);
    glColor3f(0,0,1);glVertex3f(0,0,0);glVertex3f(0,0,1);
    glEnd();

	/// gl info text
	setOrthographicProjection();
	glPushMatrix();
	glLoadIdentity();
	glColor3f(1.0f,1.0f,1.0f);
	char fovString[10];
	sprintf(fovString,"FOV:%i",(int)fov);
	renderBitmapString(5,20,0,GLUT_BITMAP_HELVETICA_18,fovString);
	char pointCountString[50];
	sprintf(pointCountString,"PointCount:%i",particles->numParticles());
	renderBitmapString(5,40,0,GLUT_BITMAP_HELVETICA_18,pointCountString);
	char frameNumString[50];
	sprintf(frameNumString,"FrameNum:%i",frameNumberOGL);
	glColor3f(1.0f,0.0f,0.0f);
	renderBitmapString(455,20,0,GLUT_BITMAP_HELVETICA_18,frameNumString);

	glPopMatrix();
	restorePerspectiveProjection();

    glPointSize(pointSize);
    glBegin(GL_POINTS);
    for(int i=0;i<particles->numParticles();i++){
        const float* pos=particles->data<float>(positionAttr,i);
        const float* color=particles->data<float>(colorAttr,i);
        const float* alpha=particles->data<float>(alphaAttr,i);

		float colorR = 0.75;
		float colorG = 0.75;
		float colorB = 0.75;
		float alphaVal  = 1.0;
		if (useColor)
		{
				colorR = color[0];
				colorG = color[1];
				colorB = color[2];
		}

		if (useAlpha)
		{
			alphaVal = alpha[0];
		}

		glColor4f(colorR+brightness,colorG+brightness,colorB+brightness,alphaVal);
		glVertex3f(pos[0],pos[1],pos[2]);

    }
    glEnd();

    glutSwapBuffers();

}

void  reloadParticleFile(int direction)
{
	string currentFrame = particleFile;

	string fileName;
	string origFileName;
	stringstream ss(currentFrame);

	while (getline(ss, fileName,'/'))
	{}
	origFileName = fileName;

	string token;
	stringstream fp(fileName);

	vector<string> fileParts;
	while(getline(fp, token, '.'))
	{
		fileParts.push_back(token);
	}

	char newFrameString[512];
	string numberString;
	if (fileParts.size() >=3)
	{
		int frameNumber;
		int newFrameNumber;
		numberString = fileParts[fileParts.size()-2];
		int padding = numberString.length();

		stringstream numberPart(numberString);
		if ( numberPart >> frameNumber )
		{
			frameNumberOGL = frameNumber;
			if (direction != 0)
			{
				newFrameNumber = frameNumber + direction;
				sprintf(newFrameString, "%04i", newFrameNumber);
			}
		}
		else
		{
			particleFile = currentFrame;
		}
	}

	// now replace the number in the string with the new one
	fileName.replace(fileName.find(numberString), numberString.length(), newFrameString);

	currentFrame.replace(currentFrame.find(origFileName), origFileName.length(), fileName);

	struct stat statinfo;
	int result = stat(currentFrame.c_str(),&statinfo);
	if(result >=0)
	{
		particleFile = currentFrame;
	}
	else
	{
		cout << "cache does not exist on disk" << endl;
	}
	particles=read(particleFile.c_str());
    if(particles)
	{
		sourceChanged = true;
		render();
	}
	else
	{
		cerr<<"failed to read particle file "<<particleFile <<endl;
	}
}


static void mouseFunc(int button,int state,int x,int y)
{
    //cout <<"mouse "<<button<<" "<<state<<" "<<x<<","<<y<<std::endl;
    if(state==GLUT_DOWN)
	{
        if(button==GLUT_LEFT_BUTTON) camera.startTumble(x,y);
        if(button==GLUT_MIDDLE_BUTTON) camera.startPan(x,y);
        if(button==GLUT_RIGHT_BUTTON) camera.startZoom(x,y);
		if(button== 3)
		{
			camera.startZoom(x,y);
			camera.update(x,y-5);
		}
		else if (button == 4)
		{
			camera.startZoom(x,y);
			camera.update(x,y+5);
		}
    }
    else if(state==GLUT_UP)
	{
        camera.stop();
    }

    glutPostRedisplay();
}

static void motionFunc(int x,int y)
{
    //std::cerr<<"motion "<<x<<" "<<y<<std::endl;
    camera.update(x,y);
    glutPostRedisplay();

}


static void processNormalKeys(unsigned char key, int x, int y)
{

	cout << key << endl;
	if (key == 27)  // escape closes the UI
		exit(0);

	else if (key == '=')
	{
		pointSize += 0.5;

		render();
	}
	else if (key == '-')
	{
		if (pointSize > .5)
		{
			pointSize -= 0.5;
			glutPostRedisplay();
		}
	}
	else if (key == 'c')
	{
		if (useColor) {	useColor = false; }
		else { useColor = true; }

		glutPostRedisplay();
	}
	else if (key == 'a')
	{
		if (useAlpha) { useAlpha = false; }
		else { useAlpha = true; }

		glutPostRedisplay();
	}
	else if (key == 'z')
	{
		if( fov > 10)
		{
			fov -= 10;
			glutPostRedisplay();
		}
	}
	else if (key == 'Z')
	{
		if(fov < 180)
		{
			fov += 10;
			glutPostRedisplay();
		}
	}
	else if (key == 'x')
	{
		reloadParticleFile(0);
	}

}

void  processSpecialKeys(int key, int x, int y)
{
	if( key == GLUT_KEY_UP )
	{
		brightness += .02;
	}
	else if ( key == GLUT_KEY_DOWN )
	{
		brightness -= .02;
	}
	else if (key == GLUT_KEY_RIGHT )
	{
		reloadParticleFile(1);
	}
	else if (key == GLUT_KEY_LEFT )
	{
		reloadParticleFile(-1);
	}
	glutPostRedisplay();

}

int main(int argc,char *argv[])
{
    glutInit(&argc,argv);
    if(argc!=2){
        std::cerr<<"Usage: "<<argv[0]<<" <particle file>"<<std::endl;
        return 1;
    }

	particleFile = argv[1];

    particles=read(particleFile.c_str());
    if(particles){
        glutInitWindowSize(1024,768);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
        glutCreateWindow("PartView");
		glutDisplayFunc(render);
        glutMotionFunc(motionFunc);
        glutMouseFunc(mouseFunc);
		glutKeyboardFunc(processNormalKeys);
		glutSpecialFunc(processSpecialKeys);
        glutMainLoop();

    }else{
        std::cerr<<"failed to read particle file "<<particleFile<<std::endl;
        return 1;
    }
    particles->release();
    return 0;

}


