/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Instancer
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

#include <maya/MFloatArray.h>
#include <maya/MVectorArray.h>
#include <maya/MPointArray.h>
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MIntArray.h>
#include <maya/MStringArray.h>
#include <maya/MBoundingBox.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MTypeId.h>
#include <maya/MStatus.h>
#include <maya/MVector.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/M3dView.h>
#include <maya/MSceneMessage.h>
#include <maya/MDrawData.h>
#include <maya/MFnArrayAttrsData.h>
#include <vector>

#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>

class partioInstReaderCache
{
public:
	partioInstReaderCache();
	int token;
	MBoundingBox bbox;
	int dList;
	Partio::ParticlesDataMutable* particles;
	Partio::ParticleAttribute positionAttr;
	Partio::ParticleAttribute idAttr;
	Partio::ParticleAttribute velocityAttr;
	Partio::ParticleAttribute rotationAttr;
	Partio::ParticleAttribute scaleAttr;
	Partio::ParticleAttribute indexAttr;
	Partio::ParticleAttribute shaderIndexAttr;
	float* flipPos;
	MFnArrayAttrsData instanceData;
	MObject instanceDataObj;


};


class partioInstancerUI : public MPxSurfaceShapeUI
{
public:

    partioInstancerUI();
    virtual ~partioInstancerUI();
	virtual void draw(const MDrawRequest & request, M3dView & view) const;
	virtual void getDrawRequests(const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests);
	void 	drawBoundingBox() const;
	void 	drawPartio(partioInstReaderCache* pvCache, int drawStyle) const;
	static void * creator();

};

class partioInstancer : public MPxSurfaceShape
{
public:
	partioInstancer();
	virtual ~partioInstancer();

    virtual MStatus   		compute( const MPlug& plug, MDataBlock& block );
	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const;
	static  void *          creator();
	static  MStatus         initialize();
	static void 			reInit(void *data);
	void 					initCallback();
	virtual void 			postConstructor();

	bool GetPlugData();
	void addParticleAttr(int attrIndex, MString attrName );
	partioInstReaderCache* updateParticleCache();


	static MObject  time;
	static MObject  aSize;         // The size of the logo
	static MObject  aFlipYZ;
	static MObject 	aUpdateCache;
	static MObject 	aCacheDir;
	static MObject 	aCachePrefix;
	static MObject 	aUseTransform;
	static MObject 	aCacheActive;
	static MObject 	aCacheOffset;
	static MObject  aCacheStatic;
	static MObject 	aCacheFormat;
	static MObject 	aCachePadding;
	static MObject  aCachePreDelim;
	static MObject  aCachePostDelim;
	static MObject 	aJitterPos;
	static MObject 	aJitterFreq;
	static MObject 	aPartioAttributes;
	static MObject  aPointSize;
	static MObject  aDrawStyle;
	static MObject  aForceReload;
	static MObject  aRenderCachePath;
	static MObject	aRotationFrom;
	static MObject	aScaleFrom;
	static MObject	aIndexFrom;
	static MObject	aShaderIndexFrom;
	static MObject	aInMeshInstances;
	static MObject	aOutMesh;
	static MObject	aInstanceData;
	static MObject  aComputeVeloPos;


	static	MTypeId			id;
	float 					multiplier;
	bool 					cacheChanged;
	partioInstReaderCache  	pvCache;


private:

	MString mLastFileLoaded;
	MString mLastPath;
	MString mLastPrefix;
	MString mLastExt;
	bool mLastFlipStatus;
	bool mFlipped;
	bool  frameChanged;
	MStringArray attributeList;
	int mLastRotationFromIndex;
	int mLastScaleFromIndex;
	int mLastIndexFromIndex;
	int mLastShaderIndexFromIndex;
	bool canMotionBlur;

protected:

	int dUpdate;
	GLuint dList;

};

