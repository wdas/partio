/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
Copyright 2012 (c)  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include <maya/MIOStream.h>
#include <math.h>
#include <stdlib.h>

#include <maya/MGlobal.h>
#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnStringData.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MArrayDataBuilder.h>

#include "partioEmitter.h"
#include "partio4MayaShared.h"
#include "iconArrays.h"

// id is registered with autodesk no need to change
#define ID_PARTIOEMITTER  0x00116ECE

using namespace Partio;
using namespace std;


MTypeId partioEmitter::id ( ID_PARTIOEMITTER );

//// OBJECT Attributes
MObject partioEmitter::aCacheDir;
MObject partioEmitter::aCachePrefix;
MObject partioEmitter::aCacheOffset;
MObject partioEmitter::aCacheActive;
MObject partioEmitter::aCacheFormat;
MObject partioEmitter::aCachePadding;
MObject partioEmitter::aCacheStatic;
MObject partioEmitter::aUseEmitterTransform;
MObject partioEmitter::aJitterPos;
MObject partioEmitter::aJitterFreq;
MObject partioEmitter::aPartioAttributes;
MObject partioEmitter::aMayaPPAttributes;

MCallbackId partioEmitterOpenCallback;
MCallbackId partioEmitterImportCallback;
MCallbackId partioEmitterReferenceCallback;
MCallbackId partioEmitterConnectionMade;


partioEmitter::partioEmitter()
        :lastWorldPoint ( 0, 0, 0, 1 )
{
}

partioEmitter::~partioEmitter()
{
    MSceneMessage::removeCallback( partioEmitterOpenCallback);
    MSceneMessage::removeCallback( partioEmitterImportCallback);
    MSceneMessage::removeCallback( partioEmitterReferenceCallback);
	MDGMessage::removeCallback( partioEmitterConnectionMade );
}

void *partioEmitter::creator()
{
    return new partioEmitter;
}

void partioEmitter::postConstructor()
{
	MStatus stat;
    partioEmitterOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioEmitter::reInit, this);
    partioEmitterImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioEmitter::reInit, this);
    partioEmitterReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioEmitter::reInit, this);
	partioEmitterConnectionMade = MDGMessage::addConnectionCallback ( partioEmitter::connectionMadeCallbk, NULL, &stat );

}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioEmitter::initCallback()
{
    MObject tmo = thisMObject();

    short extEnum;
    MPlug(tmo, aCacheFormat).getValue(extEnum);

	mLastExt = partio4Maya::setExt(extEnum);

    MPlug(tmo, aCacheDir).getValue(mLastPath);
    MPlug(tmo, aCachePrefix).getValue(mLastPrefix);
    cacheChanged = false;
}

void partioEmitter::reInit(void *data)
{
    partioEmitter  *emitterNode = (partioEmitter*) data;
    emitterNode->initCallback();
}


void partioEmitter::connectionMadeCallbk(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData)
{
	MStatus status;
	MFnDependencyNode srcNode(srcPlug.node());
	MFnDependencyNode destNode(destPlug.node());

	if (srcNode.typeId() == partioEmitter::id && (destNode.typeName() =="particle" || destNode.typeName() =="nParticle" ) )
	{
		//cout << "connection made" << srcPlug.name() <<  " -> " << destPlug.name() << endl;
		MObject  particleShapeNode 	=  destPlug.node(&status);
		MFnParticleSystem part(particleShapeNode, &status);

		createPPAttr( part, "partioID",  "pioID", 1);

	}
}



