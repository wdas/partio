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
#define _USE_MATH_DEFINES
#include <cmath>

#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "endian.h"
#include "ZIP.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cfloat>
#include <memory>
#include <sstream>

namespace Partio{

static const int ptc_magic=((((('c'<<8)|'t')<<8)|'p')<<8)|'p';


inline std::string GetString(std::istream& input,const char terminator='\0')
{
    // TODO: make this check for FEOF condition! and also more efficient
    char c=' ';
    std::string s;
    while (c!=terminator && input)
    {
        input.read(&c,sizeof(char));
	s += c;
    }
    return s;
}

bool ParseSpec(const std::string& spec,std::string& typeName,std::string& name)
{
    // TODO: make more robust
    const char* s=spec.c_str();
    typeName="";name="";
    while(*s!=' ') typeName+=*s++;
    while(*s==' ') s++;
    while(*s!='\n') name+=*s++;
    return true;
}

ParticlesDataMutable* readPTC(const char* filename,const bool headersOnly,char** attributes, int percentage)
{
    std::auto_ptr<std::istream> input(Gzip_In(filename,std::ios::in|std::ios::binary));
    if(!*input){
        std::cerr<<"Partio: Unable to open file "<<filename<<std::endl;
        return 0;
    }

    int magic;
    read<LITEND>(*input,magic);
    if(ptc_magic!=magic){
        std::cerr<<"Partio: Magic number '"<<magic<<"' of '"<<filename<<"' doesn't match pptc magic '"<<ptc_magic<<"'"<<std::endl;
        return 0;
    }

    int version;
    read<LITEND>(*input,version);
    if(version>2){
        std::cerr<<"Partio: ptc reader only supports version 2 or less"<<std::endl;
        return 0;
    }

    double nPoints;
    read<LITEND>(*input,nPoints);

    // TODO: allow access to this in the headers only mode for times when only bbox is necessary
    float xmin,ymin,zmin,xmax,ymax,zmax;
    read<LITEND>(*input,xmin,ymin,zmin,xmax,ymax,zmax);

    float dummy;
    // Who knows what this is?
    if (version>=1) for (int d=0;d<4;d++) read<LITEND>(*input,dummy);
    // This seems to be 4 bytes of something and 32 0xDEADBEEF guys
    if (version>=2){
        float maxRadius;
        read<LITEND>(*input,maxRadius);
        for (int d=0;d<32;d++) read<LITEND>(*input,dummy);
    }

    // world-to-eye
    for(int i=0;i<16;i++) read<LITEND>(*input,dummy);
    // eye-to-screen
    for(int i=0;i<16;i++) read<LITEND>(*input,dummy);

    float imgWidth,imgHeight,imgDepth;
    read<LITEND>(*input,imgWidth,imgHeight,imgDepth);

    int nVars,dataSize;
    read<LITEND>(*input,nVars,dataSize);

    // Allocate a simple particle with the appropriate number of points
    ParticlesDataMutable* simple=0;
    if(headersOnly) simple=new ParticleHeaders;
    else simple=create();
    simple->addParticles((int)nPoints);

    // PTC files always have something for these items, so allocate the data
    std::vector<ParticleAttribute> attrHandles;
    ParticleAttribute positionHandle=simple->addAttribute("position",VECTOR,3);
    ParticleAttribute normalHandle=simple->addAttribute("normal",VECTOR,3);
    ParticleAttribute radiusHandle=simple->addAttribute("radius",FLOAT,1);
    std::string typeName,name;

    // data types are "float", "point", "vector", "normal", "color", or "matrix"
    int parsedSize=0;
    for(int chanNum=0;chanNum<nVars;chanNum++){
        ParseSpec(GetString(*input,'\n'),typeName,name);

        int dataSize=0;
        ParticleAttributeType dataType;
        if(typeName=="normal" || typeName=="vector" || typeName=="point"){
            dataType=VECTOR;
            dataSize=3;
        }else if(typeName=="color"){
            dataType=FLOAT;
            dataSize=3;
        }else if(typeName=="matrix"){
            dataType=FLOAT;
            dataSize=16;
        }else if(typeName=="float"){
            dataType=FLOAT;
            dataSize=1;
        }else{
            std::cerr<<"Partio: "<<filename<<" had unknown attribute spec "<<typeName<<" "<<name<<std::endl;
            simple->release();
            return 0;
        }
        
        // make unqiue name
        int unique=1;
        std::string effectiveName=name;
        ParticleAttribute info;
        while(simple->attributeInfo(effectiveName.c_str(),info)){
            std::ostringstream ss;
            ss<<name<<unique++;
            effectiveName=ss.str();
        }

        attrHandles.push_back(simple->addAttribute(effectiveName.c_str(),dataType,dataSize));
        parsedSize+=dataSize;
    }
    if(dataSize!=parsedSize){
        std::cerr<<"Partio: error with PTC, computed dataSize ("<<dataSize
                 <<") different from read one ("<<parsedSize<<")"<<std::endl;
        simple->release();
        return 0;
    }

    // If all we care about is headers, then return.  
    if(headersOnly){
        return simple;
    }

    // more weird input attributes
    if(version>=1) for(int i=0;i<2;i++) read<LITEND>(*input,dummy);

    for(int pointIndex=0;pointIndex<nPoints;pointIndex++){
        float* pos=simple->dataWrite<float>(positionHandle,pointIndex);
        read<LITEND>(*input,pos[0],pos[1],pos[2]);

        unsigned short phi,z; // normal encoded
        read<LITEND>(*input,phi,z);
        float* norm=simple->dataWrite<float>(normalHandle,pointIndex);

	// Convert unsigned short (phi,nz) to xyz normal
        // This packing code is based on Per Christensen's rman forum post
	if (phi != 65535 || z != 65535) {
	     float nz = (float)z / 65535.0f;
	     norm[2] = 2.0f * nz - 1.0f;
	     float fphi = (float)phi / 65535.0f;
	     fphi = 2.0 * M_PI * (fphi - 0.5);
	     //assert(-M_PI-0.0001 <= fphi && fphi <= M_PI+0.0001);
	     double rxy = sqrt(1.0 - norm[2]*norm[2]);
	     norm[0] = rxy * sin(fphi);
	     norm[1] = rxy * cos(fphi);
	} else {
	     norm[0] = norm[1] = norm[2] = 0.0f;
	}        

        float* radius=simple->dataWrite<float>(radiusHandle,pointIndex);
        read<LITEND>(*input,radius[0]);


        for(unsigned int i=0;i<attrHandles.size();i++){
            float* data=simple->dataWrite<float>(attrHandles[i],pointIndex);
            for(int j=0;j<attrHandles[i].count;j++)
                read<LITEND>(*input,data[j]);
        }

    }

    return simple;
}

bool writePTC(const char* filename,const ParticlesData& p,const bool compressed)
{
    //std::ofstream output(filename,std::ios::out|std::ios::binary);

    std::auto_ptr<std::ostream> output(
        compressed ? 
        Gzip_Out(filename,std::ios::out|std::ios::binary)
        :new std::ofstream(filename,std::ios::out|std::ios::binary));

    if(!*output){
        std::cerr<<"Partio Unable to open file "<<filename<<std::endl;
        return false;
    }

    // magic & version
    write<LITEND>(*output,ptc_magic);
    write<LITEND>(*output,(int)0);

    // particle count
    double numParticlesAsDouble=p.numParticles();
    write<LITEND>(*output,numParticlesAsDouble);

    ParticleAttribute positionHandle,normalHandle,radiusHandle;
    bool foundPosition=p.attributeInfo("position",positionHandle);
    bool foundNormal=p.attributeInfo("normal",normalHandle);
    bool foundRadius=p.attributeInfo("radius",radiusHandle);

    if(!foundPosition){
        std::cerr<<"Partio: failed to find attr 'position' for PTC output"<<std::endl;
        return false;
    }
    if(!foundNormal) std::cerr<<"Partio: failed to find attr 'normal' for PTC output, using 0,0,0"<<std::endl;
    if(!foundRadius) std::cerr<<"Partio: failed to find attr 'radius' for PTC output, using 1"<<std::endl;

    // compute bounding box
    float boxmin[3]={FLT_MAX,FLT_MAX,FLT_MAX},boxmax[3]={-FLT_MAX,-FLT_MAX,-FLT_MAX};
    for(int i=0;i<p.numParticles();i++){
        const float* pos=p.data<float>(positionHandle,i);
        for(int k=0;k<3;k++){
            boxmin[k]=std::min(pos[k],boxmin[k]);
            boxmax[k]=std::max(pos[k],boxmax[k]);
        }
    }
    write<LITEND>(*output,boxmin[0],boxmin[1],boxmin[2]);
    write<LITEND>(*output,boxmax[0],boxmax[1],boxmax[2]);

    // world-to-eye & 
    for(int i=0;i<4;i++) for(int j=0;j<4;j++){
            if(i==j) write<LITEND>(*output,(float)1);
            else write<LITEND>(*output,(float)0);
    }
    // eye-to-screen
    const float foo[4][4]={{1.8,0,0,0}, {0,2.41,0,0}, {0,0,1,1}, {0,0,-.1,0}};
    for(int i=0;i<4;i++) for(int j=0;j<4;j++){
            write<LITEND>(*output,foo[i][j]);
    }
    

    // imgwidth imgheight, imgdepth
    write<LITEND>(*output,(float)640,(float)480,(float)300);

    int nVars=0,dataSize=0;
    std::vector<std::string> specs;
    std::vector<ParticleAttribute> attrs;
    for(int i=0;i<p.numAttributes();i++){
        ParticleAttribute attr;
        p.attributeInfo(i,attr);
        if(attr.name!="position" && attr.name!="radius" && attr.name!="normal"){
            if(attr.count==3 && attr.type!=INT){
                attrs.push_back(attr);
                specs.push_back("color "+attr.name+"\n");
                dataSize+=attr.count;
                nVars++;
            }else if(attr.count==1 && attr.type!=INT){
                attrs.push_back(attr);
                dataSize+=attr.count;
                nVars++;
                specs.push_back("float "+attr.name+"\n");
            }else if(attr.count==16 && attr.type!=INT){
                nVars++;
                attrs.push_back(attr);
                dataSize+=attr.count;
                specs.push_back("matrix "+attr.name+"\n");
            }else{
                std::cerr<<"Partio: Unable to write data type "<<TypeName(attr.type)<<"["<<attr.count<<"] to a ptc file"<<std::endl;
            }
        }
    }
    //std::cerr<<"writing dataSize"<<dataSize<<std::endl;
    write<LITEND>(*output,nVars,dataSize);

    for(unsigned int i=0;i<specs.size();i++){
        output->write(specs[i].c_str(),specs[i].length());
    }

    for(int pointIndex=0;pointIndex<p.numParticles();pointIndex++){
        // write position
        const float* pos=p.data<float>(positionHandle,pointIndex);
        write<LITEND>(*output,pos[0],pos[1],pos[2]);
        // write normal
        static const float static_n[3]={0,0,0};
        const float* n=static_n;
        if(foundNormal)
            n=p.data<float>(normalHandle,pointIndex);
	int phi, z;
	if (n[0] == 0 && n[1] == 0 && n[2] == 0) phi = z = 65535;
	else {
            float mag_squared=n[0]*n[0]+n[1]*n[1]+n[2]*n[2];
	    phi = int((atan2(n[0], n[1]) * (1/(2*M_PI)) + 0.5) * 65535 + 0.5);
	    z = std::min(int((n[2]/sqrt(mag_squared)+1)/2 * 65535 + 0.5), 65535);
	}
        write<LITEND>(*output,(unsigned short)phi,(unsigned short)z);
        // write radius
        static const float static_r[1]={1.f};
        const float* radius=static_r;
        if(foundRadius)
            radius=p.data<float>(radiusHandle,pointIndex);
        write<LITEND>(*output,radius[0]);
        // write other attributes
        for(unsigned int i=0;i<specs.size();i++){
            const float* data=p.data<float>(attrs[i],pointIndex);
            for(int k=0;k<attrs[i].count;k++){
                write<LITEND>(*output,data[k]);
            }
        }
        
    }
    return true;
}

}
