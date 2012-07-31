/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Visualizer
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
#include <vector>

#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPointArray.h>
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MString.h>
#include <maya/MTypes.h>

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxNode.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTime.h>
#include <maya/MGlobal.h>

#include "partioVisualizer.h"
#include "partio4MayaShared.h"
#include "iconArrays.h"
#include <set>

#define _USE_MGL_FT_
#include <maya/MGLFunctionTable.h>
static MGLFunctionTable *gGLFT = NULL;


// id is registered with autodesk no need to change
#define ID_PARTIOVISUALIZER  0x00116ECF

#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue

using namespace Partio;
using namespace std;

/// ///////////////////////////////////////////////////
/// PARTIO VISUALIZER

MTypeId partioVisualizer::id( ID_PARTIOVISUALIZER );

MObject partioVisualizer::time;
MObject partioVisualizer::aDrawSkip;
MObject partioVisualizer::aUpdateCache;
MObject partioVisualizer::aSize;         // The size of the logo
MObject partioVisualizer::aFlipYZ;
MObject partioVisualizer::aCacheDir;
MObject partioVisualizer::aCacheFile;
MObject partioVisualizer::aUseTransform;
MObject partioVisualizer::aCacheActive;
MObject partioVisualizer::aCacheOffset;
MObject partioVisualizer::aCacheStatic;
MObject partioVisualizer::aCacheFormat;
MObject partioVisualizer::aJitterPos;
MObject partioVisualizer::aJitterFreq;
MObject partioVisualizer::aPartioAttributes;
MObject partioVisualizer::aColorFrom;
MObject partioVisualizer::aRadiusFrom;
MObject partioVisualizer::aAlphaFrom;
MObject partioVisualizer::aPointSize;
MObject partioVisualizer::aDefaultPointColor;
MObject partioVisualizer::aDefaultAlpha;
MObject partioVisualizer::aInvertAlpha;
MObject partioVisualizer::aDrawStyle;
MObject partioVisualizer::aForceReload;
MObject partioVisualizer::aRenderCachePath;


MCallbackId partioVisualizerOpenCallback;
MCallbackId partioVisualizerImportCallback;
MCallbackId partioVisualizerReferenceCallback;

partioVizReaderCache::partioVizReaderCache():
	token(0),
	bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
	dList(0),
	particles(NULL),
	rgb(NULL),
	rgba(NULL),
	flipPos(NULL)
{
}


/// CREATOR
partioVisualizer::partioVisualizer()
:   mLastFileLoaded(""),
	mLastAlpha(0.0),
	mLastInvertAlpha(false),
	mLastPath(""),
	mLastFile(""),
	mLastExt(""),
	mLastColorFromIndex(-1),
	mLastAlphaFromIndex(-1),
	mLastRadiusFromIndex(-1),
	mLastColor(1,0,0),
	cacheChanged(false),
	frameChanged(false),
	multiplier(1.0),
	mFlipped(false),
	drawError(false)
{
	pvCache.particles = NULL;
	pvCache.flipPos = (float *) malloc(sizeof(float));
	pvCache.rgb = (float *) malloc (sizeof(float));
	pvCache.rgba = (float *) malloc(sizeof(float));

}
/// DESTRUCTOR
partioVisualizer::~partioVisualizer()
{

	if (pvCache.particles)
	{
		pvCache.particles->release();
	}
	free(pvCache.flipPos);
	free(pvCache.rgb);
	free(pvCache.rgba);
	MSceneMessage::removeCallback( partioVisualizerOpenCallback);
    MSceneMessage::removeCallback( partioVisualizerImportCallback);
    MSceneMessage::removeCallback( partioVisualizerReferenceCallback);

}

void* partioVisualizer::creator()
{
	return new partioVisualizer;
}