MStatus partioEmitter::initialize()
//
//	Descriptions:
//		Initialize the node, create user defined attributes.
//
{
    MStatus status;
    MFnTypedAttribute tAttr;
    MFnUnitAttribute uAttr;
    MFnNumericAttribute nAttr;
    MFnEnumAttribute  eAttr;

    aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCachePrefix = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &status );
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &status);
    nAttr.setKeyable(true);

    aCachePadding = nAttr.create("cachePadding", "cachPad" , MFnNumericData::kInt, 4, &status );

	aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, 0, &status);
	nAttr.setKeyable(true);

    aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
	std::map<short,MString> formatExtMap;
	partio4Maya::buildSupportedExtensionList(formatExtMap,false);
	for (short i = 0; i< formatExtMap.size(); i++)
	{
		eAttr.addField(formatExtMap[i].toUpperCase(),	i);
	}

    eAttr.setDefault(4); // PDC
    eAttr.setChannelBox(true);

    aUseEmitterTransform = nAttr.create("useEmitterTransform", "uet", MFnNumericData::kBoolean, false, &status);
    nAttr.setKeyable(true);

    aJitterPos = nAttr.create("jitterPos", "jpos", MFnNumericData::kFloat,0.0, &status );
    nAttr.setDefault(0);
    nAttr.setMin(0);
    nAttr.setSoftMax(999);
    nAttr.setKeyable(true);

    aJitterFreq = nAttr.create("jitterFreq", "jfreq", MFnNumericData::kFloat, 1.0, &status);
    nAttr.setDefault(1.0);
    nAttr.setKeyable(true);

    aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);
    //tAttr.setUsesArrayDataBuilder( true );

    aMayaPPAttributes = tAttr.create("mayaPPAttributes", "pioMPPAts" , MFnStringData::kString);
    tAttr.setArray(true);
    //nAttr.setUsesArrayDataBuilder( true );


    status = addAttribute ( aCacheDir );
    status = addAttribute ( aCachePrefix );
    status = addAttribute ( aCacheOffset );
    status = addAttribute ( aCacheActive );
    status = addAttribute ( aCachePadding );
	status = addAttribute ( aCacheStatic );
    status = addAttribute ( aCacheFormat );
    status = addAttribute ( aUseEmitterTransform );
    status = addAttribute ( aJitterPos );
    status = addAttribute ( aJitterFreq );
    status = addAttribute ( aPartioAttributes );
    status = addAttribute ( aMayaPPAttributes );
    status = attributeAffects ( aCacheDir, mOutput );
    status = attributeAffects ( aCachePrefix, mOutput );
    status = attributeAffects ( aCacheOffset, mOutput );
    status = attributeAffects ( aCachePadding, mOutput );
	status = attributeAffects ( aCacheStatic, mOutput );
    status = attributeAffects ( aCacheFormat, mOutput );
    status = attributeAffects ( aUseEmitterTransform, mOutput );
    status = attributeAffects ( aJitterPos, mOutput );
    status = attributeAffects ( aJitterFreq, mOutput );
    return ( status );
}


MStatus partioEmitter::compute ( const MPlug& plug, MDataBlock& block )

