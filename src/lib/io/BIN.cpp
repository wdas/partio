#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "endian.h"
#include "ZIP.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

namespace Partio{

using namespace std;

static const long BIN_MAGIC = 100;//OxFABADA;

typedef struct{
    long verificationCode;
    char fluidName[250];
    short version;
    float scaleScene;
    int fluidType;
    float elapsedSimulationTime;
    int frameNumber;
    int framePerSecond;
    long numberOfParticles;
    float radius;
    float pressure[3];
    float speed[3];
    float temperature[3];
    float emitterPosition[3];
    float emitterRotation[3];
    float emitterScale[3];
} BIN_HEADER;

ParticlesDataMutable* readBIN(const char* filename, const bool headersOnly, char** attributes, int percentage){

    auto_ptr<istream> input(Gzip_In(filename,std::ios::in|std::ios::binary));
    if(!*input){
        std::cerr << "Partio: Unable to open file " << filename << std::endl;
        return 0;
    }

    BIN_HEADER header;
    input->read((char*)&header, sizeof(header)); 
    if(BIN_MAGIC != header.verificationCode){
        std::cerr << "Partio: Magic number '" << header.verificationCode << "' of '" << filename << "' doesn't match pdc magic '" << BIN_MAGIC << "'" << std::endl;
        return 0;
    }

    BIGEND::swap(header.numberOfParticles);
    //BIGEND::swap(header.numAttrs);

    ParticlesDataMutable* simple = headersOnly ? new ParticleHeaders: create();
/*    simple->addParticles(header.numParticles);

    for(int attrIndex = 0; attrIndex < header.numAttrs; attrIndex++){
        // add attribute
        ParticleAttribute attr;
        string attrName = readName(*input);
        int type;
        read<BIGEND>(*input, type);     
        if(type == 3){
            attr = simple->addAttribute(attrName.c_str(), FLOAT, 1);
        }
        else if(type == 5){
            attr = simple->addAttribute(attrName.c_str(), VECTOR, 3);
        }

        // if headersOnly, skip 
        if(headersOnly){
            input->seekg((int)input->tellg() + header.numParticles*sizeof(double)*attr.count);
            continue;
        }
        else{
            double tmp[3];
            for(int partIndex = 0; partIndex < simple->numParticles(); partIndex++){
                for(int dim = 0; dim < attr.count; dim++){
                    read<BIGEND>(*input, tmp[dim]);
                    simple->dataWrite<float>(attr, partIndex)[dim] = (float)tmp[dim];
                }
            }
        }
    }
*/
    return simple;
}

bool writeBIN(const char* filename,const ParticlesData& p,const bool /*compressed*/)
{
    return true; 
/*
    std::auto_ptr<std::ostream> output(
    new std::ofstream(filename,std::ios::out|std::ios::binary));
    
    if (!*output) {
        std::cerr<<"Partio Unable to open file "<<filename<<std::endl;
        return false;
    }


    FileHeadder header;
    memcpy(header.magic, BIN_MAGIC, sizeof(BIN_MAGIC));
    
    // write .pdc header
    write<LITEND>(*output, PDC_MAGIC);
    write<BIGEND>(*output, (int)1); // version
    write<BIGEND>(*output, (int)1); // bitorder
    write<BIGEND>(*output, (int)0); // tmp1
    write<BIGEND>(*output, (int)0); // tmp2
    write<BIGEND>(*output, (int)p.numParticles());
    write<BIGEND>(*output, (int)p.numAttributes());

    for(int attrIndex = 0; attrIndex < p.numAttributes(); attrIndex++){
        ParticleAttribute attr;
        p.attributeInfo(attrIndex,attr);

        // write attribute name
        write<BIGEND>(*output, (int)attr.name.length());
        output->write(attr.name.c_str(), (int)attr.name.length());

        // write type
        int count = 1; // FLOAT
        if(attr.type == VECTOR){
            count = 3;
        }
        write<BIGEND>(*output, (int)(count+2));

        // write data
        for(int partIndex = 0; partIndex < p.numParticles(); partIndex++){
            const float* data = p.data<float>(attr, partIndex);
            for(int dim = 0; dim < count; dim++){
                write<BIGEND>(*output, (double)data[dim]);
            }
        }
    }
    return true;
*/
}

}// end of namespace Partio