/// POST CONSTRUCTOR
void partioVisualizer::postConstructor()
{
	setRenderable(true);
	partioVisualizerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioVisualizer::reInit, this);
    partioVisualizerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioVisualizer::reInit, this);
    partioVisualizerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioVisualizer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioVisualizer::initCallback()
{

    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    mLastExt = partio4Maya::setExt(extENum);

    MPlug(tmo,aCacheDir).getValue(mLastPath);
    MPlug(tmo,aCacheFile).getValue(mLastFile);
	MPlug(tmo,aDefaultAlpha).getValue(mLastAlpha);
	MPlug(tmo,aColorFrom).getValue(mLastColorFromIndex);
	MPlug(tmo,aAlphaFrom).getValue(mLastAlphaFromIndex);
	MPlug(tmo,aRadiusFrom).getValue(mLastRadiusFromIndex);
	MPlug(tmo,aSize).getValue(multiplier);
	MPlug(tmo,aInvertAlpha).getValue(mLastInvertAlpha);
	MPlug(tmo,aCacheStatic).getValue(mLastStatic);
	cacheChanged = false;

}

void partioVisualizer::reInit(void *data)
{
    partioVisualizer  *vizNode = (partioVisualizer*) data;
    vizNode->initCallback();
}


MStatus partioVisualizer::initialize()
{

	MFnEnumAttribute 	eAttr;
	MFnUnitAttribute 	uAttr;
	MFnNumericAttribute nAttr;
	MFnTypedAttribute 	tAttr;
	MStatus			 	stat;

	time = nAttr.create( "time", "tm", MFnNumericData::kLong ,0);
	uAttr.setKeyable( true );

	aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
	uAttr.setDefault( 0.25 );

	aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
	nAttr.setDefault ( false );
	nAttr.setKeyable ( true );

	aDrawSkip = nAttr.create( "drawSkip", "dsk", MFnNumericData::kLong ,0);
	nAttr.setKeyable( true );
	nAttr.setReadable( true );
	nAttr.setWritable( true );
	nAttr.setConnectable( true );
	nAttr.setStorable( true );
	nAttr.setMin(0);
	nAttr.setMax(1000);

	aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &stat );
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
		eAttr.addField(formatExtMap[i].toUpperCase(),	i);
	}

    eAttr.setDefault(4);  // PDC
    eAttr.setChannelBox(true);

	aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
    eAttr.addField("points",	0);
    //eAttr.addField("velocity", 1);
    //eAttr.addField("spheres",	2);
	eAttr.addField("boundingBox", 3);
	eAttr.setDefault(0);
	eAttr.setChannelBox(true);


    aUseTransform = nAttr.create("useTransform", "utxfm", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);

	aColorFrom = nAttr.create("colorFrom", "cfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);

	aAlphaFrom = nAttr.create("opacityFrom", "ofrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);

	aRadiusFrom = nAttr.create("radiusFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
	nAttr.setKeyable(true);

	aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
	nAttr.setDefault(2);
	nAttr.setKeyable(true);

	aDefaultPointColor = nAttr.createColor("defaultPointColor", "dpc", &stat);
	nAttr.setDefault(1.0f, 0.0f, 0.0f);
	nAttr.setKeyable(true);

	aDefaultAlpha = nAttr.create("defaultAlpha", "dalf", MFnNumericData::kFloat, 1.0, &stat);
	nAttr.setDefault(1.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	nAttr.setKeyable(true);

	aInvertAlpha= nAttr.create("invertAlpha", "ialph", MFnNumericData::kBoolean, false, &stat);
	nAttr.setDefault(false);
	nAttr.setKeyable(true);

	aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
	nAttr.setDefault(false);
	nAttr.setKeyable(false);

	aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
	nAttr.setHidden(true);

	aRenderCachePath = tAttr.create ( "renderCachePath", "rcp", MFnStringData::kString );
	nAttr.setHidden(true);


	addAttribute ( aUpdateCache );
	addAttribute ( aSize );
	addAttribute ( aFlipYZ );
	addAttribute ( aDrawSkip );
	addAttribute ( aCacheDir );
    addAttribute ( aCacheFile );
    addAttribute ( aCacheOffset );
	addAttribute ( aCacheStatic );
    addAttribute ( aCacheActive );
    addAttribute ( aCacheFormat );
    addAttribute ( aPartioAttributes );
	addAttribute ( aColorFrom );
	addAttribute ( aAlphaFrom );
	addAttribute ( aRadiusFrom );
	addAttribute ( aPointSize );
	addAttribute ( aDefaultPointColor );
	addAttribute ( aDefaultAlpha );
	addAttribute ( aInvertAlpha );
	addAttribute ( aDrawStyle );
	addAttribute ( aForceReload );
	addAttribute ( aRenderCachePath );
	addAttribute ( time );

    attributeAffects ( aCacheDir, aUpdateCache );
	attributeAffects ( aSize, aUpdateCache );
	attributeAffects ( aFlipYZ, aUpdateCache );
    attributeAffects ( aCacheFile, aUpdateCache );
    attributeAffects ( aCacheOffset, aUpdateCache );
	attributeAffects ( aCacheStatic, aUpdateCache );
    attributeAffects ( aCacheFormat, aUpdateCache );
	attributeAffects ( aColorFrom, aUpdateCache );
	attributeAffects ( aAlphaFrom, aUpdateCache );
	attributeAffects ( aRadiusFrom, aUpdateCache );
	attributeAffects ( aPointSize, aUpdateCache );
	attributeAffects ( aDefaultPointColor, aUpdateCache );
	attributeAffects ( aDefaultAlpha, aUpdateCache );
	attributeAffects ( aInvertAlpha, aUpdateCache );
	attributeAffects ( aDrawStyle, aUpdateCache );
	attributeAffects ( aForceReload, aUpdateCache );
	attributeAffects (time, aUpdateCache);
	attributeAffects (time,aRenderCachePath);


	return MS::kSuccess;
}


partioVizReaderCache* partioVisualizer::updateParticleCache()
{
	GetPlugData(); // force update to run compute function where we want to do all the work
    return &pvCache;
}

// COMPUTE FUNCTION

MStatus partioVisualizer::compute( const MPlug& plug, MDataBlock& block )
{


	int colorFromIndex  = block.inputValue( aColorFrom ).asInt();
	int opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
	bool cacheActive = block.inputValue(aCacheActive).asBool();

    if (!cacheActive)
    {
        return ( MS::kSuccess );
    }

	// Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache)
	{
        return ( MS::kUnknownParameter );
	}

	else
	{
		MStatus stat;

		MString cacheDir 	= block.inputValue(aCacheDir).asString();
		MString cacheFile = block.inputValue(aCacheFile).asString();

		drawError = false;
		if (cacheDir  == "" || cacheFile == "" )
		{
			drawError = true;
			// too much printing  rather force draw of icon to red or something
			//MGlobal::displayError("PartioVisualizer->Error: Please specify cache file!");
			return ( MS::kFailure );
		}

		bool cacheStatic			= block.inputValue( aCacheStatic ).asBool();
		int cacheOffset 			= block.inputValue( aCacheOffset ).asInt();
		short cacheFormat			= block.inputValue( aCacheFormat ).asShort();
		MFloatVector defaultColor 	= block.inputValue( aDefaultPointColor ).asFloatVector();
		float defaultAlpha 			= block.inputValue( aDefaultAlpha ).asFloat();
		bool invertAlpha 			= block.inputValue( aInvertAlpha ).asBool();
		bool forceReload 			= block.inputValue( aForceReload ).asBool();
		int integerTime				= block.inputValue(time).asInt();
		bool flipYZ 				= block.inputValue( aFlipYZ ).asBool();
		MString renderCachePath 	= block.inputValue( aRenderCachePath ).asString();

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

		cout << newCacheFile << endl;

		if (renderCachePath != renderCacheFile || forceReload )
		{
			block.outputValue(aRenderCachePath).setString(renderCacheFile);
		}

		cacheChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
		if (mLastExt != formatExt ||
			mLastPath != cacheDir ||
			mLastFile != cacheFile ||
			mLastFlipStatus  != flipYZ ||
			mLastStatic !=  cacheStatic ||
			forceReload )
		{
			cacheChanged = true;
			mFlipped = false;
			mLastFlipStatus = flipYZ;
			mLastExt = formatExt;
			mLastPath = cacheDir;
			mLastFile = cacheFile;
			mLastStatic = cacheStatic;
			block.outputValue(aForceReload).setBool(false);
		}

//////////////////////////////////////////////
/// or it can change from a time input change

		if(!partio4Maya::partioCacheExists(newCacheFile.asChar()))
		{
			pvCache.particles=0; // resets the particles
			pvCache.bbox.clear();
		}

		if ( newCacheFile != "" &&
			 partio4Maya::partioCacheExists(newCacheFile.asChar()) &&
			 (newCacheFile != mLastFileLoaded || forceReload)
		   )
		{

			cacheChanged = true;
			mFlipped = false;
			MGlobal::displayWarning(MString("PartioVisualizer->Loading: " + newCacheFile));
			pvCache.particles=0; // resets the particles

			pvCache.particles=read(newCacheFile.asChar());

			mLastFileLoaded = newCacheFile;
			if (pvCache.particles->numParticles() == 0)
			{
				return (MS::kSuccess);
			}

			char partCount[50];
			sprintf (partCount, "%d", pvCache.particles->numParticles());
			MGlobal::displayInfo(MString ("PartioVisualizer-> LOADED: ") + partCount + MString (" particles"));


			float * floatToRGB = (float *) realloc(pvCache.rgb, pvCache.particles->numParticles()*sizeof(float)*3);
			if (floatToRGB != NULL)
			{
				pvCache.rgb =  floatToRGB;
			}
			else
			{
				free(pvCache.rgb);
				MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
				return (MS::kFailure);
			}

			float * newRGBA = (float *) realloc(pvCache.rgba,pvCache.particles->numParticles()*sizeof(float)*4);
			if (newRGBA != NULL)
			{
				pvCache.rgba =  newRGBA;
			}
			else
			{
				free(pvCache.rgba);
				MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
				return (MS::kFailure);
			}

			pvCache.bbox.clear();

			if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
				!pvCache.particles->attributeInfo("Position",pvCache.positionAttr))
			{
				MGlobal::displayError("PartioVisualizer->Failed to find position attribute ");
				return ( MS::kFailure );
			}
			else
			{
				// resize the bounding box
				for (int i=0;i<pvCache.particles->numParticles();i++)
				{
					const float * partioPositions = pvCache.particles->data<float>(pvCache.positionAttr,i);
					MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
					pvCache.bbox.expand(pos);
				}
			}

			block.outputValue(aForceReload).setBool(false);
			block.setClean(aForceReload);

		}

		if (pvCache.particles)
		{
			/// TODO:  this does not work when scrubbing yet.. really need to put the  resort of channels into the  partio side as a filter
			/// this is only a temporary hack until we start adding  filter functions to partio

			// only flip the axis stuff if we need to
			if ( cacheChanged && flipYZ  && !mFlipped )
			{
				float * floatToPos = (float *) realloc(pvCache.flipPos, pvCache.particles->numParticles()*sizeof(float)*3);
				if (floatToPos != NULL)
				{
					pvCache.flipPos =  floatToPos;
				}
				else
				{
					free(pvCache.flipPos);
					MGlobal::displayError("PartioVisualizer->unable to allocate new memory for flip particles");
					return (MS::kFailure);
				}

				for (int i=0;i<pvCache.particles->numParticles();i++)
				{
					const float * attrVal = pvCache.particles->data<float>(pvCache.positionAttr,i);
					pvCache.flipPos[(i*3)] 		= attrVal[0];
					pvCache.flipPos[((i*3)+1)] 	= -attrVal[2];
					pvCache.flipPos[((i*3)+2)] 	= attrVal[1];
				}
				mFlipped = true;
			}

			// something changed..
			if  (cacheChanged || mLastColorFromIndex != colorFromIndex || mLastColor != defaultColor)
			{
				int numAttrs = pvCache.particles->numAttributes();
				if (colorFromIndex+1 > numAttrs || opacityFromIndex+1 > numAttrs)
				{
					// reset the attrs
					block.outputValue(aColorFrom).setInt(-1);
					block.setClean(aColorFrom);
					block.outputValue(aAlphaFrom).setInt(-1);
					block.setClean(aAlphaFrom);

					colorFromIndex  = block.inputValue( aColorFrom ).asInt();
					opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
				}

				if(colorFromIndex >=0)
				{
					pvCache.particles->attributeInfo(colorFromIndex,pvCache.colorAttr);
					// VECTOR or  4+ element float attrs
					if (pvCache.colorAttr.type == VECTOR || pvCache.colorAttr.count > 3) // assuming first 3 elements are rgb
					{
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);
							pvCache.rgb[(i*3)] 		= attrVal[0];
							pvCache.rgb[((i*3)+1)] 	= attrVal[1];
							pvCache.rgb[((i*3)+2)] 	= attrVal[2];
							pvCache.rgba[i*4] 		= attrVal[0];
							pvCache.rgba[(i*4)+1] 	= attrVal[1];
							pvCache.rgba[(i*4)+2] 	= attrVal[2];
						}
					}
					else // single FLOAT
					{
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr,i);

							pvCache.rgb[(i*3)] = pvCache.rgb[((i*3)+1)] = pvCache.rgb[((i*3)+2)] 	= attrVal[0];
							pvCache.rgba[i*4]  = pvCache.rgba[(i*4)+1] 	= pvCache.rgba[(i*4)+2] 	= attrVal[0];
						}
					}
				}

				else
				{
					for (int i=0;i<pvCache.particles->numParticles();i++)
					{
						pvCache.rgb[(i*3)] 		= defaultColor[0];
						pvCache.rgb[((i*3)+1)] 	= defaultColor[1];
						pvCache.rgb[((i*3)+2)] 	= defaultColor[2];
						pvCache.rgba[i*4] 		= defaultColor[0];
						pvCache.rgba[(i*4)+1] 	= defaultColor[1];
						pvCache.rgba[(i*4)+2] 	= defaultColor[2];
					}

				}
				mLastColorFromIndex = colorFromIndex;
				mLastColor = defaultColor;
			}

			if  (cacheChanged || opacityFromIndex != mLastAlphaFromIndex || defaultAlpha != mLastAlpha || invertAlpha != mLastInvertAlpha)
			{
				if (opacityFromIndex >=0)
				{
					pvCache.particles->attributeInfo(opacityFromIndex,pvCache.opacityAttr);
					if (pvCache.opacityAttr.count == 1)  // single float value for opacity
					{
						for (int i=0;i<pvCache.particles->numParticles();i++)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
							float temp = attrVal[0];
							if (invertAlpha)
							{
								temp = float(1.0-temp);

							}
							pvCache.rgba[(i*4)+3] = temp;
						}
					}
					else
					{
						if (pvCache.opacityAttr.count == 4)   // we have an  RGBA 4 float attribute ?
						{
							for (int i=0;i<pvCache.particles->numParticles();i++)
							{
								const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
								float temp = attrVal[3];
								if (invertAlpha)
								{
									temp = float(1.0-temp);
								}
								pvCache.rgba[(i*4)+3] = temp;
							}
						}
						else
						{
							for (int i=0;i<pvCache.particles->numParticles();i++)
							{
								const float * attrVal = pvCache.particles->data<float>(pvCache.opacityAttr,i);
								float lum = float((attrVal[0]*0.2126)+(attrVal[1]*0.7152)+(attrVal[2]*.0722));
								if (invertAlpha)
								{
									lum = float(1.0-lum);
 								}
								pvCache.rgba[(i*4)+3] =  lum;
							}
						}
					}
				}
				else
				{
					mLastAlpha = defaultAlpha;
					if (invertAlpha)
					{
						mLastAlpha= 1-defaultAlpha;
					}
					for (int i=0;i<pvCache.particles->numParticles();i++)
					{
						pvCache.rgba[(i*4)+3] = mLastAlpha;
					}
				}
				mLastAlpha = defaultAlpha;
				mLastAlphaFromIndex =opacityFromIndex;
				mLastInvertAlpha = invertAlpha;
			}
		}
	}


	if (pvCache.particles) // update the AE Controls for attrs in the cache
	{
		unsigned int numAttr=pvCache.particles->numAttributes();
		MPlug zPlug (thisMObject(), aPartioAttributes);

		if ((colorFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aColorFrom).setInt(-1);
		}
		if ((opacityFromIndex+1) > zPlug.numElements())
		{
			block.outputValue(aAlphaFrom).setInt(-1);
		}

		if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
		{
			MString command = "";
			MString zPlugName = zPlug.name();
			for (unsigned int x = 0; x<zPlug.numElements(); x++)
			{
				command += "removeMultiInstance -b true ";
				command += zPlugName;
				command += "[";
				command += x;
				command += "]";
				command += ";";
			}

			MGlobal::executeCommand(command);
			zPlug.setNumElements(0);

			//cout << "partioVisualizer->refreshing AE controls" << endl;

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
		}
	}
	block.setClean(plug);
	return MS::kSuccess;
}


