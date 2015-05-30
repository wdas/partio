/* partio4Maya  5/02/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Instancer
Copyright 2013 (c)  All rights reserved

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

#include "partioInstancer.h"

#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>

static MGLFunctionTable *gGLFT = NULL;

#define ID_PARTIOINSTANCER  0x00116ED5 // id is registered with autodesk no need to change
#define LEAD_COLOR              18  // green
#define ACTIVE_COLOR            15  // white
#define ACTIVE_AFFECTED_COLOR   8   // purple
#define DORMANT_COLOR           4   // blue
#define HILITE_COLOR            17  // pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_LABEL 1
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

/// PARTIO INSTANCER
/*
Names and types of all array attributes  the maya instancer looks for
used by the geometry instancer nodes:  ( * = currently implemented)
position (vectorArray)  *
scale (vectorArray) *
shear (vectorArray)
visibility (doubleArray)
objectIndex (doubleArray) *
rotationType (doubleArray) *
rotation (vectorArray) *
aimDirection (vectorArray) *
aimPosition (vectorArray) *
aimAxis (vectorArray) *
aimUpAxis (vectorArray) *
aimWorldUp (vectorArray) *
age (doubleArray)
id (doubleArray) *
*/

MTypeId partioInstancer::id( ID_PARTIOINSTANCER);

/// ATTRS
MObject partioInstancer::time;
MObject partioInstancer::aByFrame;
MObject partioInstancer::aSize;         // The size of the logo
MObject partioInstancer::aFlipYZ;
MObject partioInstancer::aDrawStyle;
MObject partioInstancer::aPointSize;

/// Cache file related stuff
MObject partioInstancer::aUpdateCache;
MObject partioInstancer::aCacheDir;
MObject partioInstancer::aCacheFile;
MObject partioInstancer::aCacheActive;
MObject partioInstancer::aCacheOffset;
MObject partioInstancer::aCacheStatic;
MObject partioInstancer::aCacheFormat;
MObject partioInstancer::aForceReload;
MObject partioInstancer::aRenderCachePath;

/// point position / velocity
MObject partioInstancer::aComputeVeloPos;
MObject partioInstancer::aVeloMult;

/// attributes
MObject partioInstancer::aPartioAttributes;
MObject partioInstancer::aScaleFrom;
MObject partioInstancer::aRotationType;

MObject partioInstancer::aRotationFrom;
MObject partioInstancer::aAimDirectionFrom;
MObject partioInstancer::aAimPositionFrom;
MObject partioInstancer::aAimAxisFrom;
MObject partioInstancer::aAimUpAxisFrom;
MObject partioInstancer::aAimWorldUpFrom;

MObject partioInstancer::aLastScaleFrom;
MObject partioInstancer::aLastRotationFrom;
MObject partioInstancer::aLastAimDirectionFrom;
MObject partioInstancer::aLastAimPositionFrom;

MObject partioInstancer::aIndexFrom;

/// not implemented yet
//  MObject partioInstancer::aShaderIndexFrom;
//  MObject partioInstancer::aInMeshInstances;
//  MObject partioInstancer::aOutMesh;

//  output data to instancer
MObject partioInstancer::aInstanceData;
MObject partioInstancer::aExportAttributes;

namespace {
    // these two functions  check and clean out the instance array members if they exist or make them if they don't
    // cleaning out the arrays returned by getVectorData doesn't make any sense
    // because that returns a copy, rather than a reference, and both functions
    // are creating the arrays
    void updateInstanceDataVector(partioInstReaderCache& pvCache, MVectorArray& arrayToCheck,
        const MString& arrayChannel, unsigned int numParticles)
    {
        MStatus stat;
        MFnArrayAttrsData::Type vectorType;
        arrayToCheck = pvCache.instanceData.vectorArray(arrayChannel,&stat);
        CHECK_MSTATUS(stat);
        arrayToCheck.setLength(numParticles);
    }

    void updateInstanceDataDouble(partioInstReaderCache& pvCache, MDoubleArray& arrayToCheck,
        const MString& arrayChannel, unsigned int numParticles)
    {
        MStatus stat;
        MFnArrayAttrsData::Type doubleType;
        arrayToCheck = pvCache.instanceData.doubleArray(arrayChannel,&stat);
        CHECK_MSTATUS(stat);
        arrayToCheck.setLength(numParticles);
    }
}

partioInstReaderCache::partioInstReaderCache():
        bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
        dList(0),
        particles(NULL),
        flipPos(NULL)
{
}


/// Constructor
partioInstancer::partioInstancer() :
        attributeList(MStringArray()),
        mLastFileLoaded(""),
        mLastPath(""),
        mLastFile(""),
        mLastExt(""),
        mLastExportAttributes(""),        
        mLastRotationTypeIndex(-1),
        mLastRotationFromIndex(-1),
        mLastLastRotationFromIndex(-1),
        mLastAimDirectionFromIndex(-1),
        mLastLastAimDirecitonFromIndex(-1),
        mLastAimPositionFromIndex(-1),
        mLastLastAimPositionFromIndex(-1),
        mLastAimAxisFromIndex(-1),
        mLastAimUpAxisFromIndex(-1),
        mLastAimWorldUpFromIndex(-1),
        mLastScaleFromIndex(-1),
        mLastLastScaleFromIndex(-1),
        mLastIndexFromIndex(-1),
        mLastFlipStatus(false),
        mFlipped(false),
        frameChanged(false),
        canMotionBlur(false),
        multiplier(1.0f),        
        drawError(0),
        cacheChanged(false)