{
	MStatus status, stat;

	bool cacheActive = block.inputValue(aCacheActive).asBool();
    if (!cacheActive)
    {
        return ( MS::kSuccess );
    }

    int cacheOffset 	= block.inputValue( aCacheOffset ).asInt();
    int cachePadding	= block.inputValue( aCachePadding ).asInt();
    short cacheFormat	= block.inputValue( aCacheFormat ).asShort();
    float jitterPos		= block.inputValue( aJitterPos ).asFloat();
    float jitterFreq	= block.inputValue( aJitterFreq ).asFloat();
    bool useEmitterTxfm	= block.inputValue( aUseEmitterTransform ).asBool();
	bool cacheStatic    = block.inputValue( aCacheStatic ).asBool();

    MString formatExt;

    // Determine if we are requesting the output plug for this emitter node.
    //
    if ( !( plug == mOutput ) )
        return ( MS::kUnknownParameter );

	MPlugArray  connectionArray;
    plug.connectedTo(connectionArray, FALSE, TRUE, &stat);

	MPlug particleShapeOutPlug 	=  connectionArray[0];
	MObject  particleShapeNode 	=  particleShapeOutPlug.node(&stat);
	MFnParticleSystem part(particleShapeNode, &stat);
	MString partName = part.particleName();

	MString emitterPlugName = plug.name();
	MString particleShapeOutPlugName = particleShapeOutPlug.name();

	if (!part.isPerParticleDoubleAttribute("partioID"))
	{
		MGlobal::displayWarning("PartioEmitter->error:  was unable to create/find partioID attr");
		return ( MS::kFailure );
	}

	MString cacheDir = block.inputValue(aCacheDir).asString();
    MString cachePrefix = block.inputValue(aCachePrefix).asString();

    if (cacheDir  == "" || cachePrefix == "" )
    {
        MGlobal::displayError("PartioEmitter->Error: Please specify cache file!");
        return ( MS::kFailure );
    }

   // Get the logical index of the element this plug refers to,
    // because the node can be emitting particles into more
    // than one particle shape.
    //
    int multiIndex = plug.logicalIndex ( &status );
    McheckErr ( status, "ERROR in plug.logicalIndex.\n" );

    // Get output data arrays (position, velocity, or parentId)
    // that the particle shape is holding from the previous frame.
    //
    MArrayDataHandle hOutArray = block.outputArrayValue ( mOutput, &status );
    McheckErr ( status, "ERROR in hOutArray = block.outputArrayValue.\n" );

    // Create a builder to aid in the array construction efficiently.
    //
    MArrayDataBuilder bOutArray = hOutArray.builder ( &status );
    McheckErr ( status, "ERROR in bOutArray = hOutArray.builder.\n" );

    // Get the appropriate data array that is being currently evaluated.
    //
    MDataHandle hOut = bOutArray.addElement ( multiIndex, &status );
    McheckErr ( status, "ERROR in hOut = bOutArray.addElement.\n" );

    // Create the data and apply the function set,
    // particle array initialized to length zero,
    // fnOutput.clear()
    //
    MFnArrayAttrsData fnOutput;
    MObject dOutput = fnOutput.create ( &status );
    McheckErr ( status, "ERROR in fnOutput.create.\n" );

    // Check if the particle object has reached it's maximum,
    // hence is full. If it is full then just return with zero particles.
    //
    bool beenFull = isFullValue ( multiIndex, block );
    if ( beenFull )
    {
        return ( MS::kSuccess );
    }

    // Get deltaTime, currentTime and startTime.
    // If deltaTime <= 0.0, or currentTime <= startTime,
    // do not emit new pariticles and return.
    //

    MTime cT = currentTimeValue ( block );
    MTime sT = startTimeValue ( multiIndex, block );
    MTime dT = deltaTimeValue ( multiIndex, block );
    if ( ( cT <= sT ))
    {
        // We do not emit particles before the start time,
        // and do not emit particles when moving backwards in time.
        //

        // This code is necessary primarily the first time to
        // establish the new data arrays allocated, and since we have
        // already set the data array to length zero it does
        // not generate any new particles.
        //
        hOut.set ( dOutput );
        block.setClean ( plug );

        return ( MS::kSuccess );
    }

    bool motionBlurStep = false;

    int integerTime = (int)floor(cT.value()+.52);

	// parse and get the new file name
	MString newCacheFile = partio4Maya::updateFileName(cachePrefix, cacheDir,cacheStatic,cacheOffset,cachePadding,cacheFormat,integerTime, formatExt);

    float deltaTime  = cT.value() - integerTime;

    // motion  blur rounding  frame logic
    if ((deltaTime < 1 || deltaTime > -1)&& deltaTime !=0)  // motion blur step?
    {
        motionBlurStep = true;
    }

    long seed = seedValue( multiIndex, block );

    /// get the emitter offset
    MPoint emitterOffset;
    getWorldPosition(emitterOffset);
    double inheritFactor = inheritFactorValue(multiIndex, block);

    // using std:map to give us a nice fast binary search
    map<int, int>  particleIDMap;

    cacheChanged = false;

    if (mLastExt != formatExt || mLastPath != cacheDir || mLastPrefix != cachePrefix)
    {
        cacheChanged = true;
        mLastExt = formatExt;
        mLastPath = cacheDir;
        mLastPrefix = cachePrefix;
    }

    // check if a Partio cache filepath exists and is not the same as last frame
    //if ( cacheFile != "" && partioCacheExists(cacheFile.asChar()) && cacheFile != mLastFileLoaded)
    if ( newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar()))
    {
        cout << "partioEmitter->Loading: " << newCacheFile << endl;
        ParticlesData* particles=0;
        ParticleAttribute IdAttribute;
        ParticleAttribute posAttribute;
        ParticleAttribute velAttribute;


        particles=read(newCacheFile.asChar());
        if (particles)
        {
		    //mLastFileLoaded = cacheFile;

            // SETUP THE MAP!

            for (int i=0;i<particles->numParticles();i++)
            {
                int id = -1;
                if (particles->attributeInfo("id",IdAttribute) || particles->attributeInfo("Id",IdAttribute))
                {
					const int* partioID = particles->data<int>(IdAttribute,i);
                    id = partioID[0];
                }
                else if (particles->attributeInfo("particleId",IdAttribute) || particles->attributeInfo("ParticleId",IdAttribute))
				{
					const int* partioID = particles->data<int>(IdAttribute,i);
                    id = partioID[0];
				}
                else
                {
					MGlobal::displayWarning("Loaded Partio cache has a non-standard or non-existant id attribute, this may render things unstable");
                    id = i;
                }
                particleIDMap[id] = i;
            }

            if (!particles->attributeInfo("position",posAttribute) && !particles->attributeInfo("Position",posAttribute))
			{
				std::cerr<<"Failed to find position attribute "<<std::endl;
				return ( MS::kFailure );
			}
			if (!particles->attributeInfo("velocity",velAttribute) && !particles->attributeInfo("Velocity",velAttribute))
			{
				std::cerr<<"Failed to find velocity attribute "<<std::endl;
				return ( MS::kFailure );
			}

			map <int, int>::iterator it;

			it = particleIDMap.begin();
			it = particleIDMap.end();

			int numAttr=particles->numAttributes();

			MPlug zPlug (thisMObject(), aPartioAttributes);
			MPlug yPlug (thisMObject(), aMayaPPAttributes);

			if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
			{
				cout << "partioEmitter->refreshing AE controls" << endl;

				MString command = "";
				MString zPlugName = zPlug.name();
				MString yPlugName = yPlug.name();
				for (int x = 0; x<zPlug.numElements(); x++)
				{
					command += "removeMultiInstance -b true ";
					command += zPlugName;
					command += "[";
					command += x;
					command += "]";
					command += ";removeMultiInstance -b true ";
					command += yPlugName;
					command += "[";
					command += x;
					command += "]";
					command += "; ";

				}

				MGlobal::executeCommand(command);
				zPlug.setNumElements(0);
				yPlug.setNumElements(0);


				for (int i=0;i<numAttr;i++)
				{
					ParticleAttribute attr;
					particles->attributeInfo(i,attr);

					// crazy casting string to char
					char *temp;
					temp = new char[(attr.name).length()+1];
					strcpy (temp, attr.name.c_str());

					MString  mStringAttrName("");
					mStringAttrName += MString(temp);

					zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
					zPlug.setValue(MString(temp));

					yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
					yPlug.setValue(MString(""));

					delete [] temp;
				}
			} // end cache changed

			std::map<std::string,  MVectorArray  > vectorAttrArrays;
			std::map<std::string,  MDoubleArray  > doubleAttrArrays;
			// we use this mapping to allow for direct writing of attrs to PP variables
			std::map<std::string, std::string > userPPMapping;

			for (int i=0;i<numAttr;i++)
			{
				ParticleAttribute attr;
				particles->attributeInfo(i,attr);

				yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);

				userPPMapping[yPlug.asString().asChar()] = attr.name;

				yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
				if (yPlug.asString() != "")
				{
					MString ppAttrName = yPlug.asString();
					MString shortName =  ppAttrName.substring(0,ppAttrName.length()-1);

					if (attr.type == VECTOR)
					{
						if (!part.isPerParticleVectorAttribute(ppAttrName))
						{
							/// NO WORKY YET
							//createPPAttr( part, ppAttrName,  shortName, 2);

							/// but this does.
							cout <<  "partioEmitter->adding ppAttr " << ppAttrName << endl;
							MString command;
							command += "if (!`attributeExists ";
							command += ppAttrName;
							command += " ";
							command += partName;
							command += "`){";
							command += "addAttr -ln \"";
							command += ppAttrName;
							command +="0\"  -dt vectorArray ";
							command += partName;
							command += ";addAttr -ln \"";
							command += ppAttrName;
							command +="\"  -dt vectorArray ";
							command += partName;
							command += ";setAttr -e -keyable true ";
							command += partName;
							command += ".";
							command += ppAttrName;
							command +=";}";
							MGlobal::executeCommandOnIdle(command);

						}
						if (part.isPerParticleVectorAttribute(ppAttrName))
						{
							MVectorArray vAttribute;
							part.getPerParticleAttribute(ppAttrName, vAttribute, &status);
							if ( !status )
							{
								cout << "partioEmitter->could not get vector PP array " << endl;
							}
							vectorAttrArrays[ppAttrName.asChar()] = vAttribute;
						}

					}
					else if (attr.type == FLOAT)
					{
						if (!part.isPerParticleDoubleAttribute(ppAttrName))
						{
							/// NO WORKY YET
							//createPPAttr( part, ppAttrName,  shortName, 1);

							/// but this does.
							cout <<  "partioEmiter->adding ppAttr " << ppAttrName << endl;
							MString command;
							command += "if (!`attributeExists ";
							command += ppAttrName;
							command += "";
							command += partName;
							command += "`){";
							command += "addAttr -ln \"";
							command += ppAttrName;
							command +="0\"  -dt doubleArray ";
							command += partName;
							command += ";addAttr -ln \"";
							command += ppAttrName;
							command +="\"  -dt doubleArray ";
							command += partName;
							command += ";setAttr -e -keyable true ";
							command += partName;
							command += ".";
							command += ppAttrName;
							command +=";";
							MGlobal::executeCommandOnIdle(command);
						}
						if (part.isPerParticleDoubleAttribute(ppAttrName))
						{
							MDoubleArray dAttribute;
							part.getPerParticleAttribute(ppAttrName, dAttribute, &status);
							if ( !status )
							{
								cout << "partioEmitter->could not get  double PP array " << endl;
							}
							doubleAttrArrays[ppAttrName.asChar()] = dAttribute;
						}
					}
					else
					{
						cout << "partioEmitter->skipping attr: "  << (attr.name) << endl;
					}
				}
			}

			// end loop to add user PP attributes

			MPointArray inPosArray;
			MVectorArray inVelArray;

			/// BASIC things to always load
			MIntArray ids;
			part.particleIds(ids);
			MVectorArray positions;
			part.position(positions);
			MDoubleArray lifespans;
			part.lifespan(lifespans);
			MVectorArray velocities;
			part.velocity(velocities);
			MDoubleArray partioIDs;
			part.getPerParticleAttribute("partioID", partioIDs);

			MIntArray  deletePoints;

			std::map <std::string, MVectorArray >::iterator vecIt;
			std::map <std::string, MDoubleArray >::iterator doubleIt;

			for (int x = 0; x<part.count(); x++)
			{
				//cout << "partioEmitterDebug->moving existing particles" << endl;
				it = particleIDMap.find((int)partioIDs[x]);
				if (it != particleIDMap.end())   // move the existing particles
				{
					const float* pos=particles->data<float>(posAttribute,it->second);
					const float* vel=particles->data<float>(velAttribute,it->second);

					MVector jitter = jitterPoint(it->second, jitterFreq, seed, jitterPos);

					positions[x] = MVector(pos[0],pos[1],pos[2])+(jitter);
					if (useEmitterTxfm) {
						positions[x] += emitterOffset;
					}
					MVector velo(vel[0],vel[1],vel[2]);
					if (motionBlurStep)
					{
						positions[x] += (velo/24)*deltaTime;
					}

					velocities[x] = velo;

					for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
					{
						ParticleAttribute doubleAttr;
						particles->attributeInfo(userPPMapping[doubleIt->first].c_str(),doubleAttr);
						const float*  doubleVal = particles->data<float>(doubleAttr, it->second);
						doubleAttrArrays[doubleIt->first][x] = doubleVal[0];
					}
					for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
					{
						ParticleAttribute vectorAttr;
						particles->attributeInfo(userPPMapping[vecIt->first].c_str(), vectorAttr);
						const float* vecVal = particles->data<float>(vectorAttr, it->second);
						vectorAttrArrays[vecIt->first][x] = MVector(vecVal[0],vecVal[1],vecVal[2]);
					}

					particleIDMap.erase(it);
				}
				else // kill the particle
				{
					deletePoints.append(x);
				}
			}

			// TODO: handle a "release" attribute list to allow expressions to force partio emitter to forget or skip over certain particles
			for (int y = 0; y< deletePoints.length(); y++)
			{
				//cout << "partioEmitterDebug->deleting particles" << endl;
				positions.remove(deletePoints[y]-y);
				velocities.remove(deletePoints[y]-y);
				lifespans.remove(deletePoints[y]-y);

				for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
				{
					doubleAttrArrays[doubleIt->first].remove(deletePoints[y]-y);
				}
				for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
				{
					vectorAttrArrays[vecIt->first].remove(deletePoints[y]-y);
				}

				part.setCount (particles->numParticles());
			}

			//  everything left is new particles

			for (it = particleIDMap.begin(); it != particleIDMap.end(); it++)
			{

				//cout << "partioEmitterDebug->emitting new particles" << endl;
				const float* pos=particles->data<float>(posAttribute,it->second);
				const float* vel=particles->data<float>(velAttribute,it->second);

				/// TODO: this will break if we're dealing with a format type that has no "id" attribute or one is not found using the
				///       standard  nomenclature "id" or  "particleId"

				const int* id=particles->data<int>(IdAttribute,it->second);
				MVector temp(pos[0], pos[1], pos[2]);

				MVector jitter = jitterPoint(it->second, jitterFreq, seed, jitterPos);
				if (useEmitterTxfm) {
					temp += emitterOffset;
				}

				MVector velo(vel[0],vel[1],vel[2]);

				inPosArray.append(temp+(jitter));
				inVelArray.append(velo*inheritFactor);
				partioIDs.append(id[0]);

				for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
				{
					ParticleAttribute doubleAttr;
					particles->attributeInfo(userPPMapping[doubleIt->first].c_str(),doubleAttr);
					const float*  doubleVal = particles->data<float>(doubleAttr, it->second);
					doubleAttrArrays[doubleIt->first].append(doubleVal[0]);
				}

				for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
				{
					ParticleAttribute vectorAttr;
					particles->attributeInfo(userPPMapping[vecIt->first].c_str(), vectorAttr);
					const float* vecVal = particles->data<float>(vectorAttr, it->second);
					vectorAttrArrays[vecIt->first].append(MVector(vecVal[0],vecVal[1],vecVal[2]));
				}

			}

			part.setPerParticleAttribute("position", positions);
			part.setPerParticleAttribute("velocity", velocities);
			part.setPerParticleAttribute("lifespanPP", lifespans);


			cout << "partioEmitter->Emitting  " << inPosArray.length() << " new particles" << endl;
			part.emit(inPosArray, inVelArray);

			part.setPerParticleAttribute("partioID", partioIDs);

			for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
			{
				part.setPerParticleAttribute(MString(doubleIt->first.c_str()), doubleAttrArrays[doubleIt->first]);
			}
			for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
			{
				part.setPerParticleAttribute(MString(vecIt->first.c_str()), vectorAttrArrays[vecIt->first]);
			}

			particles->release();


			//MArrayDataHandle outputArray = block.outputArrayValue(aPartioAttributes,&stat);
			//stat = outputArray.set(builder);
			//MArrayDataHandle outputArrayChx = block.outputArrayValue(aPartioAttrCheckbox, &stat);
			//stat = outputArrayChx.set(builderChx);

		} // end if particles
    }
    else
    {
        MGlobal::displayError("Error loading the Cache file it does not exist on disk, check path/prefix. ");
        cout << "partioEmitter->No File at: " << newCacheFile << endl;
    }

    // Update the data block with new dOutput and set plug clean.
    //

    hOut.set ( dOutput );
    block.setClean ( plug );
    return ( MS::kSuccess );
}

