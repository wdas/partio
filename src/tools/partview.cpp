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


#include "partview.h"





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
		//cout << "not inited" << endl;
        inited=true;
		colorMissing = false;
		colorMissing = false;


		glEnable(GL_DEPTH_TEST);
		glEnable(GL_DEPTH);
        //glDepthMask(0); // turns the particles inside out
        glPointSize(3);

        Vec3 bmin(FLT_MAX,FLT_MAX,FLT_MAX),bmax(-FLT_MAX,-FLT_MAX,-FLT_MAX);

		if (!frameMissing)
		{

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
				//std::cout<<"count  "<<particles->numParticles()<<std::endl;
				//std::cout<<"boxmin "<<bmin<<std::endl;
				//std::cout<<"boxmax "<<bmax<<std::endl;

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
				//std::cerr<<"Failed to find color attribute "<<std::endl;
				colorMissing = true;
			}
			if (!particles->attributeInfo("opacity", colorAttr) &
				!particles->attributeInfo("opacityPP", colorAttr) &
				!particles->attributeInfo("alpha", colorAttr) &
				!particles->attributeInfo("alphaPP", colorAttr) &
				!particles->attributeInfo("pointOpacity", colorAttr))
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			{
				alphaMissing = true;
				//std::cerr<<"Failed to find opacity/alpha attribute "<<std::endl;
				glDisable(GL_BLEND);
			}
		}
		sourceChanged=false;
    }

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
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
	if (!frameMissing)
	{
		sprintf(frameNumString,"FrameNum:%i",frameNumberOGL);
	}
	else
	{

		for (int a=0; a<=loadError.size();a++)
		{
			frameNumString[a] = loadError[a];
		}
	}
	glColor3f(1.0f,0.0f,0.0f);
	renderBitmapString(455,20,0,GLUT_BITMAP_HELVETICA_18,frameNumString);

	glPopMatrix();
	restorePerspectiveProjection();

    glPointSize(pointSize);


	// now setup the position/color/alpha output pointers

	const float * pos=particles->data<float>(positionAttr,0);
	glVertexPointer( 3, GL_FLOAT, 0, pos );

	float colorR = 0.75;
	float colorG = 0.75;
	float colorB = 0.75;
	float alphaVal  = 1.0;

	if (useColor && !colorMissing)
	{
		const float * rgb =particles->data<float>(colorAttr,0);
		if (useAlpha && !alphaMissing)
		{
			const float* alpha=particles->data<float>(alphaAttr,0);
			float * rgba = (float *) malloc(particles->numParticles()*sizeof(float)*4);
			for(int i=0;i<particles->numParticles();i++)
			{
				rgba[i*3] = rgb[i*3];
				rgba[(i*3)+1] = rgb[(i*3)+1];
				rgba[(i*3)+2] = rgb[(i*3)+2];
				rgba[(i*3)+3] = alpha[i];
			}
			glColorPointer(  4, GL_FLOAT, 0, rgba );
		}
		else
		{
			glColorPointer(  3, GL_FLOAT, 0, rgb );
		}
	}
	else
	{
		float * rgba;
		const float* alpha;

		if(useAlpha && !alphaMissing)
		{
			rgba = (float *) malloc(particles->numParticles()*sizeof(float)*4);
			const float* alpha=particles->data<float>(alphaAttr,0);
			for(int i=0;i<particles->numParticles();i++)
			{
				rgba[i*4] = colorR+brightness;
				rgba[(i*4)+1] = colorG+brightness;
				rgba[(i*4)+2] = colorB+brightness;
				rgba[(i*4)+3] = alpha[i];
			}
			glColorPointer(  4, GL_FLOAT, 0, rgba );
		}
		else
		{
			rgba = (float *) malloc(particles->numParticles()*sizeof(float)*3);
			for(int i=0;i<particles->numParticles()*3;i++)
			{
				rgba[i] = colorR+brightness;
			}
			glColorPointer(  3, GL_FLOAT, 0, rgba );
		}
	}

	glDrawArrays( GL_POINTS, 0, particles->numParticles() );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

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
			else
			{
				sprintf(newFrameString, "%04i", frameNumber);
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
	particleFile = currentFrame;

	struct stat statinfo;
	int result = stat(currentFrame.c_str(),&statinfo);
	if(result >=0)
	{
		particles=read(particleFile.c_str());
		if (!glutGetWindow()) {return;}
		if(particles)
		{
			frameMissing = false;
			sourceChanged = true;
			render();
		}
		else
		{
			frameMissing = true;
			loadError = "Couldn't load particle file!";
		}
	}
	else
	{
		frameMissing = true;
		loadError  = "FILE MISSING!!! on disk";
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
		int mod = glutGetModifiers();

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
	int mod = glutGetModifiers();
	if(mod == GLUT_ACTIVE_ALT)
	{
		//std::cerr<<"motion "<<x<<" "<<y<<std::endl;
		camera.update(x,y);
		glutPostRedisplay();
	}

}


void idle(int value)
{
	if (anyKeyPressed)
	{
		if (keyStates[27])  // escape pressed,  just exit
		{
			exit(0);
		}

		static GLuint Clock=glutGet(GLUT_ELAPSED_TIME);
		static GLfloat deltaT;
		Clock = glutGet(GLUT_ELAPSED_TIME);
		deltaT=Clock-PreviousClock;

		if (deltaT > 200)  // initial key press delay
		{

			if (keyStates['='])
			{
				pointSize += 0.5;
			}
			else if (keyStates['-'])
			{
				if (pointSize > .5)
				{
					pointSize -= 0.5;
				}
			}

			if (keyStates['z'])
			{
				if( fov > 10)
				{
					fov -= 5;
				}
			}
			else if (keyStates['Z'])
			{
				if(fov < 180)
				{
					fov += 5;
				}
			}

			if (brightnessDownPressed)
			{
				if (brightness >= -1)
				brightness -= .02;
			}
			if (brightnessUpPressed)
			{
				if (brightness <= 1)
				brightness += .02;
			}

			if(frameForwardPressed)
			{
				reloadParticleFile(1);
			}
			else if (frameBackwardPressed)
			{
				reloadParticleFile(-1);
			}
			else
			{
				glutPostRedisplay();
			}

		}
		glutTimerFunc(10,idle,0);
	}
	glutPostRedisplay();
}

static void processNormalKeys(unsigned char key, int x, int y)
{
	anyKeyPressed = true;
	keyStates[key] = true;

	if (keyStates['='])
	{
		pointSize += 0.5;
	}
	else if (keyStates['-'])
	{
		if (pointSize > .5)
		{
			pointSize -= 0.5;
		}
	}
	if (keyStates['c'])
	{
		if (useColor) {	useColor = false; }
		else { useColor = true; }
	}

	if (keyStates['a'])
	{
		if (useAlpha) { useAlpha = false; }
		else { useAlpha = true; }
	}

	if (keyStates['z'])
	{
		if( fov > 10)
		{
			fov -= 5;
		}
	}
	else if (keyStates['Z'])
	{
		if(fov < 180)
		{
			fov += 5;
		}
	}
	glutTimerFunc(200,idle,0);
	glutPostRedisplay();
	PreviousClock=glutGet(GLUT_ELAPSED_TIME);
}

static void processNormalUpKeys(unsigned char key, int x, int y)
{
	keyStates[key] =false;
	anyKeyPressed = false;
}


static void  processSpecialKeys(int key, int x, int y)
{
	anyKeyPressed = true;

	if (key == GLUT_KEY_UP)
	{
		if (brightness <= 1)
		brightness += .02;
		brightnessUpPressed = true;
		PreviousClock=glutGet(GLUT_ELAPSED_TIME);
	}
	if (key == GLUT_KEY_DOWN)
	{
		if (brightness >=1)
		brightness -= .02;
		brightnessDownPressed = true;
		PreviousClock=glutGet(GLUT_ELAPSED_TIME);
	}

	if (key == GLUT_KEY_RIGHT )
	{
		reloadParticleFile(1);
		frameForwardPressed = true;
		PreviousClock=glutGet(GLUT_ELAPSED_TIME);
	}
	else if (key == GLUT_KEY_LEFT )
	{
		reloadParticleFile(-1);
		frameBackwardPressed = true;
		PreviousClock=glutGet(GLUT_ELAPSED_TIME);
	}
	glutTimerFunc(200,idle,0);
	glutPostRedisplay();

}

void processSpecialUpKeys(int key, int x, int y)
{
	anyKeyPressed = false;
	if( key == GLUT_KEY_UP )
	{
		brightnessUpPressed = false;
	}
	if( key == GLUT_KEY_DOWN )
	{
		brightnessDownPressed = false;
	}
	if( key == GLUT_KEY_RIGHT )
	{
		frameForwardPressed = false;
	}
	if( key == GLUT_KEY_LEFT )
	{
		frameBackwardPressed = false;
	}
}

int main(int argc,char *argv[])
{

	// initialize variables

	particles=0;
	fov=60;
	pointSize = 1.5;
	brightness = 0.0;
	useColor = true;
	useAlpha = true;
	numPoints= 0;
	particleFile = "";
	sourceChanged = false;
	frameNumberOGL = 0;
	keyStates = new bool[256];

    glutInit(&argc,argv);
    if(argc!=2){
        std::cerr<<"Usage: "<<argv[0]<<" <particle file>"<<std::endl;
        return 1;
    }

	particleFile = argv[1];

	reloadParticleFile(0);

    if(particles){
        glutInitWindowSize(1024,768);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
        glutCreateWindow("PartView");
		glutTimerFunc(200,idle,0);
		glutDisplayFunc(render);
        glutMotionFunc(motionFunc);
        glutMouseFunc(mouseFunc);
		glutKeyboardFunc(processNormalKeys);
		glutKeyboardUpFunc(processNormalUpKeys);
		glutSpecialUpFunc(processSpecialUpKeys);
		glutSpecialFunc(processSpecialKeys);
		glutIgnoreKeyRepeat(1);
        glutMainLoop();

    }else{
        std::cerr<<"failed to read particle file "<<particleFile<<std::endl;
        return 1;
    }
    particles->release();
    return 0;

}