{
    pvCache.particles = NULL;
    pvCache.flipPos = (float *) malloc(sizeof(float));

    // create the instanceData object
    MStatus stat;
    pvCache.instanceDataObj = pvCache.instanceData.create ( &stat);
    CHECK_MSTATUS(stat);

}
/// DESTRUCTOR
partioInstancer::~partioInstancer()
{

    if (pvCache.particles)
    {
        pvCache.particles->release();
    }
    free(pvCache.flipPos);
    pvCache.instanceData.clear();
    pvCache.instanceDataObj.~MObject();


    MSceneMessage::removeCallback( partioInstancerOpenCallback);
    MSceneMessage::removeCallback( partioInstancerImportCallback);
    MSceneMessage::removeCallback( partioInstancerReferenceCallback);

}

void* partioInstancer::creator()
{
    //cout << "creator " << endl;
    return new partioInstancer;
}

/// POST CONSTRUCTOR
void partioInstancer::postConstructor()
{
    //cout << "postConstructor " << endl;
    setRenderable(true);
    partioInstancerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioInstancer::reInit, this);
    partioInstancerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioInstancer::reInit, this);
    partioInstancerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioInstancer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioInstancer::initCallback()
{
    //cout << "initCallback " << endl;
    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    mLastExt = partio4Maya::setExt(extENum);

    MPlug(tmo,aCacheDir).getValue(mLastPath);
    MPlug(tmo,aCacheFile).getValue(mLastFile);
    MPlug(tmo,aSize).getValue(multiplier);
    cacheChanged = false;

}

void partioInstancer::reInit(void *data)
{
    partioInstancer  *instNode = (partioInstancer*) data;
    instNode->initCallback();
}


