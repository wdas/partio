/*
PARTIO SOFTWARE
Copyright 2010-2011 Disney Enterprises, Inc. All rights reserved

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

/*
Support for .icecache format added by Mad Crew (www.madcrew.se)
Author: Fredrik Brannbacka
Contact: fredrik(a)madcrew.se
*/

#include "../Partio.h"
#include "../core/ParticleHeaders.h"
//#include "PartioEndian.h"
#include "ZIP.h"

#include "ICECACHE.h"
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>

namespace Partio {

using namespace std;

//
template<ParticleAttributeType ETYPE>
void readAttrData(std::istream& input, ParticlesDataMutable& particles, icecache_attribute& ice_attr)
{
    ParticleAccessor accessor(ice_attr.attr_handle); // Create a accessor for the attribute
    ParticlesDataMutable::iterator iterator=particles.begin();
    iterator.addAccessor(accessor);
    typedef typename ETYPE_TO_TYPE<ETYPE>::TYPE TYPE;
    int index = 0;
    ULONG isConstant = 0;
    TYPE val[ice_attr.attr_handle.count];
    // Iterate over all points
    for (ParticlesDataMutable::iterator end=particles.end();iterator!=end;++iterator) {
        // Read the constant attribute every 4000th element
        if (index%4000==0 && ice_attr.name!="PointPosition") {
            isConstant = read<ULONG>(input);
            if (isConstant&&ice_attr.structtype==2) {
                val[0] = read<TYPE>(input);
                break;
            }
        }
        TYPE* data=accessor.raw<TYPE>(iterator);
        for (int i=0;i<ice_attr.attr_handle.count;i++) {
            if (index==0&&ice_attr.structtype!=2) // Make sure we read first elements values
                val[i] = read<TYPE>(input);
            if (index&&!isConstant) // Only keep on reading if not Constant
                val[i] = read<TYPE>(input);
            data[i] = val[i];
        }
        index++;
    }
}

template<class T>
void writeAttrData(std::ostream& output, const ParticlesData& particles, icecache_attribute& ice_attr)
{
    ParticleAccessor accessor(ice_attr.attr_handle); // Create a accessor for the attribute
    ParticlesData::const_iterator iterator=particles.begin();
    iterator.addAccessor(accessor);
    int index = 0;
    T val[ice_attr.attr_handle.count];
    // Iterate over all points
    for (ParticlesData::const_iterator end=particles.end();iterator!=end;++iterator) {
        // Write the constant attribute every 4000th element except for PointPosition
        if (index%4000==0&&ice_attr.name!="PointPosition") {
            write<ULONG>(output, (ULONG)0);
        }

        T* data=accessor.raw<T>(iterator);
        for (int i=0;i<ice_attr.attr_handle.count;i++) {
            write<T>(output, data[i]);
        }
        index++;
    }
    return;
}

ParticlesDataMutable* readICECACHE(const char* filename,const bool headersOnly)
{
    std::auto_ptr<std::istream> input(Gzip_In(filename,std::ios::in|std::ios::binary));
    if (!*input) {
        std::cerr<<"Partio: Unable to open file "<<filename<<std::endl;
        return 0;
    }

    // Make sure it's actually an icecache file.
    if (!isIcecache(*input)) {
        std::cerr << "Partio: Not an icecache file." << std::endl;
        return 0;
    }


    // Header values
    ULONG versionnumber     = read<ULONG>(*input);
    ULONG objecttype        = read<ULONG>(*input);
    ULONG pointcount        = read<ULONG>(*input);
    ULONG edgecount         = read<ULONG>(*input);
    ULONG polygoncount      = read<ULONG>(*input);
    ULONG samplecount       = read<ULONG>(*input);
    ULONG substep           = read<ULONG>(*input);
    ULONG userdatablobcount = read<ULONG>(*input);
    ULONG attributecount    = read<ULONG>(*input);

    // Make sure the the cache is exactly version 103
    if (versionnumber!=103) {
        std::cerr<<"Partio: ICECACHE must be version 103" << versionnumber << std::endl;
        return 0;
    }

    // Allocate a simple particle with the appropriate number of points
    ParticlesDataMutable* simple=0;
    if (headersOnly) simple=new ParticleHeaders;
    else simple=create();

    simple->addParticles(pointcount);

    std::vector<icecache_attribute> ice_attrs;
    std::vector<ParticleAttribute> attrs;

    // Loop over the attributes in the
    for (ULONG attribute_index = 0; attribute_index < attributecount; attribute_index++) {
        icecache_attribute ice_attr;
        readAttribute(*input, ice_attr, *simple);
        if (ice_attr.name == "PointPosition") {
            ParticleAttribute partio_attr=simple->addAttribute("position",VECTOR,3);
            ice_attr.attr_handle = partio_attr;
        } else {
            ParticleAttribute partio_attr=simple->addAttribute(ice_attr.name.c_str(),ice_attr.type,ice_attr.attribute_length);
            ice_attr.attr_handle = partio_attr;
        }
        ice_attrs.push_back(ice_attr);
    }

    std::vector<icecache_attribute>::const_iterator attribute_it;
    for ( attribute_it=ice_attrs.begin() ; attribute_it < ice_attrs.end(); attribute_it++ ) {
        icecache_attribute ice_attr = (*attribute_it);
        switch (ice_attr.type) {
        case INT:
            readAttrData<INT>(*input, *simple, ice_attr);
            break;
        case FLOAT:
            readAttrData<FLOAT>(*input, *simple, ice_attr);
            break;
        case VECTOR:
            readAttrData<VECTOR>(*input, *simple, ice_attr);
            break;
        }
    }
    return simple;
}

bool writeICECACHE(const char* filename,const ParticlesData& p,const bool compressed)
{
    std::auto_ptr<std::ostream> output(
        compressed ?
        Gzip_Out(filename,std::ios::out|std::ios::binary)
        :new std::ofstream(filename,std::ios::out|std::ios::binary));

    if (!*output) {
        std::cerr<<"Partio Unable to open file "<<filename<<std::endl;
        return false;
    }

    char icecache_magic[8] = {'I','C','E','C','A','C','H','E'};
    output->write(icecache_magic, 8);

    std::vector<std::string> attribute_keys;
    std::map<std::string, icecache_attribute> attributes;
    for (int i=0;i<p.numAttributes();i++) {
        ParticleAttribute attr;
        p.attributeInfo(i,attr);
        if (attr.name=="position")
            attr.name = "PointPosition";
        if (attr.name[0]=='_')
            continue;
        attributes[attr.name] = createAttribute(attr);
        attribute_keys.push_back(attr.name);
    }
    std::sort(attribute_keys.begin(),attribute_keys.end());

    // header values
    write<ULONG>(*output, (ULONG)100);                 // versionnumber
    write<ULONG>(*output, (ULONG)0);                   // objecttype
    write<ULONG>(*output, (ULONG)p.numParticles());    // pointcount
    write<ULONG>(*output, (ULONG)0);                   // edgecount
    write<ULONG>(*output, (ULONG)0);                   // polygoncount
    write<ULONG>(*output, (ULONG)0);                   // samplecount
    write<ULONG>(*output, (ULONG)attribute_keys.size());// attributecount

    std::vector<std::string>::const_iterator attribute_it;
    for ( attribute_it=attribute_keys.begin() ; attribute_it < attribute_keys.end(); attribute_it++ )
    {
        writeName(*output, attributes[*attribute_it].name);
        write<ULONG>(*output, attributes[*attribute_it].datatype);
        write<ULONG>(*output, attributes[*attribute_it].structtype);
        write<ULONG>(*output, attributes[*attribute_it].contexttype);
        write<ULONG>(*output, attributes[*attribute_it].objid);
        write<ULONG>(*output, attributes[*attribute_it].category);
    }

    for ( attribute_it=attribute_keys.begin() ; attribute_it < attribute_keys.end(); attribute_it++ )
    {
        switch (attributes[*attribute_it].attr_handle.type) {
        case NONE:
            assert(false);
            break;
        case FLOAT:
            writeAttrData<ETYPE_TO_TYPE<FLOAT>::TYPE>(*output,p,attributes[*attribute_it]);
            break;
        case INT:
            writeAttrData<ETYPE_TO_TYPE<INT>::TYPE>(*output,p,attributes[*attribute_it]);
            break;
        case VECTOR:
            writeAttrData<ETYPE_TO_TYPE<VECTOR>::TYPE>(*output,p,attributes[*attribute_it]);
            break;
        case INDEXEDSTR:
            writeAttrData<ETYPE_TO_TYPE<INDEXEDSTR>::TYPE>(*output,p,attributes[*attribute_it]);
            break;
        }
    }

}
}
