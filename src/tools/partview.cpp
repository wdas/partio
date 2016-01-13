/*
PARTIO SOFTWARE
Copyright 2013 Disney Enterprises, Inc. All rights reserved

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

#include <cstdio>

#include "partview.h"

////////////////////////////////////
/// UTILITIES

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
    for (c=string; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
}

// DEBOUNCE SPEED timer for keyStates
void timer(int time)
{
    reloadParticleFile(time);
    glutPostRedisplay();
}

/////////////////////////////////////////
/// RENDER  callback

static void render()
{
    //cout << "render" << endl;
    if (keyStates[27])  // escape pressed,  just exit
    {
        exit(0);
    }
    anyKeyPressed = false;
    for (int x = 0; x<256; x++)
    {
        if (keyStates[x] == true)
        {
            anyKeyPressed = true;
        }
    }
    if (frameBackwardPressed || frameForwardPressed || brightnessDownPressed || brightnessUpPressed)
    {
        anyKeyPressed = true;
    }

    if (anyKeyPressed)
    {
        handleKeyInfo();
    }

    static bool inited=false;
    if (!inited || sourceChanged)
    {
        //cout << "not inited" << endl;


        glEnable(GL_DEPTH_TEST);
        glEnable(GL_DEPTH);
        //glDepthMask(0); // turns the particles inside out

        Vec3 bmin(FLT_MAX,FLT_MAX,FLT_MAX),bmax(-FLT_MAX,-FLT_MAX,-FLT_MAX);

        if (!frameMissing)
        {
            if (particles->numParticles() > 0)
            {

                if (!particles->attributeInfo("position",positionAttr) &&
                        !particles->attributeInfo("Position",positionAttr) &&
                        !particles->attributeInfo("pos",positionAttr) &&
                        !particles->attributeInfo("Pos",positionAttr) )
                {
                    std::cerr<<"Failed to find position attribute "<<std::endl;
                }
                else
                {
                    for (int i=0;i<particles->numParticles();i++)
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
                if (!sourceChanged)
                {
                    if (!particles->attributeInfo("rgbPP", colorAttr) &
                            !particles->attributeInfo("rgb", colorAttr) &
                            !particles->attributeInfo("color", colorAttr) &
                            !particles->attributeInfo("pointColor", colorAttr))
                    {
                        //std::cerr<<"Failed to find color attribute "<<std::endl;
                        colorFromIndex = -1;
                    }
                    else
                    {
                        colorFromIndex = colorAttr.attributeIndex;
                    }
                    if (!particles->attributeInfo("opacity", alphaAttr) &
                            !particles->attributeInfo("opacityPP", alphaAttr) &
                            !particles->attributeInfo("alpha", alphaAttr) &
                            !particles->attributeInfo("alphaPP", alphaAttr) &
                            !particles->attributeInfo("pointOpacity", alphaAttr))
                    {
                        alphaFromIndex = -1;
                    }
                    else
                    {
                        alphaFromIndex = alphaAttr.attributeIndex;
                    }
                }
            }
        }
        inited = true;
        sourceChanged=false;
    }

    glDisable(GL_BLEND);
    if (alphaFromIndex >=0)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
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
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(1,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0,1,0);
    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0,0,1);
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


    char colorString[75];
    if (colorFromIndex >=0)
    {
        sprintf(colorString,"Color->%s",colorAttr.name.c_str());
    }
    else
    {
        sprintf(colorString,"Color->None");
    }

    renderBitmapString(5,60,0,GLUT_BITMAP_HELVETICA_18, colorString);

    char alphaString[75];
    if (alphaFromIndex >=0)
    {
        sprintf(alphaString,"Alpha->%s",alphaAttr.name.c_str());
    }
    else
    {
        sprintf(alphaString,"Alpha->None");
    }

    renderBitmapString(5,80,0,GLUT_BITMAP_HELVETICA_18, alphaString);

    char frameNumString[50];
    if (!frameMissing)
    {
        sprintf(frameNumString,"FrameNum:%i",frameNumberOGL);
    }
    else
    {

        for (unsigned int a=0; a<=loadError.size();a++)
        {
            frameNumString[a] = loadError[a];
        }
    }
    glColor3f(1.0f,0.0f,0.0f);
    renderBitmapString(455,20,0,GLUT_BITMAP_HELVETICA_18,frameNumString);

    glPopMatrix();
    restorePerspectiveProjection();

    if (particles->numParticles() > 0)
    {
        // now setup the position/color/alpha output pointers

        const GLfloat * pos=particles->data<float>(positionAttr,0);
        glVertexPointer( 3, GL_FLOAT, 0, pos );

        float colorR = R;
        float colorG = G;
        float colorB = B;

        static GLfloat* rgba = NULL;

        if (colorFromIndex >=0)
        {
            particles->attributeInfo(colorFromIndex,colorAttr);

            const float * rgbPtr = particles->data<float>(colorAttr,0);
            if (alphaFromIndex >=0)
            {
                particles->attributeInfo(alphaFromIndex, alphaAttr);
				cout << alphaAttr.name.c_str() << endl;
                const float* alphaPtr=particles->data<float>(alphaAttr,0);
                rgba = (GLfloat *) malloc(particles->numParticles()*sizeof(GLfloat)*4);
                for (int i=0;i<particles->numParticles();i++)
                {
						rgba[i*4] = rgbPtr[i*3];
						rgba[(i*4)+1] = rgbPtr[(i*3)+1];
						rgba[(i*4)+2] = rgbPtr[(i*3)+2];
						if (alphaAttr.type == 3)
						{
							rgba[(i*4)+3] = (alphaPtr[i*3] + alphaPtr[(i*3)+1] + alphaPtr[(i*3)+2])/3;
						}
						else
						{
							rgba[(i*4)+3] = alphaPtr[i];
						}
                }
                glColorPointer(  4, GL_FLOAT, 0, rgba );
				if (rgba)
				{
					free (rgba);
				}
            }
            else
            {
                glColorPointer(  3, GL_FLOAT, 0, rgbPtr );
            }
        }
        else
        {
            if (alphaFromIndex >= 0)
            {
                particles->attributeInfo(alphaFromIndex, alphaAttr);
                rgba = (GLfloat *) malloc(particles->numParticles()*sizeof(GLfloat)*4);
                const float * alphaPtr=particles->data<float>(alphaAttr,0);
                for (int i=0;i<particles->numParticles();i++)
                {
                    rgba[i*4] = colorR+brightness;
                    rgba[(i*4)+1] = colorG+brightness;
                    rgba[(i*4)+2] = colorB+brightness;
                    rgba[(i*4)+3] = alphaPtr[i];
                }
                glColorPointer(  4, GL_FLOAT, 0, rgba );
            }
            else
            {
                rgba = (GLfloat *) malloc(particles->numParticles()*sizeof(GLfloat)*3);
                for (int i=0;i<particles->numParticles();i++)
                {
                    rgba[i*3] = colorR+brightness;
                    rgba[(i*3)+1] = colorG+brightness;
                    rgba[(i*3)+2] = colorB+brightness;
                }
                glColorPointer(  3, GL_FLOAT, 0, rgba );

            }
            if (rgba)
			{
				free (rgba);
			}
        }
    }

    glPointSize(pointSize);

    glDrawArrays( GL_POINTS, 0, particles->numParticles() );

    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );

    glEnd();

    glutSwapBuffers();



}


/////////////////////////////////////////////////
///  RELOAD particle file
///  parses the last frame and  string replaces to
///  create the  current frame and re-loads

/// TODO: push the  code to load a specific frame/find the next/prev frame in the sequence into each format reader file
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
    while (getline(fp, token, '.'))
    {
        fileParts.push_back(token);
    }

    char newFrameString[512];
    string numberString;
    string extension;
    int pdcMultiplier = 1;
    string paddingString = "%04i";

    if (fileParts.size() >=2)
    {
        int frameNumber;
        int newFrameNumber;
        numberString = fileParts[fileParts.size()-2];
        extension = fileParts[fileParts.size()-1];

        if (fileParts.size() <3)
        {
            if (extension == "mc") // assuming we have a real Maya NParticle export not a custom one  ex.. nParticleShape1Frame42.mc
            {
                numberString.replace(numberString.find("Frame"), 5, ".");
                string nParticleName;
                stringstream npName(numberString);

                while (getline(npName, nParticleName, '.'))
                {
                    fileParts.push_back(nParticleName);
                }
                numberString = fileParts[fileParts.size()-1];
                paddingString = "%i";
            }
            else
            {
                // SINGLE FILE?
                numberString = "";
                paddingString = "";
            }
        }
        else
        {
            if (extension  == "pdc")
            {
                pdcMultiplier = 250;
                paddingString = "%i";
            }

            /// trying to cover all our bases here.. there's probably a better way to do this.
            else if (extension == "pdb" && numberString.substr(1,1) != "0")
            {
                paddingString = "%i";
            }

            else
            {
                int padding = numberString.length();
                stringstream ss;
                ss << padding;
                paddingString = "%0";
                paddingString += ss.str();
                paddingString += "i";
            }

        }

        stringstream numberPart(numberString);
        if ( numberPart >> frameNumber )
        {

            if (direction != 0)
            {
                newFrameNumber = frameNumber + direction*pdcMultiplier;

                sprintf(newFrameString, paddingString.c_str(), newFrameNumber);
                frameNumberOGL = newFrameNumber/pdcMultiplier;
            }
            else
            {
                sprintf(newFrameString, paddingString.c_str(), frameNumber);
                frameNumberOGL = frameNumber/pdcMultiplier;
            }
            // now replace the number in the string with the new one
            fileName.replace(fileName.rfind(numberString), numberString.length(), newFrameString);

            currentFrame.replace(currentFrame.find(origFileName), origFileName.length(), fileName);
            particleFile = currentFrame;
        }
        else
        {
            particleFile = currentFrame;
        }
    }

    if (particleFile != lastParticleFile)
    {
        struct stat statinfo;
        int result = stat(particleFile.c_str(),&statinfo);
        if (result >=0)
        {

            PARTIO::ParticlesData* newParticles;
			newParticles = particles;
			particles = NULL;
			if (newParticles != NULL)
			{
				//cout << particles << endl;
				//particles = particles->reset();
				newParticles->release();
			}
			particles = PARTIO::read(particleFile.c_str());

            if (!glutGetWindow()) {
                return;
            }
            if (particles)
            {
                frameMissing = false;
                sourceChanged = true;
                render();
                //glutPostRedisplay();
                cout << particleFile << endl;
                lastParticleFile = particleFile;
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
            particleFile = origFileName;

        }
    }
}


///////////////////////////////////////////
/// PROCESS Mouse / Keyboard functions

static void mouseFunc(int button,int state,int x,int y)
{
    if (state==GLUT_DOWN)
    {
        if (button==GLUT_LEFT_BUTTON) camera.startTumble(x,y);
        if (button==GLUT_MIDDLE_BUTTON) camera.startPan(x,y);
        if (button==GLUT_RIGHT_BUTTON) camera.startZoom(x,y);

        if (button== 3)
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
    else if (state==GLUT_UP)
    {
        camera.stop();
    }

    glutPostRedisplay();
}

static void motionFunc(int x,int y)
{
    int mod = glutGetModifiers();
    if (mod == GLUT_ACTIVE_ALT)
    {
        camera.update(x,y);
        glutPostRedisplay();
    }

}

// this is the main function called by the render loop when it needs to redraw
void handleKeyInfo()
{

    bool validKey = false;

    if (keyStates['='])
    {
        pointSize += 0.5;
        validKey = true;
    }
    else if (keyStates['-'])
    {
        if (pointSize > .5)
        {
            pointSize -= 0.5;
        }
        validKey = true;
    }

    if (keyStates['z'])
    {
        if ( fov > 10)
        {
            fov -= 5;
        }
        keyStates['z'] = false;
        validKey = true;
    }
    else if (keyStates['Z'])
    {
        if (fov < 180)
        {
            fov += 5;
        }
        keyStates['Z'] = false;
        validKey = true;
    }

    if (brightnessUpPressed)
    {
        if (brightness <= 1)
        {
            brightness += .02;
        }
        brightnessDownPressed = false;
        validKey = true;
    }
    if (brightnessDownPressed)
    {
        if (brightness >=0)
        {
            brightness -= .02;
        }
        brightnessUpPressed = false;
        validKey = true;
    }

    // we have to put in a keyrepeat delay for frame changes or else the speed of the refresh banks up too many
    static GLuint Clock=glutGet(GLUT_ELAPSED_TIME);
    static GLfloat deltaT;
    Clock = glutGet(GLUT_ELAPSED_TIME);
    deltaT=Clock-PreviousClock;

    if (deltaT > 125)  // initial key press delay
    {
        if (frameForwardPressed)
        {
            glutTimerFunc(125,timer,1);
            frameBackwardPressed = false;
        }
        else if (frameBackwardPressed)
        {
            glutTimerFunc(125,timer,-1);
            frameForwardPressed = false;

        }
        PreviousClock=glutGet(GLUT_ELAPSED_TIME);
    }

    if (validKey)
    {
        glutPostRedisplay();
    }
}

static void processNormalKeys(unsigned char key, int x, int y)
{
    keyStates[key] = true;
    glutPostRedisplay();
}

static void processNormalUpKeys(unsigned char key, int x, int y)
{
    keyStates[key] =false;
    glutPostRedisplay();
}


static void  processSpecialKeys(int key, int x, int y)
{

    if (key == GLUT_KEY_UP)
    {
        brightnessUpPressed = true;
    }
    if (key == GLUT_KEY_DOWN)
    {
        brightnessDownPressed = true;
    }

    if (key == GLUT_KEY_RIGHT )
    {
        frameForwardPressed = true;
        frameBackwardPressed = false;
    }
    else if (key == GLUT_KEY_LEFT )
    {
        frameBackwardPressed = true;
        frameForwardPressed = false;
    }
    glutPostRedisplay();

}

void processSpecialUpKeys(int key, int x, int y)
{
    if ( key == GLUT_KEY_UP )
    {
        brightnessUpPressed = false;
    }
    if ( key == GLUT_KEY_DOWN )
    {
        brightnessDownPressed = false;
    }
    if ( key == GLUT_KEY_RIGHT )
    {
        frameForwardPressed = false;
    }
    if ( key == GLUT_KEY_LEFT )
    {
        frameBackwardPressed = false;
    }
    glutPostRedisplay();
}

/// END PROCESSING MOUSE/KEYS
//////////////////////////////////////////////


/// channel control menus
//////////////////////////////////////////////
int buildPopupMenu()
{
    int numAttr=particles->numAttributes();
    int colorMenu, alphaMenu, mainMenu;
    colorMenu = glutCreateMenu (colorFromMenu);
    glutAddMenuEntry("None", -1);
    for (int i = 0; i < numAttr; i++)
    {
        PARTIO::ParticleAttribute attr;
        particles->attributeInfo(i,attr);
        glutAddMenuEntry(attr.name.c_str(), i);
    }

    alphaMenu = glutCreateMenu(alphaFromMenu);
    glutAddMenuEntry("None", -1);
    for (int i = 0; i < numAttr; i++)
    {
        PARTIO::ParticleAttribute attr;
        particles->attributeInfo(i,attr);
        glutAddMenuEntry(attr.name.c_str(), i);
    }

    mainMenu = glutCreateMenu(processMainMenu);
    glutAddSubMenu("Color From->", colorMenu);
    glutAddSubMenu("Opacity From->",alphaMenu);

    return mainMenu;
}

void processMainMenu(int idCommand)
{
    // just a placeholder since it just contains the other
}

void colorFromMenu(int idCommand)
{
    colorFromIndex = idCommand;
    glutPostRedisplay();
}

void alphaFromMenu(int idCommand)
{
    alphaFromIndex = idCommand;
    glutPostRedisplay();
}


//////////////////////////////////////////////////////
/// MAIN

int main(int argc,char *argv[])
{

    // initialize variables
    particles = 0;
    fov=60;
    pointSize = 1.5;
    brightness = 0.0;
    numPoints= 0;
    particleFile = "";
    sourceChanged = false;
    frameNumberOGL = 0;
    keyStates = new bool[256];
    for (int x = 0; x<256; x++)
    {
        keyStates[x] = false;
    }
    frameForwardPressed = false;
    frameBackwardPressed = false;
    brightnessUpPressed = false;
    brightnessDownPressed =false;
    PreviousClock = 0;
    frameMissing = false;
    loadError = "";
    anyKeyPressed = false;

    R = 1;
    G = 1;
    B = 1;

    glutInit(&argc,argv);
    if (argc!=2)
    {
        std::cerr<<"Usage: "<<argv[0]<<" <particle file>"<<std::endl;
        return 1;
    }

    particleFile = argv[1];

    reloadParticleFile(0);

    if (particles)
    {
        glutInitWindowSize(1024,768);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
        glutCreateWindow("PartView");
        glutTimerFunc(200,timer,0);
        glutDisplayFunc(render);
        glutMotionFunc(motionFunc);
        glutMouseFunc(mouseFunc);
        glutKeyboardFunc(processNormalKeys);
        glutKeyboardUpFunc(processNormalUpKeys);
        glutSpecialUpFunc(processSpecialUpKeys);
        glutSpecialFunc(processSpecialKeys);
        glutIgnoreKeyRepeat(true);
        buildPopupMenu();
        glutAttachMenu(GLUT_RIGHT_BUTTON);
        glutMainLoop();
    }
    else
    {
        std::cerr<<"failed to read particle file "<<particleFile<<std::endl;
        return 1;
    }
    particles->release();
	delete [] keyStates;
    return 0;
}
