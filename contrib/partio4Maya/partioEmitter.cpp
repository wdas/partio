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

#include "partioEmitter.h"

#define ID_PARTIOEMITTER  0x00116ED0 // id is registered with autodesk no need to change

#define McheckErr(stat, msg)\
	if ( MS::kSuccess != stat )\
	{\
		cerr << msg;\
		return MS::kFailure;\
	}

#define ATTR_TYPE_INT 0
#define ATTR_TYPE_DOUBLE 1
#define ATTR_TYPE_VECTOR 2

using namespace Partio;
using namespace std;


MTypeId partioEmitter::id ( ID_PARTIOEMITTER );

//// OBJECT Attributes
MObject partioEmitter::aCacheDir;
MObject partioEmitter::aCacheFile;
MObject partioEmitter::aCacheOffset;
MObject partioEmitter::aCacheActive;
MObject partioEmitter::aCacheFormat;
MObject partioEmitter::aCacheStatic;
MObject partioEmitter::aUseEmitterTransform;
MObject partioEmitter::aSize;
MObject partioEmitter::aFlipYZ;
MObject partioEmitter::aJitterPos;
MObject partioEmitter::aJitterFreq;
MObject partioEmitter::aPartioAttributes;
MObject partioEmitter::aMayaPPAttributes;

partioEmitter::partioEmitter():
        lastWorldPoint ( 0, 0, 0, 1 ),
        mLastFileLoaded(""),
        mLastPath(""),
        mLastFile(""),
        mLastExt(""),
        cacheChanged(false)
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
    MPlug(tmo, aCacheFile).getValue(mLastFile);
    cacheChanged = false;
}

void partioEmitter::reInit(void *data)
{
    partioEmitter  *emitterNode = (partioEmitter*) data;
    emitterNode->initCallback();
}


