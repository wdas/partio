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

#ifndef _icecache_h_
#define _icecache_h_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
namespace Partio {

typedef unsigned int ULONG;
typedef unsigned short WCHAR;

// Attribute Category
enum siICEAttributeCategory
{
    siICEAttributeCategoryUnknown                               = 0,
    siICEAttributeCategoryBuiltin                               = 1,
    siICEAttributeCategoryCustom                                = 2
};
// Node Structure Types
enum siICENodeStructureType
{
    siICENodeStructureSingle                                = 1,
    siICENodeStructureArray                                 = 2,
    siICENodeStructureAny                                   = 3
};

// Node Context Type
enum siICENodeContextType
{
    siICENodeContextSingleton                               = 1,
    siICENodeContextComponent0D                             = 2,
    siICENodeContextComponent1D                             = 4,
    siICENodeContextComponent2D                             = 8,
    siICENodeContextComponent0D2D                           = 16,
    siICENodeContextElementGenerator                        = 32,
    siICENodeContextSingletonOrComponent0D                  = 3,
    siICENodeContextSingletonOrComponent1D                  = 5,
    siICENodeContextSingletonOrComponent2D                  = 9,
    siICENodeContextSingletonOrComponent0D2D                = 17,
    siICENodeContextSingletonOrElementGenerator             = 33,
    siICENodeContextComponent0DOr1DOr2D                     = 14,
    siICENodeContextNotSingleton                            = 62,
    siICENodeContextAny                                     = 63
};

// Node Data Types
enum siICENodeDataType
{
    siICENodeDataBool                                       = 1,
    siICENodeDataLong                                       = 2,
    siICENodeDataFloat                                      = 4,
    siICENodeDataVector2                                    = 8,
    siICENodeDataVector3                                    = 16,
    siICENodeDataVector4                                    = 32,
    siICENodeDataQuaternion                                 = 64,
    siICENodeDataMatrix33                                   = 128,
    siICENodeDataMatrix44                                   = 256,
    siICENodeDataColor4                                     = 512,
    siICENodeDataGeometry                                   = 1024,
    siICENodeDataLocation                                   = 2048,
    siICENodeDataExecute                                    = 4096,
    siICENodeDataReference                                  = 8192,
    siICENodeDataRotation                                   = 16384,
    siICENodeDataShape                                      = 32768,
    siICENodeDataCustomType                                 = 65536,
    siICENodeDataString                                     = 131072,
    siICENodeDataIcon                                       = 262144,
    siICENodeDataValue                                      = 508927,
    siICENodeDataInterface                                  = 1024,
    siICENodeDataMultiComp                                  = 17400,
    siICENodeDataArithmeticSupport                          = 16894,
    siICENodeDataAny                                        = 524287,
};

//  Structure representing an attribute.
typedef struct icecache_attribute
{
    std::string             name;
    ULONG                   datatype;
    ParticleAttributeType   type;
    ULONG                   attribute_length;
    ULONG                   ptlocator_size;
    ULONG                   structtype;
    ULONG                   contexttype;
    ULONG                   objid;
    ULONG                   category;
    ParticleAttribute       attr_handle;

} icecache_attribute;


// Return true if this is a valid icecache
bool isIcecache(std::istream& input)
{
    char magic[8];
    input.read((char*)&magic, 8);
    char icecache_magic[8] = {'I','C','E','C','A','C','H','E'};
    if (*magic==*icecache_magic)
        return true;
    else
        return false;
}

// Read one value from the cache file
template<class T>
T read(std::istream& input)
{
    T value;
    input.read((char*)&value,sizeof(T));
    return value;
    //E::swap(d);
}

// Write one value from the cache file
template<class T>
void write(std::ostream& output,const T& value)
{
    output.write((char*)&value,sizeof(T));
}

// Read a name from cache file
std::string iceReadName(std::istream& input)
{
    ULONG length = read<ULONG>(input);
    ULONG orig_length = length;
    if ((length % 4) > 0) length += 4-(length%4);
    char *tmpStr = new char[length];
    input.read (tmpStr, length);
    std::string name(tmpStr, orig_length);
    delete[] tmpStr;
    return name;
}

// Write a name to cache file
void writeName(std::ostream& output, std::string name)
{
    ULONG length = name.size();
    write<ULONG>(output, length);
    ULONG right_fill = length%4;
    ULONG write_length = length;
    if (right_fill)
	{
        write_length += (4 - right_fill);
        name.append( (4 - right_fill), '_');
    }
    output << name;
    return;
}

// Read one attribute header and populate an icecache_attribute
void readAttribute(std::istream& input, icecache_attribute& attribute, ParticlesDataMutable& particles)
{
    attribute.name              = iceReadName(input);
    attribute.datatype          = read<ULONG>(input);
    switch (attribute.datatype) {
    case siICENodeDataLong:
    {
        attribute.type = INT;
        attribute.attribute_length = 1;
        break;
    }
    case siICENodeDataFloat:
    {
        attribute.type = FLOAT;
        attribute.attribute_length = 1;
        break;
    }
    case siICENodeDataVector3:
    {
        attribute.type = VECTOR;
        attribute.attribute_length = 3;
        break;
    }
    case siICENodeDataColor4:
    {
        attribute.type = FLOAT;
        attribute.attribute_length = 4;
        break;
    }
    case siICENodeDataLocation:
        attribute.type = NONE;
        break;
    case siICENodeDataRotation:
    {
        attribute.type = FLOAT;
        attribute.attribute_length = 4;
        break;
    }
    }
    attribute.ptlocator_size    = 0;
    attribute.structtype        = read<ULONG>(input);
    attribute.contexttype       = read<ULONG>(input);
    attribute.objid             = read<ULONG>(input);
    attribute.category          = read<ULONG>(input);
    if (attribute.datatype == 2048)
	{
        attribute.ptlocator_size = read<ULONG>(input);
        char datablock[attribute.ptlocator_size];
        input.read(datablock, attribute.ptlocator_size);
    }
    return;
}

// Read one attribute header and populate an icecache_attribute
icecache_attribute createAttribute(ParticleAttribute& attr)
{
    icecache_attribute attribute;
    attribute.name              = attr.name;
    switch (attr.type)
	{
		case FLOAT:
		{
			if (attr.count==1) attribute.datatype = siICENodeDataFloat;
			attribute.attribute_length=attr.count;
			break;
			if (attr.count==3) attribute.datatype = siICENodeDataVector3;
			attribute.attribute_length=attr.count;
			break;
			if (attr.count==4) attribute.datatype = siICENodeDataVector4;
			attribute.attribute_length=attr.count;
			break;
		}
		case INT:
		{
			attribute.datatype = siICENodeDataLong;
			attribute.attribute_length=attr.count;
			break;
		}
		case VECTOR:
		{
			attribute.datatype = siICENodeDataVector3;
			attribute.attribute_length=3;
			break;
		}
    }
    attribute.ptlocator_size    = 0;
    attribute.structtype        = siICENodeStructureSingle;
    attribute.contexttype       = siICENodeContextComponent0D;
    attribute.objid             = 0;
    attribute.category          = siICEAttributeCategoryCustom;
    if (attribute.name == "PointPosition")
        attribute.category = siICEAttributeCategoryBuiltin;
    attribute.attr_handle = attr;
    return attribute;
}
}
#endif
