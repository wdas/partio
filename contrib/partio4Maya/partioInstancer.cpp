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

static MGLFunctionTable* gGLFT = NULL;

#define ID_PARTIOINSTANCER  0x00116ED5 // id is registered with autodesk no need to change
#define LEAD_COLOR              18  // green
#define ACTIVE_COLOR            15  // white
#define ACTIVE_AFFECTED_COLOR   8   // purple
#define DORMANT_COLOR           4   // blue
#define HILITE_COLOR            17  // pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_LABEL 1
#define DRAW_STYLE_BOUNDING_BOX 3
#define USE_SLERP_FOR_QUATERIONS


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

MTypeId partioInstancer::id(ID_PARTIOINSTANCER);

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
MObject partioInstancer::aVelocityMult;

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
MObject partioInstancer::aLastPositionFrom;
MObject partioInstancer::aVelocityFrom;
MObject partioInstancer::aAngularVelocityFrom;

MObject partioInstancer::aIndexFrom;

/// not implemented yet
//  MObject partioInstancer::aShaderIndexFrom;
//  MObject partioInstancer::aInMeshInstances;
//  MObject partioInstancer::aOutMesh;

//  output data to instancer
MObject partioInstancer::aInstanceData;
MObject partioInstancer::aExportAttributes;
MObject partioInstancer::aVelocitySource;
MObject partioInstancer::aAngularVelocitySource;

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
        arrayToCheck = pvCache.instanceData.vectorArray(arrayChannel, &stat);
        CHECK_MSTATUS(stat);
        arrayToCheck.setLength(numParticles);
    }

    void updateInstanceDataDouble(partioInstReaderCache& pvCache, MDoubleArray& arrayToCheck,
                                  const MString& arrayChannel, unsigned int numParticles)
    {
        MStatus stat;
        MFnArrayAttrsData::Type doubleType;
        arrayToCheck = pvCache.instanceData.doubleArray(arrayChannel, &stat);
        CHECK_MSTATUS(stat);
        arrayToCheck.setLength(numParticles);
    }

    enum VelocitySource {
        VS_BUILTIN = 0,
        VS_CUSTOM_CHANNEL,
        VS_LAST_POSITION
    };

    enum AngularVelocitySource {
        AVS_LAST_ROTATION,
        AVS_ANGULAR_VELOCITY
    };

    enum FloatTupleSize {
        FLOAT_TUPLE_NONE = 0,
        FLOAT_TUPLE_SINGLE,
        FLOAT_TUPLE_TRIPLE,
        FLOAT_TUPLE_QUAD
    };

    FloatTupleSize getFloatTupleSize(const PARTIO::ParticleAttribute& attribute)
    {
        if (attribute.type == PARTIO::FLOAT)
        {
            if (attribute.count == 1)
                return FLOAT_TUPLE_SINGLE;
            else if (attribute.count == 3)
                return FLOAT_TUPLE_TRIPLE;
            else if (attribute.count == 4)
                return FLOAT_TUPLE_QUAD;
            else
                return FLOAT_TUPLE_NONE;
        }
        else if (attribute.type == PARTIO::VECTOR && attribute.count == 3)
            return FLOAT_TUPLE_TRIPLE;
        else
            return FLOAT_TUPLE_NONE;
    }
}

partioInstReaderCache::partioInstReaderCache() :
        bbox(MBoundingBox(MPoint(0, 0, 0, 0), MPoint(0, 0, 0, 0))),
        particles(NULL)
{
}


/// Constructor
partioInstancer::partioInstancer() :
        m_attributeList(MStringArray()),
        m_lastFileLoaded(""),
        m_lastPath(""),
        m_lastFile(""),
        m_lastExt(""),
        m_lastExportAttributes(""),
        m_lastRotationType(""),
        m_lastRotationFrom(""),
        m_lastLastRotationFrom(""),
        m_lastAimDirectionFrom(""),
        m_lastLastAimDirectionFrom(""),
        m_lastAimPositionFrom(""),
        m_lastLastAimPositionFrom(""),
        m_lastAimAxisFrom(""),
        m_lastAimUpAxisFrom(""),
        m_lastAimWorldUpFrom(""),
        m_lastScaleFrom(""),
        m_lastLastScaleFrom(""),
        m_lastIndexFrom(""),
        m_lastAngularVelocityFrom(""),
        m_lastAngularVelocitySource(-1),
        m_lastFlipStatus(false),
        m_flipped(false),
        m_frameChanged(false),
        m_canMotionBlur(false),
        m_multiplier(1.0f),
        m_drawError(0),
        m_cacheChanged(false)
{
    pvCache.particles = 0;

    // create the instanceData object
    MStatus stat;
    pvCache.instanceDataObj = pvCache.instanceData.create(&stat);
    CHECK_MSTATUS(stat);

}

/// DESTRUCTOR
partioInstancer::~partioInstancer()
{
    if (pvCache.particles)
        pvCache.particles->release();
    pvCache.instanceData.clear();
    pvCache.instanceDataObj.~MObject();

    MSceneMessage::removeCallback(partioInstancerOpenCallback);
    MSceneMessage::removeCallback(partioInstancerImportCallback);
    MSceneMessage::removeCallback(partioInstancerReferenceCallback);
}

void* partioInstancer::creator()
{
    return new partioInstancer;
}

/// POST CONSTRUCTOR
void partioInstancer::postConstructor()
{
    //cout << "postConstructor " << endl;
    setRenderable(true);
    partioInstancerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioInstancer::reInit, this);
    partioInstancerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioInstancer::reInit,
                                                               this);
    partioInstancerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference,
                                                                  partioInstancer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////