/////////////////////////////////////////////////////
/// procs to override bounding box mode...
bool partioVisualizer::isBounded() const
{
    return true;
}

MBoundingBox partioVisualizer::boundingBox() const
{
	// Returns the bounding box for the shape.
    partioVisualizer* nonConstThis = const_cast<partioVisualizer*>(this);
    partioVizReaderCache* geom = nonConstThis->updateParticleCache();


	MPoint corner1 = geom->bbox.min();
	MPoint corner2 = geom->bbox.max();
	return MBoundingBox( corner1, corner2 );
}

//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioVisualizerUI::select( MSelectInfo &selectInfo,
							 MSelectionList &selectionList,
							 MPointArray &worldSpaceSelectPts ) const
{
	MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
	MSelectionList item;
	item.add( selectInfo.selectPath() );
	MPoint xformedPt;
	selectInfo.addSelection( item, xformedPt, selectionList,
							 worldSpaceSelectPts, priorityMask, false );
	return true;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool partioVisualizer::GetPlugData()
{
	MObject thisNode = thisMObject();
	int update = 0;
	MPlug updatePlug(thisNode, aUpdateCache );
	updatePlug.getValue( update );
	if(update != dUpdate)
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
void partioVisualizerUI::draw( const MDrawRequest& request, M3dView& view ) const
{
	MDrawData data = request.drawData();

	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

	partioVizReaderCache* cache = (partioVizReaderCache*) data.geometry();

	MObject thisNode = shapeNode->thisMObject();
	MPlug sizePlug( thisNode, shapeNode->aSize );
	MDistance sizeVal;
	sizePlug.getValue( sizeVal );

	shapeNode->multiplier= (float) sizeVal.asCentimeters();

	int drawStyle = 0;
	MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
	drawStylePlug.getValue( drawStyle );

	view.beginGL();

	if (drawStyle < 3 && view.displayStyle() != M3dView::kBoundingBox )
	{
		drawPartio(cache,drawStyle);
	}
	else
	{
		drawBoundingBox();
	}

	partio4Maya::drawPartioLogo(shapeNode->multiplier);

	view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioVisualizerUI::drawBoundingBox() const
{


	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

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

void partioVisualizerUI::drawPartio(partioVizReaderCache* pvCache, int drawStyle) const
{
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

	MObject thisNode = shapeNode->thisMObject();
	MPlug drawSkipPlug( thisNode, shapeNode->aDrawSkip );
	int drawSkipVal;
	drawSkipPlug.getValue( drawSkipVal );

	MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ );
	bool flipYZVal;
	flipYZPlug.getValue( flipYZVal );

	int stride =  3*sizeof(float)*(drawSkipVal);

	MPlug pointSizePlug( thisNode, shapeNode->aPointSize );
	float pointSizeVal;
	pointSizePlug.getValue( pointSizeVal );

	MPlug colorFromPlug( thisNode, shapeNode->aColorFrom);
	int colorFromVal;
	colorFromPlug.getValue( colorFromVal );

	MPlug alphaFromPlug( thisNode, shapeNode->aAlphaFrom);
	int alphaFromVal;
	alphaFromPlug.getValue( alphaFromVal );

	MPlug defaultAlphaPlug( thisNode, shapeNode->aDefaultAlpha);
	float defaultAlphaVal;
	defaultAlphaPlug.getValue( defaultAlphaVal );


	if (pvCache->particles)
	{
		struct Point { float p[3]; };
		glPushAttrib(GL_CURRENT_BIT);

		if (alphaFromVal >=0 || defaultAlphaVal < 1) //testing settings
		{

				// TESTING AROUND with dept/transparency  sorting issues..
				glDepthMask(true);
				//cout << "depthMask"<<  glGetError() << endl;
				glEnable(GL_DEPTH_TEST);
				//cout << "depth test" <<  glGetError() << endl;
				glEnable(GL_BLEND);
				//cout << "blend" <<  glGetError() << endl;
//				glBlendEquation(GL_FUNC_ADD);
				//cout << "blend eq" <<  glGetError() << endl;
				glEnable(GL_POINT_SMOOTH);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				//cout << "blend func" <<  glGetError() << endl;
				//glAlphaFunc(GL_GREATER, .01);
				//glEnable(GL_ALPHA_TEST);
		}

		// THIS IS KINDA TRICKY.... we do this switch between drawing procedures because on big caches, when the reallocation happens
		// its big enough that it apparently frees the memory that the GL_Color arrays are using and  causes a segfault.
		// so we only draw once  using the "one by one" method  when the arrays change size, and then  swap back to  the speedier
		// pointer copy  way  after everything is settled down a bit for main interaction.   It is a significant  improvement on large
		// datasets in user interactivity speed to use pointers

		if(!shapeNode->cacheChanged)
		{
			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_COLOR_ARRAY );

			glPointSize(pointSizeVal);
			if (pvCache->particles->numParticles() > 0)
			{
				// now setup the position/color/alpha output pointers

				const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,0);

				if(flipYZVal)
				{
					glVertexPointer( 3, GL_FLOAT, stride, pvCache->flipPos );
				}
				else
				{
					glVertexPointer( 3, GL_FLOAT, stride, partioPositions );
				}

				if (defaultAlphaVal < 1 || alphaFromVal >=0)  // use transparency switch
				{
					glColorPointer(  4, GL_FLOAT, stride, pvCache->rgba );
				}
				else
				{
					glColorPointer(  3, GL_FLOAT, stride, pvCache->rgb );
				}
				glDrawArrays( GL_POINTS, 0, (pvCache->particles->numParticles()/(drawSkipVal+1)) );
			}
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_COLOR_ARRAY );
		}

		else
		{
			/// looping thru particles one by one...

			glPointSize(pointSizeVal);

			glBegin(GL_POINTS);

			for (int i=0;i<pvCache->particles->numParticles();i++)
			{
				if (defaultAlphaVal < 1 || alphaFromVal >=0)  // use transparency switch
				{
					glColor4f(pvCache->rgb[i*3],pvCache->rgb[(i*3)+1],pvCache->rgb[(i*3)+2], pvCache->rgba[(i*4)+3] );
				}
				else
				{
					glColor3f(pvCache->rgb[i*3],pvCache->rgb[(i*3)+1],pvCache->rgb[(i*3)+2]);
				}
				if (flipYZVal)
				{
					glVertex3f(pvCache->flipPos[0], pvCache->flipPos[1], pvCache->flipPos[2]);
				}
				else
				{
					const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,i);
					glVertex3f(partioPositions[0], partioPositions[1], partioPositions[2]);
				}
			}

			glEnd( );
		}
		glDisable(GL_POINT_SMOOTH);
		glPopAttrib();
	} // if (particles)


}

partioVisualizerUI::partioVisualizerUI()
{
}
partioVisualizerUI::~partioVisualizerUI()
{
}

void* partioVisualizerUI::creator()
{
	return new partioVisualizerUI();
}


void partioVisualizerUI::getDrawRequests(const MDrawInfo & info,
		bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{

	MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
	partioVizReaderCache* geom = shapeNode->updateParticleCache();

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