MVector partioEmitter::jitterPoint(int id, float freq, float offset, float jitterMag)
///* generate a constant noise offset for this  ID
//  and return as a vector to add to the particle position
//
{
    MVector jitter(0,0,0);
    if (jitterMag > 0)
    {
        jitter.x = ((noiseAtValue((id+.124+offset)*freq))-.5)*2;
        jitter.y = ((noiseAtValue((id+1042321+offset)*freq))-.5)*2;
        jitter.z = ((noiseAtValue((id-2350212+offset)*freq))-.5)*2;

        jitter*= jitterMag;
    }

    return  jitter;
}


MStatus partioEmitter::getWorldPosition ( MPoint &point )
//
//	Descriptions:
//		get the emitter position in the world space.
//		The position value is from inherited attribute, aWorldMatrix.
//
{
    MStatus status;

    MObject thisNode = thisMObject();
    MFnDependencyNode fnThisNode ( thisNode );

    // get worldMatrix attribute.
    //
    MObject worldMatrixAttr = fnThisNode.attribute ( "worldMatrix" );

    // build worldMatrix plug, and specify which element the plug refers to.
    // We use the first element(the first dagPath of this emitter).
    //
    MPlug matrixPlug ( thisNode, worldMatrixAttr );
    matrixPlug = matrixPlug.elementByLogicalIndex ( 0 );

    // Get the value of the 'worldMatrix' attribute
    //
    MObject matrixObject;
    status = matrixPlug.getValue ( matrixObject );
    if ( !status )
    {
        status.perror ( "partioEmitter::getWorldPosition: get matrixObject" );
        return ( status );
    }

    MFnMatrixData worldMatrixData ( matrixObject, &status );
    if ( !status )
    {
        status.perror ( "partioEmitter::getWorldPosition: get worldMatrixData" );
        return ( status );
    }

    MMatrix worldMatrix = worldMatrixData.matrix ( &status );
    if ( !status )
    {
        status.perror ( "partioEmitter::getWorldPosition: get worldMatrix" );
        return ( status );
    }

    // assign the translate to the given vector.
    //
    point[0] = worldMatrix ( 3, 0 );
    point[1] = worldMatrix ( 3, 1 );
    point[2] = worldMatrix ( 3, 2 );

    return ( status );
}


