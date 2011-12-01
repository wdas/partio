/*
PARTIO SOFTWARE
Copyright (c) 2011 Disney Enterprises, Inc. and Contributors,  All rights reserved

Format Contributed by github user: redpawfx (redpawFX@gmail.com)  and Luma Pictures  2011
Some code for this format  was helped along  by referring to an implementation by

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
#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "ZIP.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>
#include <memory>

namespace Partio
{

using namespace std;

// TODO: convert this to use iterators like the rest of the readers/writers

ParticlesDataMutable* readPTS(const char* filename,const bool headersOnly,char** attributes, int percentage)
{
    auto_ptr<istream> input(Gzip_In(filename,ios::in|ios::binary));
    if(!*input)
	{
        cerr<<"Partio: Can't open particle data file: "<<filename<<endl;
        return 0;
    }

    ParticlesDataMutable* simple=0;
    if(headersOnly) simple=new ParticleHeaders;
    else simple=create();

    // read NPoints and NPointAttrib
    string word;

    vector<string> attrNames;
    vector<ParticleAttribute> attrs;

	/// FORMAT  is  7 elements  space delimited { posX posY posZ  magic?  red green blue }
	// since there's no header data to parse we're just going to hard code this

    attrNames.push_back((string)"position");
	attrNames.push_back((string)"magic");
	attrNames.push_back((string)"pointColor");

    attrs.push_back(simple->addAttribute(attrNames[0].c_str(),Partio::VECTOR,3));
    attrs.push_back(simple->addAttribute(attrNames[1].c_str(),Partio::INT,1));
	attrs.push_back(simple->addAttribute(attrNames[2].c_str(),Partio::VECTOR,3));


    unsigned int num=0;
    simple->addParticles(num);
    if(headersOnly) return simple; // escape before we try to touch data

    if(input->good())
	{ // garbage count at top of  file
        *input>>word;
    }

	uint index = 0;
    // Read actual particle data
    if(!input->good()){simple->release();return 0;}

	// we have to read line by line, because data is not  clean and consistent so we skip any lines that dont' conform

    for(unsigned int particleIndex=0;input->good();)
	{
		string token = "";
		char line[100];
		input->getline(line, 100);

		stringstream ss(line);

		float lineData[7];
		int i = 0;

		while (ss >> token)
		{
			stringstream foo(token);
			float x;
			foo >> x;
			lineData[i] = x;
			i++;
		}

		if (i == 7)
		{
			simple->addParticle();
			for(unsigned int attrIndex=0;attrIndex<attrs.size();attrIndex++)
			{
				if(attrs[attrIndex].type==Partio::INT)
				{
					int* data=simple->dataWrite<int>(attrs[attrIndex],particleIndex);
						data[0]=particleIndex;
				}
				else if(attrs[attrIndex].type==Partio::FLOAT || attrs[attrIndex].type==Partio::VECTOR)
				{
					float* data=simple->dataWrite<float>(attrs[attrIndex],particleIndex);

					if(attrs[attrIndex].name == "pointColor")
					{
						// 8 bit color  conversion
						data[0]=lineData[4]/255;
						data[1]=lineData[5]/255;
						data[2]=lineData[6]/255;
					}
					else
					{
						// position, we flip y/z
						data[0]=lineData[0];
						data[1]=lineData[2];
						data[2]=lineData[1];
					}
				}
			}
			particleIndex++;
		}
    }
    return simple;
}

bool writePTS(const char* filename,const ParticlesData& p,const bool compressed)
{
    auto_ptr<ostream> output(
        compressed ?
        Gzip_Out(filename,ios::out|ios::binary)
        :new ofstream(filename,ios::out|ios::binary));

    *output<<"ATTRIBUTES"<<endl;

    vector<ParticleAttribute> attrs;
    for (int aIndex=0;aIndex<p.numAttributes();aIndex++){
        attrs.push_back(ParticleAttribute());
        p.attributeInfo(aIndex,attrs[aIndex]);
        *output<<" "<<attrs[aIndex].name;
    }
    *output<<endl;

    // TODO: assert right count
    *output<<"TYPES"<<endl;
    for (int aIndex=0;aIndex<p.numAttributes();aIndex++){
        switch(attrs[aIndex].type){
            case FLOAT: *output<<" R";break;
            case VECTOR: *output<<" V";break;
            case INDEXEDSTR:
            case INT: *output<<" I";break;
            case NONE: assert(false); break; // TODO: more graceful
        }
    }
    *output<<endl;

    *output<<"NUMBER_OF_PARTICLES: "<<p.numParticles()<<endl;
    *output<<"BEGIN DATA"<<endl;

    for(int particleIndex=0;particleIndex<p.numParticles();particleIndex++){
        for(unsigned int attrIndex=0;attrIndex<attrs.size();attrIndex++){
            if(attrs[attrIndex].type==Partio::INT || attrs[attrIndex].type==Partio::INDEXEDSTR){
                const int* data=p.data<int>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++)
                    *output<<data[count]<<" ";
            }else if(attrs[attrIndex].type==Partio::FLOAT || attrs[attrIndex].type==Partio::VECTOR){
                const float* data=p.data<float>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++)
                    *output<<data[count]<<" ";
            }
        }
        *output<<endl;
    }
    return true;

}

}
