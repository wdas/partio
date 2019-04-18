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

#ifndef _Camera_h_
#define _Camera_h_

#include <PartioVec3.h>

#ifdef WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#endif //

#ifdef PARTIO_WIN32
#define M_PI 3.141592653589793238
#endif

class Camera
{
public:
    Partio::Vec3 lookAt;
    float theta;
    float phi;
    float distance;

    Camera()
        :lookAt(0,0,0),
        theta(0.8),phi(0.3),
        distance(3),
        tumble(false),pan(false),zoom(false)
    {}

    void look() const
    {
        Partio::Vec3 view=distance*Partio::Vec3(sin(theta)*cos(phi),sin(phi),cos(theta)*cos(phi));
        Partio::Vec3 up=distance*Partio::Vec3(sin(theta)*cos(phi+M_PI/2),sin(phi+M_PI/2),cos(theta)*cos(phi+M_PI/2));
        Partio::Vec3 eye=lookAt+view;
//        Vec3 up(0,1,0);
        gluLookAt(eye.x,eye.y,eye.z,lookAt.x,lookAt.y,lookAt.z,up.x,up.y,up.z);
        //std::cout<<"eye "<<eye<<std::endl;
        //std::cout<<"look "<<lookAt<<std::endl;
    }

private:
    bool tumble,pan,zoom;
    int x,y;

public:
    void startTumble(const int x,const int y)
    {this->x=x;this->y=y;tumble=true;}

    void startPan(const int x,const int y)
    {this->x=x;this->y=y;pan=true;}

    void startZoom(const int x,const int y)
    {this->x=x;this->y=y;zoom=true;}

    void update(const int x,const int y)
    {
        if(tumble){
            theta+=-(x-this->x)*M_PI/180.;
            phi+=(y-this->y)*M_PI/180.;
        }else if(pan){
            Partio::Vec3 view=Partio::Vec3(sin(theta)*cos(phi),sin(phi),cos(theta)*cos(phi));
            Partio::Vec3 up=Partio::Vec3(sin(theta)*cos(phi+M_PI/2),sin(phi+M_PI/2),cos(theta)*cos(phi+M_PI/2));
            Partio::Vec3 right=view.normalized().cross(up.normalized()).normalized();
            lookAt+=right*distance*.001*(x-this->x);
            lookAt+=up*distance*.001*(y-this->y);
        }else if(zoom){
            int move=y-this->y;
            distance*=exp(move*.01);
        }
        this->x=x;this->y=y;
    }

    void stop()
    {tumble=pan=zoom=false;}

    void fit(const float fov,const Partio::Vec3& boxmin,const Partio::Vec3& boxmax)
    {
        lookAt=.5*(boxmin+boxmax);
        Partio::Vec3 edges=boxmax-boxmin;
        float half_extent=.5*std::max(edges.x,std::max(edges.y,edges.z));
        distance=std::max(1e-3,half_extent/tan(.5*fov*M_PI/180.));
    }
};

#endif