////////// DRAW  the  Partio Logo  helper

void partioEmitter::draw ( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus )
{

    view.beginGL();

    float multiplier = 1;
	partio4Maya::drawPartioLogo(multiplier);

    view.endGL ();
}

//////////////////////////////////////////////////
MStatus partioEmitter::createPPAttr( MFnParticleSystem  &part, MString attrName, MString shortName, int type)
{
		// type = 0 = int
		// type = 1 = double
		// type = 2 = vector

		//  make the initial state attr first
		MFnTypedAttribute initialStateAttr;
		MFnTypedAttribute ppAttr;

		MStatus stat1,stat2 = MS::kFailure;
		MObject initialStateAttrObj;
		MObject attrObj;
		switch (type)
		{
			case 0:
			{
				if (!part.isPerParticleIntAttribute((attrName+"0")) && !part.isPerParticleIntAttribute(attrName))
				{
					initialStateAttrObj = initialStateAttr.create ((attrName+"0"), (shortName+"0"), MFnData::kIntArray, &stat1);
					attrObj 			= ppAttr.create ((attrName), (shortName), MFnData::kIntArray, &stat2);
				}
			}
			break;
			case 1:
			{
				if (!part.isPerParticleDoubleAttribute((attrName+"0")) && !part.isPerParticleDoubleAttribute(attrName))
				{
				initialStateAttrObj = initialStateAttr.create ((attrName+"0"), (shortName+"0"), MFnData::kDoubleArray, &stat1);
				attrObj 			= ppAttr.create ((attrName), (shortName), MFnData::kDoubleArray, &stat2);
				}
			}
			break;
			case 2:
			{
				if (!part.isPerParticleVectorAttribute((attrName+"0")) && !part.isPerParticleVectorAttribute(attrName))
				{
				initialStateAttrObj = initialStateAttr.create ((attrName+"0"), (shortName+"0"), MFnData::kVectorArray, &stat1);
				attrObj 			= ppAttr.create ((attrName), (shortName), MFnData::kVectorArray, &stat2);
				}
			}
			break;
			default:
			{}
		}
			if (stat1 == MStatus::kSuccess && stat2 == MStatus::kSuccess)
			{
				initialStateAttr.setStorable (TRUE);
				ppAttr.setStorable (TRUE);
				ppAttr.setKeyable (TRUE);
				stat1 = part.addAttribute (initialStateAttrObj, MFnDependencyNode::kLocalDynamicAttr);
				if (!stat1)
				{
					MGlobal::displayWarning("PartioEmitter->error:  was unable to create "+attrName+"0"+ " attr");
				}
				stat2 = part.addAttribute (attrObj, MFnDependencyNode::kLocalDynamicAttr);
				if (!stat2)
				{
					MGlobal::displayWarning("PartioEmitter->error:  was unable to create "+ (attrName)+ " attr");
				}
			}
			if (stat1 != MStatus::kSuccess || stat2 != MStatus::kSuccess)

			{
				return MStatus::kFailure;
			}
			return MStatus::kSuccess;
}