void partioInstancer::initCallback()
{
    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    m_lastExt = partio4Maya::setExt(extENum);

    MPlug(tmo, aCacheDir).getValue(m_lastPath);
    MPlug(tmo, aCacheFile).getValue(m_lastFile);
    MPlug(tmo, aSize).getValue(m_multiplier);
    m_cacheChanged = false;

}

void partioInstancer::reInit(void* data)
{
    partioInstancer* instNode = (partioInstancer*)data;
    instNode->initCallback();
}


MStatus partioInstancer::initialize()
{
    MFnEnumAttribute eAttr;
    MFnUnitAttribute uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;
    MStatus stat;

    time = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &stat);
    uAttr.setKeyable(true);

    aByFrame = nAttr.create("byFrame", "byf", MFnNumericData::kInt, 1);
    nAttr.setKeyable(true);
    nAttr.setReadable(true);
    nAttr.setWritable(true);
    nAttr.setConnectable(true);
    nAttr.setStorable(true);
    nAttr.setMin(1);
    nAttr.setMax(100);

    aSize = uAttr.create("iconSize", "isz", MFnUnitAttribute::kDistance);
    uAttr.setDefault(0.25);

    aFlipYZ = nAttr.create("flipYZ", "fyz", MFnNumericData::kBoolean);
    nAttr.setDefault(false);
    nAttr.setKeyable(true);

    aCacheDir = tAttr.create("cacheDir", "cachD", MFnStringData::kString);
    tAttr.setReadable(true);
    tAttr.setWritable(true);
    tAttr.setKeyable(true);
    tAttr.setConnectable(true);
    tAttr.setStorable(true);

    aCacheFile = tAttr.create("cachePrefix", "cachP", MFnStringData::kString);
    tAttr.setReadable(true);
    tAttr.setWritable(true);
    tAttr.setKeyable(true);
    tAttr.setConnectable(true);
    tAttr.setStorable(true);

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &stat);
    nAttr.setKeyable(true);

    aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &stat);
    nAttr.setKeyable(true);

    aCacheFormat = eAttr.create("cacheFormat", "cachFmt");
    std::map<short, MString> formatExtMap;
    partio4Maya::buildSupportedExtensionList(formatExtMap, false);
    for (unsigned short i = 0; i < formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(), i);
    }

    eAttr.setDefault(4);  // PDC
    eAttr.setChannelBox(true);

    aDrawStyle = eAttr.create("drawStyle", "drwStyl");
    eAttr.addField("points", 0);
    eAttr.addField("index#", 1);
    //eAttr.addField("spheres", 2);
    eAttr.addField("boundingBox", 3);
    eAttr.setDefault(0);
    eAttr.setChannelBox(true);


    aPartioAttributes = tAttr.create("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder(true);

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

    aScaleFrom = tAttr.create("scaleFrom", "sfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aLastScaleFrom = tAttr.create("lastScaleFrom", "lsfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

// ROTATION attrs
    aRotationType = tAttr.create("rotationType", "rottyp", MFnStringData::kString);
    nAttr.setKeyable(true);

    aRotationFrom = tAttr.create("rotationFrom", "rfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aLastRotationFrom = tAttr.create("lastRotationFrom", "lrfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAimDirectionFrom = tAttr.create("aimDirectionFrom", "adfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aLastAimDirectionFrom = tAttr.create("lastAimDirectionFrom", "ladfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAimPositionFrom = tAttr.create("aimPositionFrom", "apfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aLastAimPositionFrom = tAttr.create("lastAimPositionFrom", "lapfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAimAxisFrom = tAttr.create("aimAxisFrom", "aaxfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAimUpAxisFrom = tAttr.create("aimUpAxisFrom", "auaxfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAimWorldUpFrom = tAttr.create("aimWorldUpFrom", "awufrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aIndexFrom = tAttr.create("indexFrom", "ifrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aLastPositionFrom = tAttr.create("lastPositionFrom", "lpfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aVelocityFrom = tAttr.create("velocityFrom", "velfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aAngularVelocityFrom = tAttr.create("angularVelocityFrom", "avelfrm", MFnStringData::kString);
    nAttr.setKeyable(true);

    aInstanceData = tAttr.create("instanceData", "instd", MFnArrayAttrsData::kDynArrayAttrs, &stat);
    tAttr.setKeyable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(false);
    tAttr.setReadable(true);

    aComputeVeloPos = nAttr.create("computeVeloPos", "cvp", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(false);

    aVelocityMult = nAttr.create("veloMult", "vmul", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    nAttr.setHidden(false);
    nAttr.setReadable(true);

    aExportAttributes = tAttr.create("exportAttributes", "expattr", MFnStringData::kString);

    aVelocitySource = eAttr.create("velocitySource", "vsrc");
    eAttr.addField("Built-in", 0);
    eAttr.addField("Custom Channel", 1);
    eAttr.addField("Last Position", 2);

    aAngularVelocitySource = eAttr.create("angularVelocitySource", "avsrc");
    eAttr.addField("Last Rotation", 0);
    eAttr.addField("Angular Velocity", 1);

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
    addAttribute(aVelocityMult);

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
    addAttribute(aLastPositionFrom);
    addAttribute(aVelocityFrom);
    addAttribute(aAngularVelocityFrom);

    addAttribute(aIndexFrom);

    addAttribute(aInstanceData);

    addAttribute(aExportAttributes);
    addAttribute(aVelocitySource);
    addAttribute(aAngularVelocitySource);

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
    attributeAffects(aVelocityMult, aUpdateCache);

    attributeAffects(aScaleFrom, aUpdateCache);
    attributeAffects(aRotationType, aUpdateCache);

    attributeAffects(aRotationFrom, aUpdateCache);
    attributeAffects(aAimDirectionFrom, aUpdateCache);
    attributeAffects(aAimPositionFrom, aUpdateCache);
    attributeAffects(aAimAxisFrom, aUpdateCache);
    attributeAffects(aAimUpAxisFrom, aUpdateCache);
    attributeAffects(aAimWorldUpFrom, aUpdateCache);
    attributeAffects(aExportAttributes, aUpdateCache);
    attributeAffects(aVelocitySource, aUpdateCache);
    attributeAffects(aAngularVelocitySource, aUpdateCache);

    attributeAffects(aLastScaleFrom, aUpdateCache);
    attributeAffects(aLastRotationFrom, aUpdateCache);
    attributeAffects(aLastAimDirectionFrom, aUpdateCache);
    attributeAffects(aLastAimPositionFrom, aUpdateCache);
    attributeAffects(aLastPositionFrom, aUpdateCache);
    attributeAffects(aVelocityFrom, aUpdateCache);

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
MStatus partioInstancer::compute(const MPlug& plug, MDataBlock& block)
{
    const MString exportAttributes = block.inputValue(aExportAttributes).asString();
    const MString velocityFrom = block.inputValue(aVelocityFrom).asString();
    const MString lastPositionFrom = block.inputValue(aLastPositionFrom).asString();

    m_drawError = 0;
    bool cacheActive = block.inputValue(aCacheActive).asBool();

    if (!cacheActive)
    {
        m_drawError = 2;
        return (MS::kSuccess);
    }

    // Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache && plug != aInstanceData)
        return (MS::kUnknownParameter);
    else
    {
        MString cacheDir = block.inputValue(aCacheDir).asString();
        MString cacheFile = block.inputValue(aCacheFile).asString();

        if (cacheDir == "" || cacheFile == "")
        {
            m_drawError = 1;
            return (MS::kFailure);
        }

        bool cacheStatic = block.inputValue(aCacheStatic).asBool();
        int cacheOffset = block.inputValue(aCacheOffset).asInt();
        short cacheFormat = block.inputValue(aCacheFormat).asShort();
        bool forceReload = block.inputValue(aForceReload).asBool();
        MTime inputTime = block.inputValue(time).asTime();
        int byFrame = block.inputValue(aByFrame).asInt();
        bool flipYZ = block.inputValue(aFlipYZ).asBool();
        MString renderCachePath = block.inputValue(aRenderCachePath).asString();
        bool computeMotionBlur = block.inputValue(aComputeVeloPos).asBool();
        float veloMult = block.inputValue(aVelocityMult).asFloat();

        float fps = static_cast<float>((MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit())));
        int integerTime = (int)floor((inputTime.value()) + .52);
        float deltaTime = inputTime.value() - (float)integerTime;

        bool motionBlurStep = false;
        // motion  blur rounding  frame logic
        if (deltaTime != 0.0f)  // motion blur step?
            motionBlurStep = true;

        MString formatExt = "";
        int cachePadding = 0;

        MString newCacheFile = "";
        MString renderCacheFile = "";

        partio4Maya::updateFileName(cacheFile, cacheDir,
                                    cacheStatic, cacheOffset,
                                    cacheFormat, integerTime, byFrame,
                                    cachePadding, formatExt,
                                    newCacheFile, renderCacheFile
        );

        if (renderCachePath != newCacheFile || renderCachePath != m_lastFileLoaded)
        {
            block.outputValue(aRenderCachePath).setString(newCacheFile);
        }
        m_cacheChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
        if (m_lastExt != formatExt || m_lastPath != cacheDir || m_lastFile != cacheFile || m_lastFlipStatus != flipYZ ||
            forceReload)
        {
            m_cacheChanged = true;
            m_flipped = false;
            m_lastFlipStatus = flipYZ;
            m_lastExt = formatExt;
            m_lastPath = cacheDir;
            m_lastFile = cacheFile;
            block.outputValue(aForceReload).setBool(false);
        }

//////////////////////////////////////////////
/// or it can change from a time input change

        if (!partio4Maya::partioCacheExists(newCacheFile.asChar()))
        {
            if (pvCache.particles != NULL)
            {
                PARTIO::ParticlesDataMutable* newParticles;
                newParticles = pvCache.particles;
                pvCache.particles = NULL; // resets the pointer

                if (newParticles != NULL)
                {
                    newParticles->release(); // frees the mem
                }
            }
            pvCache.bbox.clear();
            pvCache.instanceData.clear();
            m_lastFileLoaded = "";
            m_drawError = 1;
        }

        if (newCacheFile != "" && partio4Maya::partioCacheExists(newCacheFile.asChar()) &&
            (newCacheFile != m_lastFileLoaded || forceReload))
        {
            m_cacheChanged = true;
            m_flipped = false;
            MGlobal::displayWarning(MString("PartioInstancer->Loading: " + newCacheFile));

            if (pvCache.particles != NULL)
            {
                PARTIO::ParticlesDataMutable* newParticles;
                newParticles = pvCache.particles;
                pvCache.particles = NULL; // resets the pointer

                if (newParticles != NULL)
                {
                    newParticles->release(); // frees the mem
                }
            }
            pvCache.particles = PARTIO::read(newCacheFile.asChar());

            m_lastFileLoaded = newCacheFile;
            if (pvCache.particles->numParticles() == 0)
            {
                pvCache.instanceData.clear();
                return (MS::kSuccess);
            }

            char partCount[50];
            sprintf(partCount, "%d", pvCache.particles->numParticles());
            MGlobal::displayInfo(MString("PartioInstancer-> LOADED: ") + partCount + MString(" particles"));
            block.outputValue(aForceReload).setBool(false);
            block.setClean(aForceReload);

            pvCache.instanceData.clear();

        }

        if (pvCache.particles)
        {

            if (!pvCache.particles->attributeInfo("id", pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ID", pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("particleId", pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ParticleId", pvCache.idAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find id attribute ");
                return (MS::kFailure);
            }

            if (!pvCache.particles->attributeInfo("position", pvCache.positionAttr) &&
                !pvCache.particles->attributeInfo("Position", pvCache.positionAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find position attribute ");
                return (MS::kFailure);
            }

            // instanceData arrays
            MVectorArray positionArray;
            MDoubleArray idArray;

            // this creates or  gets an existing handles to the instanceData and then clears it to be ready to fill

            const int numParticles = pvCache.particles->numParticles();
            updateInstanceDataVector(pvCache, positionArray, "position", numParticles);
            updateInstanceDataDouble(pvCache, idArray, "id", numParticles);

            const short velocitySource = block.inputValue(aVelocitySource).asShort();

            m_canMotionBlur = false;
            if (computeMotionBlur)
            {
                if (velocitySource == VS_BUILTIN)
                {
                    if ((pvCache.particles->attributeInfo("velocity", pvCache.velocityAttr) ||
                         pvCache.particles->attributeInfo("Velocity", pvCache.velocityAttr)) ||
                        pvCache.particles->attributeInfo("V", pvCache.velocityAttr))
                    {
                        if (pvCache.velocityAttr.type == PARTIO::VECTOR)
                            m_canMotionBlur = true;
                    }
                }
                else if (velocitySource == VS_CUSTOM_CHANNEL)
                {
                    if (pvCache.particles->attributeInfo(velocityFrom.asChar(), pvCache.velocityAttr) &&
                        (pvCache.velocityAttr.type == PARTIO::VECTOR))
                        m_canMotionBlur = true;
                }
                else
                {
                    if (pvCache.particles->attributeInfo(lastPositionFrom.asChar(), pvCache.lastPosAttr) &&
                        (pvCache.lastPosAttr.type == PARTIO::VECTOR))
                        m_canMotionBlur = true;
                }

                if (!m_canMotionBlur)
                    MGlobal::displayWarning(
                            "PartioInstancer->Failed to find velocity attribute motion blur will be impaired ");
            }

            pvCache.bbox.clear();

            const float velocityMultiplier = veloMult * deltaTime / fps;

            // first we do position and ID because we need those two for sure
            for (int i = 0; i < pvCache.particles->numParticles(); ++i)
            {
                const float* partioPositions = pvCache.particles->data<float>(pvCache.positionAttr, i);
                MPoint pos(partioPositions[0], partioPositions[1], partioPositions[2]);

                if (m_canMotionBlur)
                {
                    if (motionBlurStep)
                    {
                        if (velocitySource == VS_LAST_POSITION)
                        {
                            const float* lastPos = pvCache.particles->data<float>(pvCache.lastPosAttr, i);
                            const MVector lPos(lastPos[0], lastPos[1], lastPos[2]);
                            pos += (pos - lPos) * deltaTime;
                        }
                        else
                        {
                            const float* vel = pvCache.particles->data<float>(pvCache.velocityAttr, i);
                            const MVector velo(vel[0], vel[1], vel[2]);
                            pos += velo * velocityMultiplier;
                        }
                    }
                }

                positionArray[i] = pos;

                const int* attrVal = pvCache.particles->data<int>(pvCache.idAttr, i);
                idArray[i] = (double)attrVal[0];

                // resize the bounding box
                pvCache.bbox.expand(pos);
            }

            // export custom attributes for the particle instancer
            if (motionBlurStep || m_cacheChanged ||
                m_lastExportAttributes != exportAttributes)
            {
                MStringArray attrs;

                exportAttributes.split(' ', attrs);

                const unsigned int numAttrs = attrs.length();
                for (unsigned int i = 0; i < numAttrs; ++i)
                {
                    MString attrName = attrs[i];
                    PARTIO::ParticleAttribute particleAttr;
                    if (pvCache.particles->attributeInfo(attrName.asChar(), particleAttr))
                    {
                        if (((particleAttr.type == PARTIO::VECTOR) || (particleAttr.type == PARTIO::FLOAT))
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
                        else if ((particleAttr.type == PARTIO::FLOAT) && (particleAttr.count == 1))
                        {
                            MDoubleArray arr = pvCache.instanceData.doubleArray(attrName);
                            arr.setLength(numParticles);

                            for (int p = 0; p < numParticles; ++p)
                            {
                                const float* data = pvCache.particles->data<float>(particleAttr, p);
                                arr[p] = static_cast<double>(data[0]);
                            }
                        }
                        else if ((particleAttr.type == PARTIO::INT) && (particleAttr.count == 1))
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

                m_lastExportAttributes = exportAttributes;
            }

            const MString rotationFrom = block.inputValue(aRotationFrom).asString();
            const MString lastRotFrom = block.inputValue(aLastRotationFrom).asString();
            const MString scaleFrom = block.inputValue(aScaleFrom).asString();
            const MString lastScaleFrom = block.inputValue(aLastScaleFrom).asString();
            const MString aimDirectionFrom = block.inputValue(aAimDirectionFrom).asString();
            const MString lastAimDirectionFrom = block.inputValue(aLastAimDirectionFrom).asString();
            const MString aimPositionFrom = block.inputValue(aAimPositionFrom).asString();
            const MString lastAimPositionFrom = block.inputValue(aLastAimPositionFrom).asString();
            const MString aimAxisFrom = block.inputValue(aAimAxisFrom).asString();
            const MString aimUpAxisFrom = block.inputValue(aAimUpAxisFrom).asString();
            const MString aimWorldUpFrom = block.inputValue(aAimWorldUpFrom).asString();
            const MString indexFrom = block.inputValue(aIndexFrom).asString();
            const MString angularVelocityFrom = block.inputValue(aAngularVelocityFrom).asString();

            const short angularVelocitySource = block.inputValue(aAngularVelocitySource).asShort();

            if (motionBlurStep || m_cacheChanged ||
                scaleFrom != m_lastScaleFrom ||
                rotationFrom != m_lastRotationFrom ||
                aimDirectionFrom != m_lastAimDirectionFrom ||
                aimPositionFrom != m_lastAimPositionFrom ||
                aimAxisFrom != m_lastAimAxisFrom ||
                aimUpAxisFrom != m_lastAimUpAxisFrom ||
                aimWorldUpFrom != m_lastAimWorldUpFrom ||
                indexFrom != m_lastIndexFrom ||
                lastScaleFrom != m_lastLastScaleFrom ||
                lastRotFrom != m_lastLastRotationFrom ||
                lastAimDirectionFrom != m_lastLastAimDirectionFrom ||
                lastAimPositionFrom != m_lastLastAimPositionFrom ||
                angularVelocityFrom != m_lastAngularVelocityFrom ||
                angularVelocitySource != m_lastAngularVelocitySource)
            {
                m_lastScaleFrom = scaleFrom;
                m_lastRotationFrom = rotationFrom;
                m_lastAimDirectionFrom = aimDirectionFrom;
                m_lastAimPositionFrom = aimPositionFrom;
                m_lastAimAxisFrom = aimAxisFrom;
                m_lastAimUpAxisFrom = aimUpAxisFrom;
                m_lastAimWorldUpFrom = aimWorldUpFrom;
                m_lastIndexFrom = indexFrom;
                m_lastLastScaleFrom = lastScaleFrom;
                m_lastLastRotationFrom = lastRotFrom;
                m_lastLastAimDirectionFrom = lastAimDirectionFrom;
                m_lastLastAimPositionFrom = lastAimPositionFrom;
                m_lastAngularVelocityFrom = angularVelocityFrom;
                m_lastAngularVelocitySource = angularVelocitySource;

                MDoubleArray indexArray;
                MVectorArray scaleArray;
                MVectorArray rotationArray;
                MVectorArray aimDirectionArray;
                MVectorArray aimPositionArray;
                MVectorArray aimAxisArray;
                MVectorArray aimUpAxisArray;
                MVectorArray aimWorldUpArray;

                // clear these out to update on any change
                pvCache.rotationAttr.type = PARTIO::NONE;
                pvCache.angularVelocityAttr.type = PARTIO::NONE;
                pvCache.aimDirAttr.type = PARTIO::NONE;
                pvCache.aimPosAttr.type = PARTIO::NONE;
                pvCache.aimAxisAttr.type = PARTIO::NONE;
                pvCache.aimUpAttr.type = PARTIO::NONE;
                pvCache.aimWorldUpAttr.type = PARTIO::NONE;
                pvCache.lastRotationAttr.type = PARTIO::NONE;
                pvCache.scaleAttr.type = PARTIO::NONE;
                pvCache.lastScaleAttr.type = PARTIO::NONE;
                pvCache.lastAimDirAttr.type = PARTIO::NONE;
                pvCache.lastAimPosAttr.type = PARTIO::NONE;
                pvCache.indexAttr.type = PARTIO::NONE;

                // Index
                if (pvCache.particles->attributeInfo(indexFrom.asChar(), pvCache.indexAttr))
                    updateInstanceDataDouble(pvCache, indexArray, "objectIndex", numParticles);

                // Scale
                if (pvCache.particles->attributeInfo(scaleFrom.asChar(), pvCache.scaleAttr))
                {
                    updateInstanceDataVector(pvCache, scaleArray, "scale", numParticles);
                    pvCache.particles->attributeInfo(lastScaleFrom.asChar(), pvCache.lastScaleAttr);
                }

                // Rotation
                if (pvCache.particles->attributeInfo(rotationFrom.asChar(), pvCache.rotationAttr))
                {
                    updateInstanceDataVector(pvCache, rotationArray, "rotation", numParticles);
                    if (angularVelocitySource == AVS_LAST_ROTATION)
                        pvCache.particles->attributeInfo(lastRotFrom.asChar(), pvCache.lastRotationAttr);
                    else if (angularVelocitySource == AVS_ANGULAR_VELOCITY)
                        pvCache.particles->attributeInfo(angularVelocityFrom.asChar(), pvCache.angularVelocityAttr);
                }

                // Aim Direction
                if (pvCache.particles->attributeInfo(aimDirectionFrom.asChar(), pvCache.aimDirAttr))
                {
                    updateInstanceDataVector(pvCache, aimDirectionArray, "aimDirection", numParticles);
                    pvCache.particles->attributeInfo(lastAimDirectionFrom.asChar(), pvCache.lastAimDirAttr);
                }

                // Aim Position
                if (pvCache.particles->attributeInfo(aimPositionFrom.asChar(), pvCache.aimPosAttr))
                {
                    updateInstanceDataVector(pvCache, aimPositionArray, "aimPosition", numParticles);
                    pvCache.particles->attributeInfo(lastAimPositionFrom.asChar(), pvCache.lastAimPosAttr);
                }

                // Aim Axis
                if (pvCache.particles->attributeInfo(aimAxisFrom.asChar(), pvCache.aimAxisAttr))
                    updateInstanceDataVector(pvCache, aimAxisArray, "aimAxis", numParticles);

                // Aim Up Axis
                if (pvCache.particles->attributeInfo(aimUpAxisFrom.asChar(), pvCache.aimUpAttr))
                    updateInstanceDataVector(pvCache, aimUpAxisArray, "aimUpAxis", numParticles);

                // World Up Axis
                if (pvCache.particles->attributeInfo(aimWorldUpFrom.asChar(), pvCache.aimWorldUpAttr))
                    updateInstanceDataVector(pvCache, aimWorldUpArray, "aimWorldUp", numParticles);

                const FloatTupleSize rotationTupleSize = getFloatTupleSize(pvCache.rotationAttr);
                const FloatTupleSize aimDirTupleSize = getFloatTupleSize(pvCache.aimDirAttr);
                const FloatTupleSize aimPosTupleSize = getFloatTupleSize(pvCache.aimPosAttr);
                const FloatTupleSize aimAxisTupleSize = getFloatTupleSize(pvCache.aimAxisAttr);
                const FloatTupleSize aimUpTupleSize = getFloatTupleSize(pvCache.aimUpAttr);
                const FloatTupleSize aimWorldUpTupleSize = getFloatTupleSize(pvCache.aimWorldUpAttr);
                const FloatTupleSize lastRotationTupleSize = getFloatTupleSize(pvCache.lastRotationAttr);
                const FloatTupleSize scaleAttrTupleSize = getFloatTupleSize(pvCache.scaleAttr);
                const FloatTupleSize lastScaleAttrTupleSize = getFloatTupleSize(pvCache.lastScaleAttr);
                const FloatTupleSize lastAimDirTupleSize = getFloatTupleSize(pvCache.lastAimDirAttr);
                const FloatTupleSize lastAimPosTupleSize = getFloatTupleSize(pvCache.lastAimPosAttr);
                const FloatTupleSize angularVelocityTupleSize = getFloatTupleSize(pvCache.angularVelocityAttr);

                const float angularVelocityMultiplier = deltaTime / fps;

                // TODO: break into smaller loops because this is ugly and we rely on the compiler's mercy to optimize!
                // MAIN LOOP ON PARTICLES
                for (int i = 0; i < numParticles; ++i)
                {
                    // SCALE
                    if (scaleAttrTupleSize == FLOAT_TUPLE_SINGLE)  // single float value for scale
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.scaleAttr, i);
                        float scale = attrVal[0];
                        if (m_canMotionBlur && lastScaleAttrTupleSize == FLOAT_TUPLE_SINGLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr, i);
                            scale += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                        }
                        scaleArray[i] = MVector(scale, scale, scale);
                    }
                    else if (scaleAttrTupleSize == FLOAT_TUPLE_TRIPLE)   // we have a 4 float attribute ?
                    {

                        const float* attrVal = pvCache.particles->data<float>(pvCache.scaleAttr, i);
                        MVector scale = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        if (m_canMotionBlur && lastScaleAttrTupleSize == FLOAT_TUPLE_TRIPLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr, i);
                            scale.x += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                            scale.y += (attrVal[1] - lastAttrVal[1]) * deltaTime;
                            scale.z += (attrVal[2] - lastAttrVal[2]) * deltaTime;
                        }
                        scaleArray[i] = scale;
                    }
                    // ROTATION
                    if (rotationTupleSize == FLOAT_TUPLE_SINGLE)  // single float value for rotation
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.rotationAttr, i);
                        float rot = attrVal[0];
                        if (m_canMotionBlur && lastRotationTupleSize == FLOAT_TUPLE_SINGLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr, i);
                            rot += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                        }
                        rotationArray[i] = MVector(rot, rot, rot);
                    }
                    else if (rotationTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.rotationAttr, i);
                        MVector rot = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        if (m_canMotionBlur)
                        {
                            if (angularVelocitySource == AVS_LAST_ROTATION && lastRotationTupleSize == FLOAT_TUPLE_TRIPLE)
                            {
                                const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr, i);
                                rot.x += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                                rot.y += (attrVal[1] - lastAttrVal[1]) * deltaTime;
                                rot.z += (attrVal[2] - lastAttrVal[2]) * deltaTime;
                            }
                            else if (angularVelocitySource == AVS_ANGULAR_VELOCITY && angularVelocityTupleSize == FLOAT_TUPLE_TRIPLE)
                            {
                                const float* lastAttrVal = pvCache.particles->data<float>(pvCache.angularVelocityAttr, i);
                                rot.x += attrVal[0] * angularVelocityMultiplier;
                                rot.y += attrVal[1] * angularVelocityMultiplier;
                                rot.z += attrVal[2] * angularVelocityMultiplier;
                            }
                        }
                        rotationArray[i] = rot;
                    }
                    else if (rotationTupleSize == FLOAT_TUPLE_QUAD)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.rotationAttr, i);
                        MQuaternion rotQ(attrVal[0], attrVal[1], attrVal[2], attrVal[3]);
                        if (m_canMotionBlur)
                        {
                            if (angularVelocitySource == AVS_LAST_ROTATION && lastRotationTupleSize == FLOAT_TUPLE_QUAD)
                            {
                                const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr, i);
                                MQuaternion lastRotQ(lastAttrVal[0], lastAttrVal[1], lastAttrVal[2], lastAttrVal[3]);
                                //slerp has input params between 0 and 1, while our delta time is -1 .. 1
#ifndef USE_SLERP_FOR_QUATERIONS
                                rotQ = rotQ + deltaTime * (rotQ - lastRotQ);
#else
                                if (deltaTime < 0.0f)
                                    rotQ = slerp(lastRotQ, rotQ, 1.0 - deltaTime);
                                else
                                    rotQ = slerp(rotQ, rotQ + rotQ - lastRotQ, deltaTime);
#endif
                                rotationArray[i] = rotQ.asEulerRotation().asVector();
                            }
                            // angular velocity is always eulers ATM
                            else if (angularVelocitySource == AVS_ANGULAR_VELOCITY && angularVelocityTupleSize == FLOAT_TUPLE_TRIPLE)
                            {
                                MVector rot = rotQ.asEulerRotation().asVector();
                                const float* attrVal = pvCache.particles->data<float>(pvCache.angularVelocityAttr, i);
                                rot.x += attrVal[0] * angularVelocityMultiplier;
                                rot.y += attrVal[1] * angularVelocityMultiplier;
                                rot.z += attrVal[2] * angularVelocityMultiplier;
                                rotationArray[i] = rot;
                            }
                            else
                                rotationArray[i] = rotQ.asEulerRotation().asVector();
                        }
                        else
                            rotationArray[i] = rotQ.asEulerRotation().asVector();
                    }

                    // AIM DIRECTION
                    if (aimDirTupleSize == FLOAT_TUPLE_SINGLE)  // single float value for aimDirection
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr, i);
                        float aimDir = attrVal[0];
                        if (m_canMotionBlur && lastAimDirTupleSize == FLOAT_TUPLE_SINGLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr, i);
                            aimDir += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                        }
                        aimDirectionArray[i] = MVector(aimDir, aimDir, aimDir);
                    }
                    else if (aimDirTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr, i);
                        MVector aimDir = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        if (m_canMotionBlur && lastAimDirTupleSize == FLOAT_TUPLE_SINGLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr, i);
                            aimDir.x += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                            aimDir.y += (attrVal[1] - lastAttrVal[1]) * deltaTime;
                            aimDir.z += (attrVal[2] - lastAttrVal[2]) * deltaTime;
                            // TODO: figure out why this is not working on subframes correctly
                        }
                        aimDirectionArray[i] = aimDir;
                    }
                    // AIM POSITION
                    if (aimPosTupleSize == FLOAT_TUPLE_SINGLE) // single float value for aimDirection
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr, i);
                        float aimPos = attrVal[0];
                        if (m_canMotionBlur && lastAimPosTupleSize == FLOAT_TUPLE_SINGLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr, i);
                            aimPos += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                        }
                        aimPositionArray[i] = MVector(aimPos, aimPos, aimPos);
                    }
                    else if (aimPosTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr, i);
                        MVector aimPos = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        if (m_canMotionBlur && lastAimPosTupleSize == FLOAT_TUPLE_TRIPLE)
                        {
                            const float* lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr, i);
                            aimPos.x += (attrVal[0] - lastAttrVal[0]) * deltaTime;
                            aimPos.y += (attrVal[1] - lastAttrVal[1]) * deltaTime;
                            aimPos.z += (attrVal[2] - lastAttrVal[2]) * deltaTime;
                        }
                        aimPositionArray[i] = aimPos;
                    }
                    // AIM Axis
                    if (aimAxisTupleSize == FLOAT_TUPLE_SINGLE) // single float value for aimDirection
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr, i);
                        float aimAxis = attrVal[0];
                        aimAxisArray[i] = MVector(aimAxis, aimAxis, aimAxis);
                    }
                    else if (aimAxisTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr, i);
                        MVector aimAxis = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        aimAxisArray[i] = aimAxis;
                    }

                    // AIM Up Axis
                    if (aimUpTupleSize == FLOAT_TUPLE_SINGLE) // single float value for aimDirection
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr, i);
                        float aimUp = attrVal[0];
                        aimUpAxisArray[i] = MVector(aimUp, aimUp, aimUp);
                    }
                    else if (aimUpTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr, i);
                        MVector aimUp = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        aimUpAxisArray[i] = aimUp;
                    }
                    // World Up Axis
                    if (aimWorldUpTupleSize == FLOAT_TUPLE_SINGLE) // single float value for aimDirection
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr, i);
                        float worldUp = attrVal[0];
                        aimWorldUpArray[i] = MVector(worldUp, worldUp, worldUp);
                    }
                    else if (aimWorldUpTupleSize == FLOAT_TUPLE_TRIPLE)
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr, i);
                        MVector worldUp = MVector(attrVal[0], attrVal[1], attrVal[2]);
                        aimWorldUpArray[i] = worldUp;
                    }
                    // INDEX
                    if (pvCache.indexAttr.type == PARTIO::FLOAT ||
                        pvCache.indexAttr.type == PARTIO::INT)  // single float value for index
                    {
                        if (pvCache.indexAttr.type == PARTIO::FLOAT)
                        {
                            const float* attrVal = pvCache.particles->data<float>(pvCache.indexAttr, i);
                            indexArray[i] = (double)(int)attrVal[0];
                        }
                        else if (pvCache.indexAttr.type == PARTIO::INT)
                        {
                            const int* attrVal = pvCache.particles->data<int>(pvCache.indexAttr, i);
                            indexArray[i] = (double)attrVal[0];
                        }
                    }
                    else if (pvCache.indexAttr.type == PARTIO::VECTOR)   // we have a 3or4 float attribute
                    {
                        const float* attrVal = pvCache.particles->data<float>(pvCache.indexAttr, i);
                        indexArray[i] = (double)(int)attrVal[0];
                    }
                } // end frame loop

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
        unsigned int numAttr = pvCache.particles->numAttributes();
        MPlug zPlug(thisMObject(), aPartioAttributes);

        // no need to reset attributes, again invalid attributes are fine, as they might
        // not be present on all frames

        if (m_cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
        {
            //cout << "partioInstancer->refreshing AE controls" << endl;

            m_attributeList.clear();

            for (unsigned int i = 0; i < numAttr; i++)
            {
                PARTIO::ParticleAttribute attr;
                pvCache.particles->attributeInfo(i, attr);

                // crazy casting string to  char
                char* temp;
                temp = new char[(attr.name).length() + 1];
                strcpy(temp, attr.name.c_str());

                MString mStringAttrName("");
                mStringAttrName += MString(temp);

                zPlug.selectAncestorLogicalIndex(i, aPartioAttributes);
                zPlug.setValue(MString(temp));
                m_attributeList.append(mStringAttrName);

                delete[] temp;
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
    return MBoundingBox(corner1, corner2);
}

//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioInstancerUI::select(MSelectInfo& selectInfo,
                               MSelectionList& selectionList,
                               MPointArray& worldSpaceSelectPts) const
{
    MSelectionMask priorityMask(MSelectionMask::kSelectObjectsMask);
    MSelectionList item;
    item.add(selectInfo.selectPath());
    MPoint xformedPt;
    selectInfo.addSelection(item, xformedPt, selectionList,
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
    updatePlug.getValue(update);
    if (update != m_dUpdate)
    {
        m_dUpdate = update;
        return true;
    }
    else
    {
        return false;
    }
    return false;

}

// note the "const" at the end, its different than other draw calls
void partioInstancerUI::draw(const MDrawRequest& request, M3dView& view) const
{
    MDrawData data = request.drawData();

    partioInstancer* shapeNode = (partioInstancer*)surfaceShape();

    partioInstReaderCache* cache = (partioInstReaderCache*)data.geometry();

    MObject thisNode = shapeNode->thisMObject();
    MPlug sizePlug(thisNode, shapeNode->aSize);
    MDistance sizeVal;
    sizePlug.getValue(sizeVal);

    shapeNode->m_multiplier = (float)sizeVal.asCentimeters();

    int drawStyle = 0;
    MPlug drawStylePlug(thisNode, shapeNode->aDrawStyle);
    drawStylePlug.getValue(drawStyle);

    view.beginGL();

    if (drawStyle < 3 && view.displayStyle() != M3dView::kBoundingBox)
    {
        drawPartio(cache, drawStyle, view);
    }
    else
    {
        drawBoundingBox();
    }

    if (shapeNode->m_drawError == 1)
    {
        glColor3f(.75f, 0.0f, 0.0f);
    }
    else if (shapeNode->m_drawError == 2)
    {
        glColor3f(0.0f, 0.0f, 0.0f);
    }

    partio4Maya::drawPartioLogo(shapeNode->m_multiplier);

    view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioInstancerUI::drawBoundingBox() const
{

    partioInstancer* shapeNode = (partioInstancer*)surfaceShape();

    MPoint bboxMin = shapeNode->pvCache.bbox.min();
    MPoint bboxMax = shapeNode->pvCache.bbox.max();

    float xMin = float(bboxMin.x);
    float yMin = float(bboxMin.y);
    float zMin = float(bboxMin.z);
    float xMax = float(bboxMax.x);
    float yMax = float(bboxMax.y);
    float zMax = float(bboxMax.z);

    /// draw the bounding box
    glBegin(GL_LINES);

    glColor3f(1.0f, 0.5f, 0.5f);

    glVertex3f(xMin, yMin, zMax);
    glVertex3f(xMax, yMin, zMax);

    glVertex3f(xMin, yMin, zMin);
    glVertex3f(xMax, yMin, zMin);

    glVertex3f(xMin, yMin, zMax);
    glVertex3f(xMin, yMin, zMin);

    glVertex3f(xMax, yMin, zMax);
    glVertex3f(xMax, yMin, zMin);

    glVertex3f(xMin, yMax, zMin);
    glVertex3f(xMin, yMax, zMax);

    glVertex3f(xMax, yMax, zMax);
    glVertex3f(xMax, yMax, zMin);

    glVertex3f(xMin, yMax, zMax);
    glVertex3f(xMax, yMax, zMax);

    glVertex3f(xMin, yMax, zMin);
    glVertex3f(xMax, yMax, zMin);


    glVertex3f(xMin, yMax, zMin);
    glVertex3f(xMin, yMin, zMin);

    glVertex3f(xMax, yMax, zMin);
    glVertex3f(xMax, yMin, zMin);

    glVertex3f(xMin, yMax, zMax);
    glVertex3f(xMin, yMin, zMax);

    glVertex3f(xMax, yMax, zMax);
    glVertex3f(xMax, yMin, zMax);

    glEnd();
}

////////////////////////////////////////////
/// DRAW PARTIO

void partioInstancerUI::drawPartio(partioInstReaderCache* pvCache, int drawStyle, M3dView& view) const
{
    partioInstancer* shapeNode = (partioInstancer*)surfaceShape();

    MObject thisNode = shapeNode->thisMObject();
    //  MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ);
    //  bool flipYZVal;
    //  flipYZPlug.getValue( flipYZVal);

    MPlug pointSizePlug(thisNode, shapeNode->aPointSize);
    float pointSizeVal;
    pointSizePlug.getValue(pointSizeVal);

    if (pvCache->particles)
    {
        glPushAttrib(GL_CURRENT_BIT);

        /// looping thru particles one by one...
        glPointSize(pointSizeVal);
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_POINTS);

        MVectorArray positions = pvCache->instanceData.vectorArray("position");
        for (unsigned int i = 0; i < positions.length(); i++)
        {
            glVertex3f(positions[i].x, positions[i].y, positions[i].z);
        }

        glEnd();
        glDisable(GL_POINT_SMOOTH);

        if (drawStyle == DRAW_STYLE_LABEL)
        {
            glColor3f(0.0, 0.0, 0.0);
            for (unsigned int i = 0; i < positions.length(); i++)
            {
                MString idVal;
                if (pvCache->indexAttr.type == PARTIO::FLOAT)
                {
                    const float* attrVal = pvCache->particles->data<float>(pvCache->indexAttr, i);
                    idVal = (double)(int)attrVal[0];
                }
                else if (pvCache->indexAttr.type == PARTIO::INT)
                {
                    const int* attrVal = pvCache->particles->data<int>(pvCache->indexAttr, i);
                    idVal = (double)attrVal[0];
                }
                else if (pvCache->indexAttr.type == PARTIO::VECTOR)
                {
                    const float* attrVal = pvCache->particles->data<float>(pvCache->indexAttr, i);
                    char idString[100];
                    sprintf(idString, "(%.2f,%.2f,%.2f)", (double)attrVal[0], (double)attrVal[1], (double)attrVal[2]);
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

void partioInstancerUI::getDrawRequests(const MDrawInfo& info,
                                        bool /*objectAndActiveOnly*/, MDrawRequestQueue& queue)
{
    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioInstancer* shapeNode = (partioInstancer*)surfaceShape();
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