MStatus partioInstancer::initialize()
{
    //cout << "initialize" << endl;
    MFnEnumAttribute    eAttr;
    MFnUnitAttribute    uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute   tAttr;
    MStatus             stat;

    time = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &stat);
    uAttr.setKeyable( true);

    aByFrame = nAttr.create( "byFrame", "byf", MFnNumericData::kInt ,1);
    nAttr.setKeyable( true);
    nAttr.setReadable( true);
    nAttr.setWritable( true);
    nAttr.setConnectable( true);
    nAttr.setStorable( true);
    nAttr.setMin(1);
    nAttr.setMax(100);

    aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance);
    uAttr.setDefault( 0.25);

    aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
    nAttr.setDefault ( false);
    nAttr.setKeyable ( true);

    aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString);
    tAttr.setReadable ( true);
    tAttr.setWritable ( true);
    tAttr.setKeyable ( true);
    tAttr.setConnectable ( true);
    tAttr.setStorable ( true);

    aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString);
    tAttr.setReadable ( true);
    tAttr.setWritable ( true);
    tAttr.setKeyable ( true);
    tAttr.setConnectable ( true);
    tAttr.setStorable ( true);

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &stat);
    nAttr.setKeyable(true);

    aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &stat);
    nAttr.setKeyable(true);

    aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
    std::map<short,MString> formatExtMap;
    partio4Maya::buildSupportedExtensionList(formatExtMap,false);
    for (unsigned short i = 0; i< formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(),   i);
    }

    eAttr.setDefault(4);  // PDC
    eAttr.setChannelBox(true);

    aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
    eAttr.addField("points",    0);
    eAttr.addField("index#",    1);
    //eAttr.addField("spheres", 2);
    eAttr.addField("boundingBox", 3);
    eAttr.setDefault(0);
    eAttr.setChannelBox(true);


    aPartioAttributes = tAttr.create("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder( true);

    aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
    nAttr.setDefault(2);
    nAttr.setKeyable(true);

    aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
    nAttr.setDefault(false);
    nAttr.setKeyable(false);

    aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);

    aRenderCachePath = tAttr.create("renderCachePath", "rcp", MFnStringData::kString);
    tAttr.setHidden(true);

    aScaleFrom = nAttr.create("scaleFrom", "sfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aLastScaleFrom = nAttr.create("lastScaleFrom", "lsfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

// ROTATION attrs
    aRotationType = nAttr.create("rotationType", "rottyp",  MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aRotationFrom = nAttr.create("rotationFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aLastRotationFrom = nAttr.create("lastRotationFrom", "lrfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAimDirectionFrom = nAttr.create("aimDirectionFrom", "adfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aLastAimDirectionFrom = nAttr.create("lastAimDirectionFrom", "ladfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAimPositionFrom = nAttr.create("aimPositionFrom", "apfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aLastAimPositionFrom = nAttr.create("lastAimPositionFrom", "lapfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAimAxisFrom = nAttr.create("aimAxisFrom", "aaxfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAimUpAxisFrom = nAttr.create("aimUpAxisFrom", "auaxfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAimWorldUpFrom = nAttr.create("aimWorldUpFrom", "awufrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aIndexFrom = nAttr.create("indexFrom", "ifrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aInstanceData = tAttr.create("instanceData", "instd", MFnArrayAttrsData::kDynArrayAttrs, &stat);
    tAttr.setKeyable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(false);
    tAttr.setReadable(true);

    aComputeVeloPos = nAttr.create("computeVeloPos", "cvp", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(false);

    aVeloMult = nAttr.create("veloMult", "vmul", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    nAttr.setHidden(false);
    nAttr.setReadable(true);

    aExportAttributes = tAttr.create("exportAttributes", "expattr", MFnStringData::kString);

// add attributes

    addAttribute(aSize);
    addAttribute(aFlipYZ);
    addAttribute(aDrawStyle);
    addAttribute(aPointSize);

    addAttribute(aUpdateCache);
    addAttribute(aCacheDir);
    addAttribute(aCacheFile);
    addAttribute(aCacheActive);
    addAttribute(aCacheOffset);
    addAttribute(aCacheStatic);
    addAttribute(aCacheFormat);
    addAttribute(aForceReload);
    addAttribute(aRenderCachePath);

    addAttribute(aComputeVeloPos);
    addAttribute(aVeloMult);

    addAttribute(aPartioAttributes);
    addAttribute(aScaleFrom);
    addAttribute(aRotationType);
    addAttribute(aRotationFrom);
    addAttribute(aAimDirectionFrom);
    addAttribute(aAimPositionFrom);
    addAttribute(aAimAxisFrom);
    addAttribute(aAimUpAxisFrom);
    addAttribute(aAimWorldUpFrom);

    addAttribute(aLastScaleFrom);
    addAttribute(aLastRotationFrom);
    addAttribute(aLastAimDirectionFrom);
    addAttribute(aLastAimPositionFrom);

    addAttribute(aIndexFrom);

    addAttribute(aInstanceData);

    addAttribute(aExportAttributes);

    addAttribute(aByFrame);
    addAttribute(time);

// attribute affects

    attributeAffects(aSize, aUpdateCache);
    attributeAffects(aFlipYZ, aUpdateCache);
    attributeAffects(aPointSize, aUpdateCache);
    attributeAffects(aDrawStyle, aUpdateCache);

    attributeAffects(aCacheDir, aUpdateCache);
    attributeAffects(aCacheFile, aUpdateCache);
    attributeAffects(aCacheActive, aUpdateCache);
    attributeAffects(aCacheOffset, aUpdateCache);
    attributeAffects(aCacheStatic, aUpdateCache);
    attributeAffects(aCacheFormat, aUpdateCache);
    attributeAffects(aForceReload, aUpdateCache);

    attributeAffects(aComputeVeloPos, aUpdateCache);
    attributeAffects(aVeloMult, aUpdateCache);

    attributeAffects(aScaleFrom, aUpdateCache);
    attributeAffects(aRotationType, aUpdateCache);

    attributeAffects(aRotationFrom, aUpdateCache);
    attributeAffects(aAimDirectionFrom, aUpdateCache);
    attributeAffects(aAimPositionFrom, aUpdateCache);
    attributeAffects(aAimAxisFrom, aUpdateCache);
    attributeAffects(aAimUpAxisFrom, aUpdateCache);
    attributeAffects(aAimWorldUpFrom, aUpdateCache);
    attributeAffects(aExportAttributes, aUpdateCache);

    attributeAffects(aLastScaleFrom, aUpdateCache);
    attributeAffects(aLastRotationFrom, aUpdateCache);
    attributeAffects(aLastAimDirectionFrom, aUpdateCache);
    attributeAffects(aLastAimPositionFrom, aUpdateCache);

    attributeAffects(aIndexFrom, aUpdateCache);

    attributeAffects(aInstanceData, aUpdateCache);
    attributeAffects(time, aUpdateCache);
    attributeAffects(time, aInstanceData);
    attributeAffects(aByFrame, aUpdateCache);
    attributeAffects(aByFrame, aInstanceData);

    return MS::kSuccess;
}


partioInstReaderCache* partioInstancer::updateParticleCache()
{
    GetPlugData(); // force update to run compute function where we want to do all the work
    return &pvCache;
}

// COMPUTE FUNCTION

MStatus partioInstancer::compute( const MPlug& plug, MDataBlock& block )
{
    //cout << "compute" << endl;
    MStatus stat;
    int rotationType                = block.inputValue(aRotationType).asInt();
    int rotationFromIndex           = block.inputValue(aRotationFrom).asInt();
    int lastRotFromIndex            = block.inputValue(aLastRotationFrom).asInt();
    int scaleFromIndex              = block.inputValue(aScaleFrom).asInt();
    int lastScaleFromIndex          = block.inputValue(aLastScaleFrom).asInt();
    int aimDirectionFromIndex       = block.inputValue(aAimDirectionFrom).asInt();
    int lastAimDirectionFromIndex   = block.inputValue(aLastAimDirectionFrom).asInt();
    int aimPositionFromIndex        = block.inputValue(aAimPositionFrom).asInt();
    int lastAimPositionFromIndex    = block.inputValue(aLastAimPositionFrom).asInt();
    int aimAxisFromIndex            = block.inputValue(aAimAxisFrom).asInt();
    int aimUpAxisFromIndex          = block.inputValue(aAimUpAxisFrom).asInt();
    int aimWorldUpFromIndex         = block.inputValue(aAimWorldUpFrom).asInt();
    int indexFromIndex              = block.inputValue(aIndexFrom).asInt();
    MString exportAttributes        = block.inputValue(aExportAttributes).asString();

    drawError = 0;
    bool cacheActive = block.inputValue(aCacheActive).asBool();

    if (!cacheActive)
    {
        drawError = 2;
        return ( MS::kSuccess);
    }

    // Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache && plug != aInstanceData)
    {
        return ( MS::kUnknownParameter);
    }

    else
    {

        MString cacheDir    = block.inputValue(aCacheDir).asString();
        MString cacheFile = block.inputValue(aCacheFile).asString();

        if (cacheDir  == "" || cacheFile == "" )
        {
            drawError = 1;
            // too much noise!
            //MGlobal::displayError("PartioEmitter->Error: Please specify cache file!");
            return ( MS::kFailure);
        }

        bool cacheStatic    = block.inputValue( aCacheStatic ).asBool();
        int cacheOffset     = block.inputValue( aCacheOffset ).asInt();
        short cacheFormat   = block.inputValue( aCacheFormat ).asShort();
        bool forceReload    = block.inputValue( aForceReload ).asBool();
        MTime inputTime     = block.inputValue( time ).asTime();
        int byFrame         = block.inputValue( aByFrame ).asInt();
        bool flipYZ         = block.inputValue( aFlipYZ ).asBool();
        MString renderCachePath = block.inputValue( aRenderCachePath ).asString();
        bool computeMotionBlur =block.inputValue( aComputeVeloPos ).asBool();
        float veloMult      = block.inputValue ( aVeloMult ).asFloat();

        float fps = static_cast<float>((MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit())));
        int integerTime = (int)floor((inputTime.value()) + .52);
        float deltaTime  = inputTime.value() - (float)integerTime;

        bool motionBlurStep = false;
        // motion  blur rounding  frame logic
        if (deltaTime != 0.0f)  // motion blur step?
            motionBlurStep = true;

        MString formatExt = "";
        int cachePadding = 0;

        MString newCacheFile = "";
        MString renderCacheFile = "";

        partio4Maya::updateFileName( cacheFile,  cacheDir,
                                     cacheStatic,  cacheOffset,
                                     cacheFormat,  integerTime, byFrame,
                                     cachePadding, formatExt,
                                     newCacheFile, renderCacheFile
                                  );

        if (renderCachePath != newCacheFile || renderCachePath != mLastFileLoaded )
        {
            block.outputValue(aRenderCachePath).setString(newCacheFile);
        }
        cacheChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
        if (mLastExt != formatExt || mLastPath != cacheDir || mLastFile != cacheFile ||  mLastFlipStatus  != flipYZ || forceReload )
        {
            cacheChanged = true;
            mFlipped = false;
            mLastFlipStatus = flipYZ;
            mLastExt = formatExt;
            mLastPath = cacheDir;
            mLastFile = cacheFile;
            block.outputValue(aForceReload).setBool(false);
        }

//////////////////////////////////////////////
/// or it can change from a time input change

        if (!partio4Maya::partioCacheExists(newCacheFile.asChar()))
        {
            if (pvCache.particles != NULL)
            {
                ParticlesDataMutable* newParticles;
                newParticles = pvCache.particles;
                pvCache.particles=NULL; // resets the pointer

                if (newParticles != NULL)
                {
                    newParticles->release(); // frees the mem
                }
            }
            pvCache.bbox.clear();
            pvCache.instanceData.clear();
            mLastFileLoaded = "";
            drawError = 1;
        }

        if ( newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar()) && (newCacheFile != mLastFileLoaded || forceReload) )
        {
            cacheChanged = true;
            mFlipped = false;
            MGlobal::displayWarning(MString("PartioInstancer->Loading: " + newCacheFile));

            if (pvCache.particles != NULL)
            {
                ParticlesDataMutable* newParticles;
                newParticles = pvCache.particles;
                pvCache.particles=NULL; // resets the pointer

                if (newParticles != NULL)
                {
                    newParticles->release(); // frees the mem
                }
            }
            pvCache.particles=read(newCacheFile.asChar());

            mLastFileLoaded = newCacheFile;
            if (pvCache.particles->numParticles() == 0)
            {
                pvCache.instanceData.clear();
                return (MS::kSuccess);
            }

            char partCount[50];
            sprintf (partCount, "%d", pvCache.particles->numParticles());
            MGlobal::displayInfo(MString ("PartioInstancer-> LOADED: ") + partCount + MString (" particles"));
            block.outputValue(aForceReload).setBool(false);
            block.setClean(aForceReload);

            pvCache.instanceData.clear();

        }

        if (pvCache.particles)
        {

            if (!pvCache.particles->attributeInfo("id",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ID",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("particleId",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ParticleId",pvCache.idAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find id attribute ");
                return ( MS::kFailure);
            }

            if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
                !pvCache.particles->attributeInfo("Position",pvCache.positionAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find position attribute ");
                return ( MS::kFailure);
            }

            // instanceData arrays
            MVectorArray  positionArray;
            MDoubleArray  idArray;

            // this creates or  gets an existing handles to the instanceData and then clears it to be ready to fill

            const int numParticles = pvCache.particles->numParticles();
            updateInstanceDataVector(pvCache, positionArray, "position", numParticles);
            updateInstanceDataDouble(pvCache, idArray, "id", numParticles);


            canMotionBlur = false;
            if (computeMotionBlur)
            {
                if ((pvCache.particles->attributeInfo("velocity",pvCache.velocityAttr) ||
                     pvCache.particles->attributeInfo("Velocity",pvCache.velocityAttr))||
                     pvCache.particles->attributeInfo("V"       ,pvCache.velocityAttr))
                {
                    canMotionBlur = true;
                }
                else
                {
                    MGlobal::displayWarning("PartioInstancer->Failed to find velocity attribute motion blur will be impaired ");
                }
            }

            pvCache.bbox.clear();


            // first we do position and ID because we need those two for sure
            for (int i = 0; i < pvCache.particles->numParticles(); ++i)
            {
                const float* partioPositions = pvCache.particles->data<float>(pvCache.positionAttr, i);
                MPoint pos(partioPositions[0], partioPositions[1], partioPositions[2]);

                if (canMotionBlur)
                {
                    const float * vel = pvCache.particles->data<float>(pvCache.velocityAttr, i);

                    MVector velo(vel[0], vel[1], vel[2]);
                    if (motionBlurStep)
                    {
                        //int mFps = (int)(MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit()));
                        pos += ((velo * veloMult) / fps) * deltaTime;
                    }
                }

                positionArray[i] = pos;

                const int* attrVal = pvCache.particles->data<int>(pvCache.idAttr,i);
                idArray[i] = (double)attrVal[0];

                // resize the bounding box
                pvCache.bbox.expand(pos);
            }

            // export custom attributes for the particle instancer
            if (motionBlurStep || cacheChanged ||
                mLastExportAttributes != exportAttributes)
            {
                MStringArray attrs;

                exportAttributes.split(' ', attrs);

                const unsigned int numAttrs = attrs.length();
                for (unsigned int i = 0; i < numAttrs; ++i)
                {
                    MString attrName = attrs[i];
                    ParticleAttribute particleAttr;
                    if (pvCache.particles->attributeInfo(attrName.asChar(), particleAttr))
                    {
                    	if (((particleAttr.type == Partio::VECTOR) || (particleAttr.type == Partio::FLOAT))
                    		&& (particleAttr.count == 3))
                    	{
                    		MVectorArray arr = pvCache.instanceData.vectorArray(attrName);
                    		arr.setLength(numParticles);

                    		for (int p = 0; p < numParticles; ++p)
							{
								const float* data = pvCache.particles->data<float>(particleAttr, p);
								arr[p] = MVector(static_cast<double>(data[0]),
												 static_cast<double>(data[1]),
												 static_cast<double>(data[2]));
							}
                    	}
                    	else if ((particleAttr.type == Partio::FLOAT) && (particleAttr.count == 1))
                    	{
                    		MDoubleArray arr = pvCache.instanceData.doubleArray(attrName);
                    		arr.setLength(numParticles);

                    		for (int p = 0; p < numParticles; ++p)
                    		{
                    			const float* data = pvCache.particles->data<float>(particleAttr, p);
                    			arr[p] = static_cast<double>(data[0]);
                    		}
                    	}
                    	else if ((particleAttr.type == Partio::INT) && (particleAttr.count == 1))
                    	{
                    		MIntArray arr = pvCache.instanceData.intArray(attrName);
							arr.setLength(numParticles);

							for (int p = 0; p < numParticles; ++p)
							{
								const int* data = pvCache.particles->data<int>(particleAttr, p);
								arr[p] = data[0];
							}
                    	}
                    }
                }

                mLastExportAttributes = exportAttributes;
            }

            if  (motionBlurStep          || cacheChanged ||
                 scaleFromIndex          != mLastScaleFromIndex ||
                 rotationFromIndex       != mLastRotationFromIndex ||
                 aimDirectionFromIndex   != mLastAimDirectionFromIndex ||
                 aimPositionFromIndex    != mLastAimPositionFromIndex ||
                 aimAxisFromIndex        != mLastAimAxisFromIndex ||
                 aimUpAxisFromIndex      != mLastAimUpAxisFromIndex ||
                 aimWorldUpFromIndex     != mLastAimWorldUpFromIndex ||
                 indexFromIndex          != mLastIndexFromIndex ||
                 lastScaleFromIndex      != mLastLastScaleFromIndex ||
                 lastRotFromIndex        != mLastLastRotationFromIndex ||
                 lastAimDirectionFromIndex!= mLastLastAimDirecitonFromIndex ||
                 lastAimPositionFromIndex!= mLastLastAimPositionFromIndex)
            {
                MDoubleArray indexArray;
                MVectorArray scaleArray;
                MVectorArray rotationArray;
                MDoubleArray visibiltyArray;
                MVectorArray aimDirectionArray;
                MVectorArray aimPositionArray;
                MVectorArray aimAxisArray;
                MVectorArray aimUpAxisArray;
                MVectorArray aimWorldUpArray;

                // clear these out to update on any change
                pvCache.rotationAttr.type = NONE;
                pvCache.aimDirAttr.type = NONE;
                pvCache.aimPosAttr.type = NONE;
                pvCache.aimAxisAttr.type = NONE;
                pvCache.aimUpAttr.type = NONE;
                pvCache.aimWorldUpAttr.type = NONE;
                pvCache.lastRotationAttr.type = NONE;
                pvCache.scaleAttr.type = NONE;
                pvCache.lastScaleAttr.type = NONE;
                pvCache.lastAimDirAttr.type = NONE;
                pvCache.lastAimPosAttr.type = NONE;
                pvCache.indexAttr.type = NONE;

                // Index
                if (indexFromIndex >=0)
                {
                    updateInstanceDataDouble(pvCache, indexArray, "objectIndex", numParticles);
                    pvCache.particles->attributeInfo(indexFromIndex,pvCache.indexAttr);
                }
                // Scale
                if (scaleFromIndex >=0)
                {
                    updateInstanceDataVector(pvCache, scaleArray, "scale", numParticles);
                    pvCache.particles->attributeInfo(scaleFromIndex,pvCache.scaleAttr);
                    if (lastScaleFromIndex >=0)
                    {
                        pvCache.particles->attributeInfo(lastScaleFromIndex,pvCache.lastScaleAttr);
                    }
                    else
                    {
                        pvCache.particles->attributeInfo(scaleFromIndex,pvCache.lastScaleAttr);
                    }
                }
                // Rotation
                if (rotationFromIndex >= 0)
                {
                    updateInstanceDataVector(pvCache, rotationArray, "rotation", numParticles);
                    pvCache.particles->attributeInfo(rotationFromIndex,pvCache.rotationAttr);
                    if (lastRotFromIndex >= 0)
                    {
                        pvCache.particles->attributeInfo(lastRotFromIndex,pvCache.lastRotationAttr);
                    }
                    else
                    {
                        pvCache.particles->attributeInfo(rotationFromIndex,pvCache.lastRotationAttr);
                    }
                }

                // Aim Direction
                if (aimDirectionFromIndex >= 0)
                {
                    updateInstanceDataVector(pvCache, aimDirectionArray, "aimDirection", numParticles);
                    pvCache.particles->attributeInfo(aimDirectionFromIndex,pvCache.aimDirAttr);
                    if (lastAimDirectionFromIndex >= 0)
                    {
                        pvCache.particles->attributeInfo(lastAimDirectionFromIndex,pvCache.lastAimDirAttr);
                    }
                    else
                    {
                        pvCache.particles->attributeInfo(aimDirectionFromIndex,pvCache.lastAimDirAttr);
                    }
                }
                // Aim Position
                if (aimPositionFromIndex >= 0)
                {
                    updateInstanceDataVector(pvCache, aimPositionArray, "aimPosition", numParticles);
                    pvCache.particles->attributeInfo(aimPositionFromIndex,pvCache.aimPosAttr);
                    if (lastAimPositionFromIndex >= 0)
                    {
                        pvCache.particles->attributeInfo(lastAimPositionFromIndex,pvCache.lastAimPosAttr);
                    }
                    else
                    {
                        pvCache.particles->attributeInfo(aimPositionFromIndex,pvCache.lastAimPosAttr);
                    }
                }
                // Aim Axis
                if (aimAxisFromIndex >= 0)
                {
                    updateInstanceDataVector( pvCache, aimAxisArray, "aimAxis", numParticles);
                    pvCache.particles->attributeInfo(aimAxisFromIndex,pvCache.aimAxisAttr);
                }
                // Aim Up Axis
                if (aimUpAxisFromIndex >= 0)
                {
                    updateInstanceDataVector( pvCache, aimUpAxisArray, "aimUpAxis", numParticles);
                    pvCache.particles->attributeInfo(aimUpAxisFromIndex,pvCache.aimUpAttr);
                }
                // World Up Axis
                if (aimWorldUpFromIndex >= 0)
                {
                    updateInstanceDataVector( pvCache, aimWorldUpArray, "aimWorldUp", numParticles);
                    pvCache.particles->attributeInfo(aimWorldUpFromIndex,pvCache.aimWorldUpAttr);
                }

                // MAIN LOOP ON PARTICLES
                for (int i = 0; i < numParticles; ++i)
                {
                    // SCALE
                    if (pvCache.scaleAttr.type == Partio::FLOAT )  // single float value for scale
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
                        float scale = attrVal[0];
                        if (canMotionBlur)
                        {
                            if (pvCache.lastScaleAttr.type == Partio::FLOAT )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr,i);
                                scale += (attrVal[0] - lastAttrVal[0])*deltaTime;
                            }
                        }
                        scaleArray[i] = MVector(scale, scale, scale);
                    }
                    else if (pvCache.scaleAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {

                        const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
                        MVector scale = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        if (canMotionBlur)
                        {
                            if (pvCache.lastScaleAttr.type == Partio::VECTOR )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr,i);
                                scale.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
                                scale.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
                                scale.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
                            }
                        }
                        scaleArray[i] = scale;
                    }
                    // ROTATION
                    if (pvCache.rotationAttr.type == Partio::FLOAT )  // single float value for rotation
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
                        float rot = attrVal[0];
                        if (canMotionBlur && lastRotFromIndex >= 0)
                        {
                            if (pvCache.lastRotationAttr.type == Partio::FLOAT )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr,i);
                                rot += (attrVal[0] - lastAttrVal[0])*deltaTime;
                            }
                        }
                        rotationArray[i] = MVector(rot, rot, rot);
                    }
                    else if (pvCache.rotationAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
                        MVector rot = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        if (canMotionBlur && lastRotFromIndex >= 0)
                        {
                            if (pvCache.lastRotationAttr.type == Partio::VECTOR )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr,i);
                                rot.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
                                rot.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
                                rot.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
                            }
                        }
                        rotationArray[i] = rot;
                    }

                    // AIM DIRECTION
                    if (pvCache.aimDirAttr.type == Partio::FLOAT )  // single float value for aimDirection
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr,i);
                        float aimDir = attrVal[0];
                        if (canMotionBlur)
                        {
                            if (pvCache.lastAimDirAttr.type == Partio::FLOAT )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr,i);
                                aimDir += (attrVal[0] - lastAttrVal[0])*deltaTime;
                            }
                        }
                        aimDirectionArray[i] = MVector(aimDir, aimDir, aimDir);
                    }
                    else if (pvCache.aimDirAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr,i);
                        MVector aimDir = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        if (canMotionBlur)
                        {
                            if (pvCache.lastAimDirAttr.type == Partio::VECTOR )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr,i);
                                aimDir.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
                                aimDir.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
                                aimDir.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
                                /// TODO: figure out why this is not working on subframes correctly
                                //cout << lastAttrVal[0] << " " << lastAttrVal[1] << " " << lastAttrVal[2] << endl;
                            }
                        }
                        aimDirectionArray[i] = aimDir;
                    }
                    // AIM POSITION
                    if (pvCache.aimPosAttr.type == Partio::FLOAT )  // single float value for aimDirection
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr,i);
                        float aimPos = attrVal[0];
                        if (canMotionBlur)
                        {
                            if (pvCache.lastAimPosAttr.type == Partio::FLOAT )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr,i);
                                aimPos += (attrVal[0] - lastAttrVal[0])*deltaTime;
                            }
                        }
                        aimPositionArray[i] = MVector(aimPos, aimPos, aimPos);
                    }
                    else if (pvCache.aimPosAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr,i);
                        MVector aimPos = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        if (canMotionBlur)
                        {
                            if (pvCache.lastAimPosAttr.type == Partio::VECTOR )
                            {
                                const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr,i);
                                aimPos.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
                                aimPos.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
                                aimPos.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
                            }
                        }
                        aimPositionArray[i] = aimPos;
                    }
                    // AIM Axis
                    if (pvCache.aimAxisAttr.type == Partio::FLOAT )  // single float value for aimDirection
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr,i);
                        float aimAxis = attrVal[0];
                        aimAxisArray[i] = MVector(aimAxis, aimAxis, aimAxis);
                    }
                    else if (pvCache.aimAxisAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr,i);
                        MVector aimAxis = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        aimAxisArray[i] = aimAxis;
                    }
                    // AIM Up Axis
                    if (pvCache.aimUpAttr.type == Partio::FLOAT )  // single float value for aimDirection
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr,i);
                        float aimUp = attrVal[0];
                        aimUpAxisArray[i] = MVector(aimUp, aimUp, aimUp);
                    }
                    else if (pvCache.aimUpAttr.type == Partio::VECTOR )   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr,i);
                        MVector aimUp = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        aimUpAxisArray[i] = aimUp;
                    }
                    // World Up Axis
                    if (pvCache.aimWorldUpAttr.type == Partio::FLOAT)  // single float value for aimDirection
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr,i);
                        float worldUp = attrVal[0];
                        aimWorldUpArray[i] = MVector(worldUp, worldUp, worldUp);
                    }
                    else if (pvCache.aimWorldUpAttr.type == Partio::VECTOR)   // we have a 4 float attribute ?
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr,i);
                        MVector worldUp = MVector(attrVal[0],attrVal[1],attrVal[2]);
                        aimWorldUpArray[i] = worldUp;
                    }
                    // INDEX
                    if (pvCache.indexAttr.type == Partio::FLOAT || pvCache.indexAttr.type == Partio::INT)  // single float value for index
                    {
                        if (pvCache.indexAttr.type == Partio::FLOAT)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
                            indexArray[i] = (double)(int)attrVal[0];
                        }
                        else if (pvCache.indexAttr.type == Partio::INT)
                        {
                            const int * attrVal = pvCache.particles->data<int>(pvCache.indexAttr,i);
                            indexArray[i] = (double)attrVal[0];
                        }
                    }
                    else if (pvCache.indexAttr.type == Partio::VECTOR)   // we have a 3or4 float attribute
                    {
                        const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
                        indexArray[i]= (double)(int)attrVal[0];
                    }
                } // end frame loop

                mLastScaleFromIndex = scaleFromIndex;
                mLastRotationFromIndex = rotationFromIndex;
                mLastAimDirectionFromIndex = aimDirectionFromIndex;
                mLastAimPositionFromIndex = aimPositionFromIndex;
                mLastAimAxisFromIndex = aimAxisFromIndex;
                mLastAimUpAxisFromIndex = aimUpAxisFromIndex;
                mLastAimWorldUpFromIndex = aimWorldUpFromIndex;
                mLastIndexFromIndex = indexFromIndex;
                mLastLastScaleFromIndex = lastScaleFromIndex;
                mLastLastRotationFromIndex = lastRotFromIndex;
                mLastLastAimDirecitonFromIndex = lastAimDirectionFromIndex;
                mLastLastAimPositionFromIndex = lastAimPositionFromIndex;

            } // end if frame/attrs changed

        } // end if particles
        else
            pvCache.instanceData.clear();
        //cout << pvCache.instanceData.list()<< endl;
        block.outputValue(aInstanceData).set(pvCache.instanceDataObj);
        block.setClean(aInstanceData);
    }


    if (pvCache.particles) // update the AE Controls for attrs in the cache
    {
        unsigned int numAttr=pvCache.particles->numAttributes();
        MPlug zPlug (thisMObject(), aPartioAttributes);

        if ((rotationFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aRotationFrom).setInt(-1);
        }
        if ((scaleFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aScaleFrom).setInt(-1);
        }
        if ((lastRotFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aLastRotationFrom).setInt(-1);
        }
        if ((lastScaleFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aLastScaleFrom).setInt(-1);
        }
        if ((indexFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aIndexFrom).setInt(-1);
        }
        if ((aimDirectionFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aAimDirectionFrom).setInt(-1);
        }
        if ((aimPositionFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aAimPositionFrom).setInt(-1);
        }
        if ((aimAxisFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aAimAxisFrom).setInt(-1);
        }
        if ((aimWorldUpFromIndex+1) > (int)zPlug.numElements())
        {
            block.outputValue(aAimWorldUpFrom).setInt(-1);
        }

        if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
        {
            //cout << "partioInstancer->refreshing AE controls" << endl;

            attributeList.clear();

            for (unsigned int i=0;i<numAttr;i++)
            {
                ParticleAttribute attr;
                pvCache.particles->attributeInfo(i,attr);

                // crazy casting string to  char
                char *temp;
                temp = new char[(attr.name).length()+1];
                strcpy (temp, attr.name.c_str());

                MString  mStringAttrName("");
                mStringAttrName += MString(temp);

                zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
                zPlug.setValue(MString(temp));
                attributeList.append(mStringAttrName);

                delete [] temp;
            }

            MArrayDataHandle hPartioAttrs = block.inputArrayValue(aPartioAttributes);
            MArrayDataBuilder bPartioAttrs = hPartioAttrs.builder();
            // do we need to clean up some attributes from our array?
            if (bPartioAttrs.elementCount() > numAttr)
            {
                unsigned int current = bPartioAttrs.elementCount();
                //unsigned int attrArraySize = current - 1;

                // remove excess elements from the end of our attribute array
                for (unsigned int x = numAttr; x < current; x++)
                {
                    bPartioAttrs.removeElement(x);
                }
            }
        }
    }
    block.setClean(plug);
    return MS::kSuccess;
}