////////////////////////////////////////////////////
/// NOISE FOR JITTER!



const int kTableMask = TABLE_SIZE - 1;

float partioEmitter::noiseAtValue( float x )
//
//  Description:
//      Get the Noise value at the given point in 1-space and time
//
//  Arguments:
//      x - the point at which to calculate the lxgNoise
//
//  Return Value:
//      the Noise value at the point
//
{
    int ix;
    float fx;

    if ( !isInitialized ) {
        initTable( 23479015 );
        isInitialized = 1;
    }

    ix = (int)floorf( x );
    fx = x - (float)ix;

    return spline( fx, value( ix - 1 ), value( ix ), value( ix + 1 ), value( ix + 2 ) );
}



void  partioEmitter::initTable( long seed )
//
//  Description:
//      Initialize the table of random values with the given seed.
//
//  Arguments:
//      seed - the new seed value
//
{
    srand48( seed );

    for ( int i = 0; i < TABLE_SIZE; i++ ) {
        valueTable1[i] = (float)drand48();
        valueTable2[i] = (float)drand48();
        valueTable3[i] = (float)drand48();
    }
    isInitialized = 1;
}


float partioEmitter::spline( float x, float knot0, float knot1, float knot2, float knot3 )
//
//  Description:
//      This is a simple version of a Catmull-Rom spline interpolation.
//
//  Assumptions:
//
//      0 < x < 1
//
//
{
    float c0, c1, c2, c3;

    // Evaluate span of cubic at x using Horner's rule
    //
    c3 = (-0.5F * knot0 ) + ( 1.5F * knot1 ) + (-1.5F * knot2 ) + ( 0.5F * knot3 );
    c2 = ( 1.0F * knot0 ) + (-2.5F * knot1 ) + ( 2.0F * knot2 ) + (-0.5F * knot3 );
    c1 = (-0.5F * knot0 ) + ( 0.0F * knot1 ) + ( 0.5F * knot2 ) + ( 0.0F * knot3 );
    c0 = ( 0.0F * knot0 ) + ( 1.0F * knot1 ) + ( 0.0F * knot2 ) + ( 0.0F * knot3 );

    return ( ( c3 * x + c2 ) * x + c1 ) * x + c0;;
}

