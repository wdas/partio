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
#include <stdlib.h>
#if defined(__DARWIN__) || defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <float.h>
#include "Camera.h"

using namespace Partio;

ParticlesData* particles=0;
Camera camera;
ParticleAttribute positionAttr;
ParticleAttribute colorAttr;
ParticleAttribute alphaAttr;
static const double fov=60;

static void render()
{
    static bool inited=false;
    if(!inited){
        inited=true;
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glDepthMask(0);
        glPointSize(3);

        Vec3 bmin(FLT_MAX,FLT_MAX,FLT_MAX),bmax(-FLT_MAX,-FLT_MAX,-FLT_MAX);

        if(!particles->attributeInfo("position",positionAttr)){
            std::cerr<<"Failed to find position attribute "<<std::endl;
            exit(1);
        }else{
            for(int i=0;i<particles->numParticles();i++){
                const float* pos=particles->data<float>(positionAttr,i);
                bmin=bmin.min(Vec3(pos));
                bmax=bmax.max(Vec3(pos));
            }
            std::cout<<"count  "<<particles->numParticles()<<std::endl;
            std::cout<<"boxmin "<<bmin<<std::endl;
            std::cout<<"boxmax "<<bmax<<std::endl;
            
            
            camera.fit(fov,bmin,bmax);
        }
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
    //gluLookAt(eye.x,eye.y,eye.z,center.x,center.y,center.z,up.x,up.y,up.z);
    

    glBegin(GL_LINES);
    glColor3f(1,0,0);glVertex3f(0,0,0);glVertex3f(1,0,0);
    glColor3f(0,1,0);glVertex3f(0,0,0);glVertex3f(0,1,0);
    glColor3f(0,0,1);glVertex3f(0,0,0);glVertex3f(0,0,1);
    glEnd();
    
    //glColor3f(1,1,1);
    glPointSize(1.5);
    glBegin(GL_POINTS);
    for(int i=0;i<particles->numParticles();i++){
        const float* pos=particles->data<float>(positionAttr,i);
        const float* color=particles->data<float>(colorAttr,i);
        const float* alpha=particles->data<float>(alphaAttr,i);
        glColor3f(color[0],color[1],color[2],alpha[0]);
        glVertex3f(pos[0],pos[1],pos[2]);
    }
    glEnd();
    
    glutSwapBuffers();

}

static void mouseFunc(int button,int state,int x,int y)
{
    //std::cerr<<"mouse "<<button<<" "<<state<<" "<<x<<","<<y<<std::endl;
    if(state==GLUT_DOWN){
        if(button==GLUT_LEFT_BUTTON) camera.startTumble(x,y);
        if(button==GLUT_MIDDLE_BUTTON) camera.startPan(x,y);
        if(button==GLUT_RIGHT_BUTTON) camera.startZoom(x,y);
    }else if(state==GLUT_UP){
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


int main(int argc,char *argv[])
{
    glutInit(&argc,argv);
    if(argc!=2){
        std::cerr<<"Usage: "<<argv[0]<<" <particle file>"<<std::endl;
        return 1;
    }

    particles=read(argv[1]);
    if(particles){
        glutInitWindowSize(1024,768);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
        glutCreateWindow("partview");
        glutDisplayFunc(render);
        glutMotionFunc(motionFunc);
        glutMouseFunc(mouseFunc);
        glutMainLoop();

    }else{
        std::cerr<<"failed to read particle file "<<argv[1]<<std::endl;
        return 1;
    }
    particles->release();
    return 0;

}