/////////////////////////////////////////////////////
/// procs to override bounding box mode...
bool partioInstancer::isBounded() const
{
    return true;
}

MBoundingBox partioInstancer::boundingBox() const
{
    // Returns the bounding box for the shape.
    partioInstancer* nonConstThis = const_cast<partioInstancer*>(this);
    partioInstReaderCache* geom = nonConstThis->updateParticleCache();
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
    MPoint corner1 = geom->bbox.min();
    MPoint corner2 = geom->bbox.max();
    return MBoundingBox( corner1, corner2);
}

//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioInstancerUI::select( MSelectInfo &selectInfo,
                                MSelectionList &selectionList,
                                MPointArray &worldSpaceSelectPts ) const
{
    MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask);
    MSelectionList item;
    item.add( selectInfo.selectPath());
    MPoint xformedPt;
    selectInfo.addSelection( item, xformedPt, selectionList,
                             worldSpaceSelectPts, priorityMask, false);
    return true;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool partioInstancer::GetPlugData()
{
    MObject thisNode = thisMObject();
    int update = 0;
    MPlug updatePlug(thisNode, aUpdateCache);
    updatePlug.getValue( update);
    if (update != dUpdate)
    {
        dUpdate = update;
        return true;
    }
    else
    {
        return false;
    }
    return false;

}

