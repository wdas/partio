/*
 * <b>CONFIDENTIAL INFORMATION: This software is the confidential and
 * proprietary information of Walt Disney Animation Studios ("Disney").
 * This software is owned by Disney and may not be used, disclosed,
 * reproduced or distributed for any purpose without prior written
 * authorization and license from Disney. Reproduction of any section of
 * this software must include this legend and all copyright notices.
 * (c) Disney. All rights reserved.</b>
 *
 */
#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "ZIP.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <memory>

namespace Partio{

// TODO: convert this to use iterators like the rest of the readers/writers

ParticlesDataMutable* readPDA(const char* filename,const bool headersOnly)
{
    std::auto_ptr<std::istream> input(Gzip_In(filename,std::ios::in|std::ios::binary));
    if(!*input){
        std::cerr<<"Partio: Can't open particle data file: "<<filename<<std::endl;
        return 0;
    }

    ParticlesDataMutable* simple=0;
    if(headersOnly) simple=new ParticleHeaders;
    else simple=create();

    // read NPoints and NPointAttrib
    std::string word;
    
    if(input->good()){
        *input>>word;
        if(word!="ATTRIBUTES"){simple->release();return 0;}
    }

    std::vector<std::string> attrNames;
    std::vector<ParticleAttribute> attrs;

    while(input->good()){
        *input>>word;
        if(word=="TYPES") break;;
        attrNames.push_back(word);
    }

    size_t index=0;
    while(input->good()){
        *input>>word;
        if(word=="NUMBER_OF_PARTICLES:") break;

        if(index>=attrNames.size()) continue;

        if(word=="V"){
            attrs.push_back(simple->addAttribute(attrNames[index].c_str(),Partio::VECTOR,3));
        }else if("R"){
            attrs.push_back(simple->addAttribute(attrNames[index].c_str(),Partio::FLOAT,1));
        }else if("I"){
            attrs.push_back(simple->addAttribute(attrNames[index].c_str(),Partio::INT,1));
        }

        index++;
    }

    unsigned int num=0;
    if(input->good()){
        *input>>num;
        simple->addParticles(num);
        if(headersOnly) return simple; // escape before we try to touch data
    }else{
        simple->release();
        return 0;
    }
    
    // look for beginning of header
    if(input->good()){
        *input>>word;
        if(word != "BEGIN"){simple->release();return 0;}
    }
    if(input->good()){
        *input>>word;
        if(word != "DATA"){simple->release();return 0;}
    }

    // Read actual particle data
    if(!input->good()){simple->release();return 0;}
    for(unsigned int particleIndex=0;input->good() && particleIndex<num; particleIndex++) {
        for(unsigned int attrIndex=0;attrIndex<attrs.size();attrIndex++){
            if(attrs[attrIndex].type==Partio::INT){
                int* data=simple->dataWrite<int>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++){
                    int ival;
                    *input>>ival;
                    data[count]=ival;
                }
            }else if(attrs[attrIndex].type==Partio::FLOAT || attrs[attrIndex].type==Partio::VECTOR){
                float* data=simple->dataWrite<float>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++){
                    float fval;
                    *input>>fval;
                    data[count]=fval;
                }
            }
        }
    }
    
    return simple;
}

bool writePDA(const char* filename,const ParticlesData& p,const bool compressed)
{
    std::auto_ptr<std::ostream> output(
        compressed ? 
        Gzip_Out(filename,std::ios::out|std::ios::binary)
        :new std::ofstream(filename,std::ios::out|std::ios::binary));

    *output<<"ATTRIBUTES"<<std::endl;

    std::vector<ParticleAttribute> attrs;
    for (int aIndex=0;aIndex<p.numAttributes();aIndex++){
        attrs.push_back(ParticleAttribute());
        p.attributeInfo(aIndex,attrs[aIndex]);
        *output<<" "<<attrs[aIndex].name;
    }
    *output<<std::endl;

    // TODO: assert right count
    *output<<"TYPES"<<std::endl;
    for (int aIndex=0;aIndex<p.numAttributes();aIndex++){
        switch(attrs[aIndex].type){
            case FLOAT: *output<<" R";break;
            case VECTOR: *output<<" V";break;
            case INT: *output<<" I";break;
            case NONE: assert(false); break; // TODO: more graceful
        }
    }
    *output<<std::endl;

    *output<<"NUMBER_OF_PARTICLES: "<<p.numParticles()<<std::endl;
    *output<<"BEGIN DATA"<<std::endl;

    for(int particleIndex=0;particleIndex<p.numParticles();particleIndex++){
        for(unsigned int attrIndex=0;attrIndex<attrs.size();attrIndex++){
            if(attrs[attrIndex].type==Partio::INT){
                const int* data=p.data<int>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++)
                    *output<<data[count]<<" ";
            }else if(attrs[attrIndex].type==Partio::FLOAT || attrs[attrIndex].type==Partio::VECTOR){
                const float* data=p.data<float>(attrs[attrIndex],particleIndex);
                for(int count=0;count<attrs[attrIndex].count;count++)
                    *output<<data[count]<<" ";
            }
        }
        *output<<std::endl;
    }
    return true;

}

}
