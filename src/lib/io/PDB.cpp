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

This code is partially based on the Gifts/readpdb directory of Autodesk Maya
*/

#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "pdb.h"
#include "endian.h"
#include "ZIP.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <memory>
#include <string.h>
namespace Partio{

std::string GetString(std::istream& input,bool& error)
{
    //std::cerr<<"enter"<<std::endl;
    const char terminator='\0';
    // TODO: make this check for FEOF condition! and also more efficient
    char c=' ';
    std::string s="";
    error=true;
    while (input)
    {
        input.read(&c,sizeof(char));
        if(c == terminator){error=false;break;}
        s += c;
    }
    return s;
}

ParticlesDataMutable* readPDB(const char* filename,const bool headersOnly)
{
    std::auto_ptr<std::istream> input(Gzip_In(filename,std::ios::in|std::ios::binary));
    if(!*input){
        std::cerr<<"Partio: Unable to open file "<<filename<<std::endl;
        return 0;
    }

    // Use simple particle since we don't have optimized storage.
    ParticlesDataMutable* simple=0;
    if(headersOnly) simple=new ParticleHeaders;
    else simple=create();

    // Read header and add as many particles as found
    PDB_Header32 header32;

    input->read((char*)&header32,sizeof(PDB_Header32));
    if(header32.magic != PDB_MAGIC){
        std::cerr<<"Partio: failed to get PDB magic"<<std::endl;
        return 0;
    }

    simple->addParticles(header32.data_size);
    
    for(unsigned int i=0;i<header32.num_data;i++){
        Channel_io_Header channelIOHeader;
        input->read((char*)&channelIOHeader,sizeof(channelIOHeader));
        Channel32 channelHeader;
        input->read((char*)&channelHeader,sizeof(channelHeader));
        bool error;
        std::string name=GetString(*input,error);
        if(error){
            simple->release();
            return 0;
        }

        Channel_Data32 channelData;
        input->read((char*)&channelData,sizeof(channelData));

        
        ParticleAttributeType type;
        switch(channelHeader.type){
            case PDB_VECTOR: type=VECTOR;break;
            case PDB_REAL: type=FLOAT;break;
            case PDB_LONG: type=INT;break;
            default: type=NONE;break;
        }
        int size=header32.data_size*channelData.datasize;

        // Read data or skip if we haven't found appropriate type handle
        if(type==NONE){
            char buf[1024];
            int toSkip=size;
            while(toSkip>0){
                input->read(buf,std::min(toSkip,1024));
                toSkip-=1024;
            }
            std::cerr<<"Partio: Attribute '"<<name<<"' cannot map type"<<std::endl;
        }else{
            int count=channelData.datasize/TypeSize(type);
            ParticleAttribute attrHandle=simple->addAttribute(name.c_str(),type,count);
            if(headersOnly){
                char buf[1024];
                int toSkip=size;
                while(toSkip>0){
                    input->read(buf,std::min(toSkip,1024));
                    toSkip-=1024;
                }
            }else{
                Partio::ParticlesDataMutable::iterator it=simple->begin();
                Partio::ParticleAccessor accessor(attrHandle);
                it.addAccessor(accessor);

                for(Partio::ParticlesDataMutable::iterator end=simple->end();it!=end;++it){
                    input->read(accessor.raw<char>(it),sizeof(float)*attrHandle.count);
                }
            }
        }
    }
    return simple;
}

bool writePDB(const char* filename,const ParticlesData& p,const bool compressed)
{
    std::auto_ptr<std::ostream> output(
        compressed ? 
        Gzip_Out(filename,std::ios::out|std::ios::binary)
        :new std::ofstream(filename,std::ios::out|std::ios::binary));

    if(!*output){
        std::cerr<<"Partio Unable to open file "<<filename<<std::endl;
        return false;
    }

    PDB_Header32 h32;
    memset(&h32,0,sizeof(PDB_Header32));
    h32.magic=PDB_MAGIC;
    h32.swap=1;
    h32.version=1.0;
    h32.time=0.0;
    h32.data_size=p.numParticles();
    h32.num_data=p.numAttributes();
    for(int k=0;k<32;k++) h32.padding[k]=0;
    h32.data=0;
    output->write((char*)&h32,sizeof(PDB_Header32));
    
    for(int attrIndex=0;attrIndex<p.numAttributes();attrIndex++){
        ParticleAttribute attr;
        p.attributeInfo(attrIndex,attr);

        Channel_io_Header cio;
        Channel32 channel32;
        Channel_Data32 data_header32;
        memset(&cio,0,sizeof(Channel_io_Header));
        memset(&channel32,0,sizeof(Channel32));
        memset(&data_header32,0,sizeof(Channel_Data32));

        cio.magic=99;
        cio.swap=1;
        cio.encoding=0;
        cio.type=0;
        output->write((char*)&cio,sizeof(Channel_io_Header));
 
        // TODO: assert cproper count!
        channel32.name=0;
        switch(attr.type){
            case INT:channel32.type=PDB_LONG;break;
            case FLOAT:channel32.type=PDB_REAL;break;
            case VECTOR:channel32.type=PDB_VECTOR;break;
            default: assert(false);
        }
        channel32.size=0;
        channel32.active_start=0;
        channel32.active_end=h32.data_size-1;
        channel32.hide=0;
        channel32.disconnect=0;
        channel32.data=0;
        channel32.link=0;
        channel32.next=0;
        output->write((char*)&channel32,sizeof(channel32));
        output->write(attr.name.c_str(),attr.name.length()*sizeof(char)+1);
        data_header32.type=channel32.type;
        data_header32.datasize=attr.count*sizeof(float);
        data_header32.blocksize=p.numParticles();
        data_header32.num_blocks=1;
        data_header32.block=0;
        output->write((char*)&data_header32,sizeof(data_header32));

        Partio::ParticlesData::const_iterator it=p.begin();
        Partio::ParticleAccessor accessor(attr);
        it.addAccessor(accessor);

        for(Partio::ParticlesData::const_iterator end=p.end();it!=end;++it){
            output->write(accessor.raw<char>(it),sizeof(float)*attr.count);
        }
    }
    return true;
}


}