// note the "const" at the end, its different than other draw calls
void partioInstancerUI::draw( const MDrawRequest& request, M3dView& view ) const
{
    MDrawData data = request.drawData();

    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    partioInstReaderCache* cache = (partioInstReaderCache*) data.geometry();

    MObject thisNode = shapeNode->thisMObject();
    MPlug sizePlug( thisNode, shapeNode->aSize);
    MDistance sizeVal;
    sizePlug.getValue( sizeVal);

    shapeNode->multiplier= (float) sizeVal.asCentimeters();

    int drawStyle = 0;
    MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle);
    drawStylePlug.getValue( drawStyle);

    view.beginGL();

    if (drawStyle < 3 && view.displayStyle() != M3dView::kBoundingBox )
    {
        drawPartio(cache,drawStyle,view);
    }
    else
    {
        drawBoundingBox();
    }

    if (shapeNode->drawError == 1)
    {
        glColor3f(.75f,0.0f,0.0f);
    }
    else if (shapeNode->drawError == 2)
    {
        glColor3f(0.0f,0.0f,0.0f);
    }

    partio4Maya::drawPartioLogo(shapeNode->multiplier);

    view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioInstancerUI::drawBoundingBox() const
{

    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    MPoint  bboxMin = shapeNode->pvCache.bbox.min();
    MPoint  bboxMax = shapeNode->pvCache.bbox.max();

    float xMin = float(bboxMin.x);
    float yMin = float(bboxMin.y);
    float zMin = float(bboxMin.z);
    float xMax = float(bboxMax.x);
    float yMax = float(bboxMax.y);
    float zMax = float(bboxMax.z);

    /// draw the bounding box
    glBegin (GL_LINES);

    glColor3f(1.0f,0.5f,0.5f);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMax,yMin,zMax);

    glVertex3f (xMin,yMin,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMin,zMax);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMax,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMax,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMax,yMax,zMax);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMax,yMax,zMin);


    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMax,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMin,yMin,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMin,zMax);

    glEnd();

}