/// Creates the tracking ID attribute on the  particle object
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
    tAttr.setKeyable ( false );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( false );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &status );
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &status);
    nAttr.setKeyable(true);

    aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, 0, &status);
    nAttr.setKeyable(true);

    aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
    std::map<short,MString> formatExtMap;
    partio4Maya::buildSupportedExtensionList(formatExtMap,false);
    for (unsigned short i = 0; i< formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(),	i);
    }

    eAttr.setDefault(4); // PDC
    eAttr.setChannelBox(true);
    eAttr.setKeyable(false);

    aUseEmitterTransform = nAttr.create("useEmitterTransform", "uet", MFnNumericData::kBoolean, false, &status);
    nAttr.setKeyable(true);

    aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
    uAttr.setDefault( 0.25 );

    aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
    nAttr.setDefault ( false );
    nAttr.setKeyable ( true );

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
    status = addAttribute ( aCacheFile );
    status = addAttribute ( aCacheOffset );
    status = addAttribute ( aCacheActive );
    status = addAttribute ( aCacheStatic );
    status = addAttribute ( aCacheFormat );
    status = addAttribute ( aUseEmitterTransform );
    status = addAttribute ( aSize );
    status = addAttribute ( aFlipYZ );
    status = addAttribute ( aJitterPos );
    status = addAttribute ( aJitterFreq );
    status = addAttribute ( aPartioAttributes );
    status = addAttribute ( aMayaPPAttributes );
    status = attributeAffects ( aCacheDir, mOutput );
    status = attributeAffects ( aCacheFile, mOutput );
    status = attributeAffects ( aCacheOffset, mOutput );
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
    short cacheFormat	= block.inputValue( aCacheFormat ).asShort();
    float jitterPos		= block.inputValue( aJitterPos ).asFloat();
    float jitterFreq	= block.inputValue( aJitterFreq ).asFloat();
    bool useEmitterTxfm	= block.inputValue( aUseEmitterTransform ).asBool();
    bool cacheStatic    = block.inputValue( aCacheStatic ).asBool();
    MString cacheDir = block.inputValue(aCacheDir).asString();
    MString cacheFile = block.inputValue(aCacheFile).asString();


    // Determine if we are requesting the output plug for this emitter node.
    //
    if ( !( plug == mOutput ) )
        return ( MS::kUnknownParameter );

    MPlugArray  connectionArray;
    plug.connectedTo(connectionArray, false, true, &stat);

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

    if (cacheDir  == "" || cacheFile == "" )
    {
        // Too Much noise!
        //MGlobal::displayError("PartioEmitter->Error: Please specify cache file!");
        return ( MS::kFailure );
    }

    // Get the logical index of the element this plug refers to,
    // because the node can be emitting particles into more
    // than one particle shape.
    int multiIndex = plug.logicalIndex ( &status );
    McheckErr ( status, "ERROR in plug.logicalIndex.\n" );

    // Get output data arrays (position, velocity, or parentId)
    // that the particle shape is holding from the previous frame.
    MArrayDataHandle hOutArray = block.outputArrayValue ( mOutput, &status );
    McheckErr ( status, "ERROR in hOutArray = block.outputArrayValue.\n" );

    // Create a builder to aid in the array construction efficiently.
    MArrayDataBuilder bOutArray = hOutArray.builder ( &status );
    McheckErr ( status, "ERROR in bOutArray = hOutArray.builder.\n" );

    // Get the appropriate data array that is being currently evaluated.
    MDataHandle hOut = bOutArray.addElement ( multiIndex, &status );
    McheckErr ( status, "ERROR in hOut = bOutArray.addElement.\n" );

    // Create the data and apply the function set,
    // particle array initialized to length zero,
    // fnOutput.clear()
    MFnArrayAttrsData fnOutput;
    MObject dOutput = fnOutput.create ( &status );
    McheckErr ( status, "ERROR in fnOutput.create.\n" );

    // Check if the particle object has reached it's maximum,
    // hence is full. If it is full then just return with zero particles.
    bool beenFull = isFullValue ( multiIndex, block );
    if ( beenFull )
    {
        return ( MS::kSuccess );
    }

    // Get deltaTime, currentTime and startTime.
    // If deltaTime <= 0.0, or currentTime <= startTime,
    // do not emit new pariticles and return.
    MTime cT = currentTimeValue ( block );
    MTime sT = startTimeValue ( multiIndex, block );
    MTime dT = deltaTimeValue ( multiIndex, block );
    if ( ( cT <= sT ))
    {
        // We do not emit particles before the start time,
        // we do support emitting / killing of particles if we scroll backward in time.
        // This code is necessary primarily the first time to
        // establish the new data arrays allocated, and since we have
        // already set the data array to length zero it does
        // not generate any new particles.
        hOut.set ( dOutput );
        block.setClean ( plug );

        return ( MS::kSuccess );
    }

    bool motionBlurStep = false;

    int integerTime = (int)floor(cT.value()+.52);

    // parse and get the new file name
    MString formatExt = "";
    int cachePadding = 0;

    MString newCacheFile = "";
    MString renderCacheFile = "";

    partio4Maya::updateFileName( cacheFile,  cacheDir,
                                 cacheStatic,  cacheOffset,
                                 cacheFormat,  integerTime,
                                 cachePadding, formatExt,
                                 newCacheFile, renderCacheFile
                               );

    float deltaTime  = float(cT.value() - integerTime);

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

    if (mLastExt != formatExt || mLastPath != cacheDir || mLastFile != cacheFile)
    {
        cacheChanged = true;
        mLastExt = formatExt;
        mLastPath = cacheDir;
        mLastFile = cacheFile;
    }

    // check if a Partio cache filepath exists and is not the same as last frame
    //if ( cacheFile != "" && partioCacheExists(cacheFile.asChar()) && cacheFile != mLastFileLoaded)
    if ( newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar()))
    {
        MGlobal::displayInfo(MString("partioEmitter->Loading: " + newCacheFile));
        ParticlesDataMutable* particles=0;
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
                    // we attempt to use the array index as our only last resort
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

            unsigned int numAttr=particles->numAttributes();

            MPlug zPlug (thisMObject(), aPartioAttributes);
            MPlug yPlug (thisMObject(), aMayaPPAttributes);

            if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
            {
                //MGlobal::displayInfo("partioEmitter->refreshing AE controls");

                /*
                MString command = "";
                MString zPlugName = zPlug.name();
                MString yPlugName = yPlug.name();
                for (unsigned int x = 0; x<zPlug.numElements(); x++)
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
                */


                for (unsigned int i=0;i<numAttr;i++)
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
                /* this crashes
                MArrayDataHandle hPartioAttrs = block.inputArrayValue(aPartioAttributes);
                MArrayDataBuilder bPartioAttrs = hPartioAttrs.builder();
                MArrayDataHandle hMayaAttrs = block.inputArrayValue(aMayaPPAttributes);
                MArrayDataBuilder bMayaAttrs = hMayaAttrs.builder();
                // do we need to clean up some attributes from our array?
                // going to assume for now, that they are both equal
                if (bPartioAttrs.elementCount() > numAttr)
                {
                	unsigned int current = bPartioAttrs.elementCount();
                	//unsigned int attrArraySize = current - 1;

                	// remove excess elements from the end of our attribute array
                	for (unsigned int x = numAttr; x < current; x++)
                	{
                		bPartioAttrs.removeElement(x);
                		bMayaAttrs.removeElement(x);
                	}
                }
                */

                // after overwriting the string array with new attrs, delete any extra entries not needed
                MString command = "";
                MString zPlugName = zPlug.name();
                MString yPlugName = yPlug.name();
                for (unsigned int x = numAttr; x<zPlug.numElements(); x++)
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
                MGlobal::executeCommandOnIdle(command);
                MGlobal::executeCommand("refreshAE;");
            } // end cache changed

            std::map<std::string,  MVectorArray  > vectorAttrArrays;
            std::map<std::string,  MDoubleArray  > doubleAttrArrays;
            // we use this mapping to allow for direct writing of attrs to PP variables
            std::map<std::string, std::string > userPPMapping;

            for (unsigned int i=0;i<numAttr;i++)
            {
                ParticleAttribute attr;
                particles->attributeInfo(i,attr);

                yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);

                userPPMapping[yPlug.asString().asChar()] = attr.name;

                yPlug.selectAncestorLogicalIndex(i,aMayaPPAttributes);
                if (yPlug.asString() != "")
                {
                    MString ppAttrName = yPlug.asString();


                    if (attr.count == 3)
                    {
                        if (!part.isPerParticleVectorAttribute(ppAttrName))
                        {
                            MGlobal::displayInfo(MString("partioEmitter->adding ppAttr " + ppAttrName) );

                            // moving this to an outside mel proc
                            MString command;
                            command += "pioEmAddPPAttr ";
                            command += ppAttrName;
                            command += " vectorArray ";
                            command += partName;
                            command += ";";
                            MGlobal::executeCommandOnIdle(command);


                        }
                        if (part.isPerParticleVectorAttribute(ppAttrName))
                        {
                            MVectorArray vAttribute;
                            part.getPerParticleAttribute(ppAttrName, vAttribute, &status);
                            if ( !status )
                            {
                                MGlobal::displayError("PartioEmitter->could not get vector PP array ");
                            }
                            vectorAttrArrays[ppAttrName.asChar()] = vAttribute;
                        }

                    }
                    else if (attr.count == 1)
                    {
                        if (!part.isPerParticleDoubleAttribute(ppAttrName))
                        {
                            MGlobal::displayInfo(MString("PartioEmiter->adding ppAttr " + ppAttrName));
                            MString command;
                            command += "pioEmAddPPAttr ";
                            command += ppAttrName;
                            command += " doubleArray ";
                            command += partName;
                            command += ";";
                            MGlobal::executeCommandOnIdle(command);

                        }
                        if (part.isPerParticleDoubleAttribute(ppAttrName))
                        {
                            MDoubleArray dAttribute;
                            part.getPerParticleAttribute(ppAttrName, dAttribute, &status);
                            if ( !status )
                            {
                                MGlobal::displayError("PartioEmitter->could not get double PP array ");
                            }
                            doubleAttrArrays[ppAttrName.asChar()] = dAttribute;
                        }
                    }
                    else
                    {
                        MGlobal::displayError(MString("PartioEmitter->skipping attr: " + MString(attr.name.c_str())));
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

            for (unsigned int x = 0; x<part.count(); x++)
            {
                //cout << "partioEmitterDebug->moving existing particles" << endl;
                it = particleIDMap.find((int)partioIDs[x]);
                if (it != particleIDMap.end())   // move the existing particles
                {
                    const float* pos=particles->data<float>(posAttribute,it->second);
                    const float* vel=particles->data<float>(velAttribute,it->second);

                    MVector jitter = partio4Maya::jitterPoint(it->second, jitterFreq, float(seed), jitterPos);

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
            for (unsigned int y = 0; y< deletePoints.length(); y++)
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

                MVector jitter = partio4Maya::jitterPoint(it->second, jitterFreq, float(seed), jitterPos);
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


            MGlobal::displayInfo (MString ("PartioEmitter->Emitting  ") + inPosArray.length() + MString( " new particles"));
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
        MGlobal::displayError("PartioEmitter->Error loading the Cache file, it does not exist on disk, check path/file.");
        //cout << "partioEmitter->No File at: " << newCacheFile << endl;
    }

    // Update the data block with new dOutput and set plug clean.


    hOut.set ( dOutput );
    block.setClean ( plug );
    return ( MS::kSuccess );
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
    MObject worldMatrixAttr = fnThisNode.attribute ( "worldMatrix" );

    // build worldMatrix plug, and specify which element the plug refers to.
    // We use the first element(the first dagPath of this emitter).
    MPlug matrixPlug ( thisNode, worldMatrixAttr );
    matrixPlug = matrixPlug.elementByLogicalIndex ( 0 );

    // Get the value of the 'worldMatrix' attribute
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
    point[0] = worldMatrix ( 3, 0 );
    point[1] = worldMatrix ( 3, 1 );
    point[2] = worldMatrix ( 3, 2 );

    return ( status );
}


////////// DRAW  the  Partio Logo  helper

void partioEmitter::draw ( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus )
{

    view.beginGL();

    MObject thisNode = thisMObject();
    MPlug sizePlug( thisNode, aSize );
    MDistance sizeVal;
    sizePlug.getValue( sizeVal );

    float multiplier = (float) sizeVal.asCentimeters();
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
        initialStateAttr.setStorable (true);
        ppAttr.setStorable (true);
        ppAttr.setKeyable (true);
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


