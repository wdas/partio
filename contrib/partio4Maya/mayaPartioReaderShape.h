#ifndef MAYAPARTIOREADERSHAPE_H_
#define MAYAPARTIOREADERSHAPE_H_



#include <maya/MBoundingBox.h>
#include <maya/MPxNode.h>
#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>

#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MSelectionMask.h>
#include <maya/MSelectionList.h>


#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>


#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMeshData.h>


#include "Partio.h"

class CPartioReaderCache
{
public:
	CPartioReaderCache();
	MString cacheFile;
	float frame;
	int decimation;
	float frameOffset;
	float fps;
	bool useFrameExtension;
	bool IsParticleLoaded;
	bool updateView;
	int numParticles;

	float sphereSize;
	int sphereTesselatation;
	int mode;

	int token;

	MBoundingBox bbox;

	//Partio::ParticlesDataMutable* p;
	//Partio::ParticleAttribute positionhandle;

	int dList;

};

// Shape class - defines the non-UI part of a shape node
class mayaPartioReaderShape: public MPxSurfaceShape
{
public:
	mayaPartioReaderShape();
	virtual ~mayaPartioReaderShape();

	virtual void postConstructor();
	virtual MStatus compute(const MPlug& plug, MDataBlock& data);
	//virtual bool getInternalValueInContext(const MPlug&, MDataHandle&,
	//		MDGContext &context);
	//virtual bool setInternalValueInContext(const MPlug&, const MDataHandle&,
	//		MDGContext &context);

	virtual bool isBounded() const;
	virtual MBoundingBox boundingBox() const;

	//MStatus GetPointsFromCache();

	static void* creator();
	static MStatus initialize();
	CPartioReaderCache* particleCache();
	bool GetPlugData();



private:
	CPartioReaderCache fGeometry;
	// Attributes
	//static CStaticAttrHelper s_attributes;
	static MObject s_particleCache;
	static MObject s_decimation;
	static MObject s_mode;
	static MObject s_sphereRadius;
	static MObject s_sphereTesselatation;
	static MObject s_useFrameExtension;
	static MObject s_frameNumber;
	static MObject s_useSubFrame;
	static MObject s_frameOffset;
	static MObject s_updateNode;

public:
	static MTypeId id;

protected:
	int dUpdate;

}; // class mayaPartioReaderShape


// UI class	- defines the UI part of a shape node
class CPartioReaderUI: public MPxSurfaceShapeUI {
public:
	CPartioReaderUI();
	virtual ~CPartioReaderUI();
	virtual void getDrawRequests(const MDrawInfo & info,
			bool objectAndActiveOnly, MDrawRequestQueue & requests);
	virtual void draw(const MDrawRequest & request, M3dView & view) const;
	virtual bool select(MSelectInfo &selectInfo, MSelectionList &selectionList,
			MPointArray &worldSpaceSelectPts) const;

	void getDrawRequestsWireFrame(MDrawRequest&, const MDrawInfo&);
	//void getDrawRequestsShaded(	  MDrawRequest&,
	//										  const MDrawInfo&,
	//										  MDrawRequestQueue&,
	//										  MDrawData& data );

	static void * creator();
	// Draw Tokens
	//
	enum {
		kDrawWireframe,
		kDrawWireframeOnShaded,
		kDrawSmoothShaded,
		kDrawFlatShaded,
		kLastToken
	};

}; // class CArnoldStandInShapeUI

#endif