////////////////////////////////////////////
/// DRAW PARTIO

void partioInstancerUI::drawPartio(partioInstReaderCache* pvCache, int drawStyle, M3dView &view) const
{
    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    MObject thisNode = shapeNode->thisMObject();
//  MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ);
//  bool flipYZVal;
//  flipYZPlug.getValue( flipYZVal);

    MPlug pointSizePlug( thisNode, shapeNode->aPointSize);
    float pointSizeVal;
    pointSizePlug.getValue( pointSizeVal);

    if (pvCache->particles)
    {
        glPushAttrib(GL_CURRENT_BIT);

        /// looping thru particles one by one...

        glPointSize(pointSizeVal);
        glColor3f(1.0,1.0,1.0);
        glBegin(GL_POINTS);

        MVectorArray positions = pvCache->instanceData.vectorArray("position");
        for (unsigned int i=0; i < positions.length(); i++)
        {
            glVertex3f(positions[i].x, positions[i].y, positions[i].z);
        }

        glEnd();
        glDisable(GL_POINT_SMOOTH);

        if (drawStyle == DRAW_STYLE_LABEL)
        {
            glColor3f(0.0,0.0,0.0);
            for (unsigned int i=0; i < positions.length();i++)
            {
                MString idVal;
                if (pvCache->indexAttr.type == Partio::FLOAT)
                {
                    const float * attrVal = pvCache->particles->data<float>(pvCache->indexAttr,i);
                    idVal = (double)(int)attrVal[0];
                }
                else if (pvCache->indexAttr.type == Partio::INT)
                {
                    const int * attrVal = pvCache->particles->data<int>(pvCache->indexAttr,i);
                    idVal = (double)attrVal[0];
                }
                else if (pvCache->indexAttr.type == Partio::VECTOR)
                {
                    const float * attrVal = pvCache->particles->data<float>(pvCache->indexAttr,i);
                    char idString[100];
                    sprintf(idString,"(%.2f,%.2f,%.2f)", (double)attrVal[0], (double)attrVal[1], (double)attrVal[2]);
                    idVal = idString;
                }
                else
                {
                    idVal = "";
                }
                /// TODO: draw text label per particle here
                view.drawText(idVal, MPoint(positions[i].x, positions[i].y, positions[i].z), M3dView::kLeft);
            }
        }

        glPopAttrib();
    } // if (particles)

}

partioInstancerUI::partioInstancerUI()
{
}
partioInstancerUI::~partioInstancerUI()
{
}

void* partioInstancerUI::creator()
{
    return new partioInstancerUI();
}


void partioInstancerUI::getDrawRequests(const MDrawInfo & info,
                                        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
    partioInstReaderCache* geom = shapeNode->updateParticleCache();

    getDrawData(geom, data);
    request.setDrawData(data);

    // Are we displaying locators?
    if (!info.objectDisplayStatus(M3dView::kDisplayLocators))
        return;

    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus)
    {
    case M3dView::kLead:
        request.setColor(LEAD_COLOR, activeColorTable);
        break;
    case M3dView::kActive:
        request.setColor(ACTIVE_COLOR, activeColorTable);
        break;
    case M3dView::kActiveAffected:
        request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
        break;
    case M3dView::kDormant:
        request.setColor(DORMANT_COLOR, dormantColorTable);
        break;
    case M3dView::kHilite:
        request.setColor(HILITE_COLOR, activeColorTable);
        break;
    default:
        break;
    }
    queue.add(request);

}

