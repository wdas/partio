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
#include <maya/MPxLocatorNode.h>
#include <maya/M3dView.h>
#include <maya/MSceneMessage.h>
#include <vector>

#include "Partio.h"


class partioVisualizer : public MPxLocatorNode
{
public:
	partioVisualizer();
	virtual ~partioVisualizer();

    virtual MStatus   		compute( const MPlug& plug, MDataBlock& block );


	virtual void            draw( M3dView & view, const MDagPath & path,
								  M3dView::DisplayStyle style,
								  M3dView::DisplayStatus status );

	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const;

	static  void *          creator();
	static  MStatus         initialize();
	static void 			reInit(void *data);
	void 					initCallback();
	virtual void 			postConstructor();

	static MObject  time;
	static MObject  aSize;         // The size of the logo
	static MObject  aDrawSkip;
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
	static MObject 	aJitterPos;
	static MObject 	aJitterFreq;
	static MObject 	aPartioAttributes;
	static MObject  aColorFrom;
	static MObject  aAlphaFrom;
	static MObject  aRadiusFrom;
	static MObject  aPointSize;
	static MObject  aDefaultPointColor;
	static MObject  aDefaultAlpha;
	static MObject  aInvertAlpha;
	static MObject  aDrawStyle;
	static MObject  aForceReload;



	bool GetPlugData();

	void drawBoundingBox();
	void drawPartio(int drawStyle);

public:
	static	MTypeId		id;

private:
	MString mLastFileLoaded;
	MString mLastPath;
	MString mLastPrefix;
	MString mLastExt;
	int mLastColorFromIndex;
	int mLastAlphaFromIndex;
	int mLastRadiusFromIndex;
	MFloatVector mLastColor;
	float mLastAlpha;
	bool mLastInvertAlpha;
	bool cacheChanged;
	float multiplier;
	bool  frameChanged;

	Partio::ParticlesData* particles;
	Partio::ParticleAttribute positionAttr;
	Partio::ParticleAttribute colorAttr;
	Partio::ParticleAttribute opacityAttr;
	float* rgb;
	float* rgba;

	MBoundingBox bbox;
	MStringArray attributeList;

protected:

	float xMin, xMax, yMin, yMax, zMin, zMax;

	int dUpdate;
	GLuint dList;
};