int partioEmitter::isInitialized = 0;

int partioEmitter::permtable[256] = {
    254,    91,     242,    186,    90,     204,    85,     133,    233,
    50,     187,    49,     182,    224,    144,    166,    7,      51,
    20,     179,    36,     203,    114,    156,    195,    40,     24,
    60,     162,    84,     126,    102,    63,     194,    220,    161,
    72,     94,     193,    229,    140,    57,     3,      189,    106,
    54,     164,    198,    199,    44,     245,    235,    100,    87,
    25,     41,     62,     111,    13,     70,     27,     82,     69,
    53,     66,     247,    124,    67,     163,    125,    155,    228,
    122,    19,     113,    143,    121,    9,      1,      241,    171,
    200,    83,     244,    185,    170,    141,    115,    190,    154,
    48,     32,     178,    127,    167,    56,     134,    15,     160,
    238,    64,     6,      11,     196,    232,    26,     89,     0,
    219,    112,    68,     30,     215,    227,    75,     132,    71,
    239,    251,    92,     14,     104,    231,    29,     180,    150,
    226,    191,    47,     73,     37,     183,    88,     105,    42,
    22,     2,      38,     5,      119,    74,     249,    184,    52,
    8,      55,     118,    255,    206,    173,    165,    78,     31,
    123,    98,     212,    80,     139,    61,     138,    77,     177,
    45,     137,    145,    28,     168,    128,    95,     223,    35,
    205,    76,     211,    175,    81,     33,     207,    21,     131,
    58,     152,    16,     240,    18,     96,     210,    109,    214,
    216,    202,    148,    34,     146,    117,    176,    93,     246,
    172,    97,     159,    197,    218,    65,     147,    253,    221,
    217,    79,     101,    142,    23,     149,    99,     39,     12,
    135,    110,    234,    108,    153,    129,    4,      169,    174,
    116,    243,    130,    107,    222,    10,     43,     188,    46,
    213,    252,    86,     157,    192,    236,    158,    120,    17,
    103,    248,    225,    230,    250,    208,    181,    151,    237,
    201,    59,     136,    209
};

float partioEmitter::valueTable1[256];
float partioEmitter::valueTable2[256];
float partioEmitter::valueTable3[256];

