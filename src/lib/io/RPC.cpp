/*
PARTIO SOFTWARE
Copyright (c) 2013  Disney Enterprises, Inc. and Contributors,  All rights reserved

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

Format Contributed by github user: redpawfx (redpawFX@gmail.com)  and Luma Pictures  2013
Some code for this format  was helped along  by referring to the official nextlimit repo for their fileformats
*/

#include "../Partio.h"
#include "../core/ParticleHeaders.h"
#include "PartioEndian.h"
#include "ZIP.h"
#include "3rdParty/nextLimit/RPCReader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>


ENTER_PARTIO_NAMESPACE

using namespace std;

static const long RPC_MAGIC = 0x70FABADA;

typedef struct{
    int verificationCode;
    uint version; // current version = 3
	uint numParticles;
	uint numChannels;
	const float* bboxMin;
	const float* bboxMax;

} RPC_HEADER;

ParticlesDataMutable* readRPC(const char* filename, const bool headersOnly)
{
	RPCFile reader;
	if(!reader.Open(filename))
	{
		assert(!"Cannot open file or file version is incompatible");
		return 0;
	}

	RPC_HEADER header;
	header.bboxMax = reader.GetBBoxMax();
	header.bboxMin = reader.GetBBoxMin();
	header.numChannels = reader.GetNumChannels();
	header.numParticles = reader.GetNumParticles();

	/// DEBUG
	//cout << "numParticles: " << header.numParticles << endl;
	//cout << "numChannels: " << header.numChannels << endl;
	//cout << "bboxMin: " << header.bboxMin[0] << " " << header.bboxMin[1] << " " << header.bboxMin[2]  << endl;
	//cout << "bboxMax: " << header.bboxMax[0] << " " << header.bboxMax[1] << " " << header.bboxMax[2]  << endl;


	ParticlesDataMutable* partData = headersOnly ? new ParticleHeaders: create();
    partData->addParticles(header.numParticles);
	std::vector<const RPCFile::ChannelInfo*> chanInfoVec;
	int sizeArry[header.numChannels];
	std::vector<void*> dataArrays;
	std::vector<Partio::ParticleAttribute>  partAttrVec;

	// PARTIO TYPES
	//ParticleAttributeType {NONE=0,VECTOR=1,FLOAT=2,INT=3,INDEXEDSTR=4};
	for (uint chan = 0; chan < header.numChannels; chan++)
	{
		const RPCFile::ChannelInfo* chanInfo;
		chanInfo = reader.GetChannelInfo(chan);
		Partio::ParticleAttributeType type = NONE;
		unsigned short size = 0;
		chanInfoVec.push_back(chanInfo);

		void* buf = reader.ReadWholeChannel(chan);
		dataArrays.push_back(buf);

		switch(chanInfo->GetDataType())
		{
			case RPCFile::ChannelDataType_Float3:
				type = VECTOR;
				size = 3;
				break;
			case RPCFile::ChannelDataType_Double3:
				type = VECTOR;
				size = 3;
				break;

			case RPCFile::ChannelDataType_Float2:
				type = FLOAT;
				size = 2;
				break;

			case RPCFile::ChannelDataType_UInt64:
				type = INT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_UInt16:
				type = INT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_UInt8:
				type = INT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_Int32:
				type = INT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_Bool:
				type = INT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_Float:
				type = FLOAT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_Double:
				type = FLOAT;
				size = 1;
				break;

			case RPCFile::ChannelDataType_Float4:
				type = VECTOR;
				size = 4;
				break;

			default:
				assert(!"Unknown data type in PrintChannelInfo()");
				type = NONE;
				size = 0;
				break;
		}

		if (type > 0 && size > 0)
		{
			sizeArry[chan] = size;
			ParticleAttribute chAttr;
			chAttr = partData->addAttribute(chanInfo->GetName(),  type, size);
			partAttrVec.push_back(chAttr);
		}
	}
	if (!headersOnly) // headersOnly
	{
		for(int partIndex = 0; partIndex < partData->numParticles(); partIndex++)
        {
			for (uint chan = 0; chan < header.numChannels; chan++)
			{
				int chanType = chanInfoVec[chan]->GetDataType();

				if (chanType == RPCFile::ChannelDataType_Float ||
					chanType == RPCFile::ChannelDataType_Float2 ||
					chanType == RPCFile::ChannelDataType_Float3 ||
					chanType == RPCFile::ChannelDataType_Float4)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<float>(partAttrVec[chan], partIndex)[chanLength] = static_cast<float*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_Double ||
						 chanType == RPCFile::ChannelDataType_Double3)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<float>(partAttrVec[chan], partIndex)[chanLength] = static_cast<double*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_Bool)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<int>(partAttrVec[chan], partIndex)[chanLength] = static_cast<bool*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_UInt64)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<int>(partAttrVec[chan], partIndex)[chanLength] = static_cast<uint64_t*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_Int32)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<int>(partAttrVec[chan], partIndex)[chanLength] = static_cast<int32_t*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_UInt16)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<int>(partAttrVec[chan], partIndex)[chanLength] = static_cast<uint16_t*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
				else if (chanType == RPCFile::ChannelDataType_UInt8)
				{
					for (int chanLength = 0; chanLength<sizeArry[chan]; chanLength++)
					{
						partData->dataWrite<int>(partAttrVec[chan], partIndex)[chanLength] = static_cast<uint8_t*>(dataArrays[chan])[sizeArry[chan]*partIndex+chanLength];
					}
				}
			}
		}
	}
	for (uint chan = 0; chan < header.numChannels; chan++)
	{
		int chanType = chanInfoVec[chan]->GetDataType();

		if (chanType == RPCFile::ChannelDataType_Float ||
			chanType == RPCFile::ChannelDataType_Float2 ||
			chanType == RPCFile::ChannelDataType_Float3 ||
			chanType == RPCFile::ChannelDataType_Float4)
		{
			delete static_cast<float*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_Double ||
				 chanType == RPCFile::ChannelDataType_Double3)
		{
			delete static_cast<double*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_Bool)
		{
			delete static_cast<bool*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_UInt64)
		{
			delete	static_cast<uint64_t*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_Int32)
		{
			delete static_cast<int32_t*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_UInt16)
		{
			delete static_cast<uint16_t*>(dataArrays[chan]);
		}
		else if (chanType == RPCFile::ChannelDataType_UInt8)
		{
			delete static_cast<uint8_t*>(dataArrays[chan]);
		}
	}
	reader.Close();
	return partData;
}

// TODO implement writer for RPC
bool writeRPC(const char* filename,const ParticlesData& p,const bool /*compressed*/)
{
	return true;
}

EXIT_PARTIO_NAMESPACE
