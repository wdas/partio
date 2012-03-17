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
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MPxEmitterNode.h>
#include <maya/MPlugArray.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MIntArray.h>
#include <maya/MStringArray.h>
#include <maya/MSceneMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MMessage.h>
#include <vector>
#include <map>

#include "Partio.h"

#define TABLE_SIZE 256

extern const int kTableMask;
#define MODPERM(x) permtable[(x)&kTableMask]

#define McheckErr(stat, msg)\
	if ( MS::kSuccess != stat )\
	{\
		cerr << msg;\
		return MS::kFailure;\
	}

class partioEmitter: public MPxEmitterNode
{
	public:
		partioEmitter();
		virtual ~partioEmitter();

		static void	*creator();
		static MStatus	initialize();
		virtual void postConstructor();
		static MTypeId	id;
		static MObject 	aCacheDir;
		static MObject 	aCachePrefix;
		static MObject 	aUseEmitterTransform;
		static MObject 	aCacheActive;
		static MObject 	aCacheOffset;
		static MObject 	aCacheFormat;
		static MObject 	aCachePadding;
		static MObject  aCacheStatic;
		static MObject 	aJitterPos;
		static MObject 	aJitterFreq;
		static MObject 	aPartioAttributes;
		static MObject  aMayaPPAttributes;

		virtual MStatus	compute ( const MPlug& plug, MDataBlock& block );
		static void 	reInit(void *data);
		void 			initCallback();
		static void 	connectionMadeCallbk(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData);
		static float  	noiseAtValue( float x);
		static void   	initTable( long seed );

	private:

		static int    	permtable   [256];
		static float  	valueTable1 [256];
		static float  	valueTable2 [256];
		static float  	valueTable3 [256];
		static int    	isInitialized;

		static float  	spline( float x, float knot0, float knot1, float knot2, float knot3 );

		static float  	value( int x, float table[] = valueTable1 );
		long 		seedValue( int  plugIndex, MDataBlock& block );

		MStatus	getWorldPosition ( MPoint &p );

		// methods to get attribute value.
		//
		double	rateValue ( MDataBlock& block );
		double	speedValue ( MDataBlock& block );
		MVector	directionVector ( MDataBlock& block );

		bool	isFullValue ( int plugIndex, MDataBlock& block );
		double	inheritFactorValue ( int plugIndex, MDataBlock& block );

		MTime	currentTimeValue ( MDataBlock& block );
		MTime	startTimeValue ( int plugIndex, MDataBlock& block );
		MTime	deltaTimeValue ( int plugIndex, MDataBlock& block );

		// the previous position in the world space.
		//
		MPoint	lastWorldPoint;
		MString mLastFileLoaded;
		MString mLastPath;
		MString mLastPrefix;
		MString mLastExt;
		bool cacheChanged;

		virtual void 	draw ( M3dView  & view, const  MDagPath  & path,  M3dView::DisplayStyle  style, M3dView:: DisplayStatus );
		MVector  jitterPoint(int id, float freq, float offset, float jitterMag);
		static MStatus createPPAttr( MFnParticleSystem  &part, MString attrName, MString shortName, int type);
};

//////////////////////////////////////////////
// inlines
//

inline float partioEmitter::value( int x, float table[] ) {
	return table[MODPERM( x )];
}

inline long partioEmitter::seedValue( int plugIndex, MDataBlock& block )
{
	MStatus status;
	long seed = 0;

	MArrayDataHandle mhValue = block.inputArrayValue( mSeed, &status );
	if( status == MS::kSuccess )
	{
		status = mhValue.jumpToElement( plugIndex );
		if( status == MS::kSuccess )
		{
			MDataHandle hValue = mhValue.inputValue( &status );
			if( status == MS::kSuccess )
				seed = hValue.asInt();
		}
	}

	return( seed );
}


inline double partioEmitter::rateValue ( MDataBlock& block )
{//mLastFileLoaded = "";
	MStatus status;

	MDataHandle hValue = block.inputValue ( mRate, &status );

	double value = 0.0;
	if ( status == MS::kSuccess )
		value = hValue.asDouble();

	return ( value );
}

inline double partioEmitter::speedValue ( MDataBlock& block )
{
	MStatus status;

	MDataHandle hValue = block.inputValue ( mSpeed, &status );

	double value = 0.0;
	if ( status == MS::kSuccess )
		value = hValue.asDouble();

	return ( value );
}

inline MVector partioEmitter::directionVector ( MDataBlock& block )
{
	MStatus status;
	MVector dirV ( 0.0, 0.0, 0.0 );

	MDataHandle hValue = block.inputValue ( mDirection, &status );

	if ( status == MS::kSuccess )
	{
		double3 &value = hValue.asDouble3();

		dirV[0] = value[0];
		dirV[1] = value[1];
		dirV[2] = value[2];
	}

	return ( dirV );
}

inline bool partioEmitter::isFullValue ( int plugIndex, MDataBlock& block )
{
	MStatus status;
	bool value = true;

	MArrayDataHandle mhValue = block.inputArrayValue ( mIsFull, &status );
	if ( status == MS::kSuccess )
	{
		status = mhValue.jumpToElement ( plugIndex );
		if ( status == MS::kSuccess )
		{
			MDataHandle hValue = mhValue.inputValue ( &status );
			if ( status == MS::kSuccess )
				value = hValue.asBool();
		}
	}

	return ( value );
}

inline double partioEmitter::inheritFactorValue ( int plugIndex,MDataBlock& block )
{
	MStatus status;
	double value = 0.0;

	MArrayDataHandle mhValue = block.inputArrayValue ( mInheritFactor, &status );
	if ( status == MS::kSuccess )
	{
		status = mhValue.jumpToElement ( plugIndex );
		if ( status == MS::kSuccess )
		{
			MDataHandle hValue = mhValue.inputValue ( &status );
			if ( status == MS::kSuccess )
				value = hValue.asDouble();
		}
	}

	return ( value );
}

inline MTime partioEmitter::currentTimeValue ( MDataBlock& block )
{
	MStatus status;

	MDataHandle hValue = block.inputValue ( mCurrentTime, &status );

	MTime value ( 0.0 );
	if ( status == MS::kSuccess )
		value = hValue.asTime();

	return ( value );
}

inline MTime partioEmitter::startTimeValue ( int plugIndex, MDataBlock& block )
{
	MStatus status;
	MTime value ( 0.0 );

	MArrayDataHandle mhValue = block.inputArrayValue ( mStartTime, &status );
	if ( status == MS::kSuccess )
	{
		status = mhValue.jumpToElement ( plugIndex );
		if ( status == MS::kSuccess )
		{
			MDataHandle hValue = mhValue.inputValue ( &status );
			if ( status == MS::kSuccess )
				value = hValue.asTime();
		}
	}

	return ( value );
}

inline MTime partioEmitter::deltaTimeValue ( int plugIndex, MDataBlock& block )
{
	MStatus status;
	MTime value ( 0.0 );

	MArrayDataHandle mhValue = block.inputArrayValue ( mDeltaTime, &status );
	if ( status == MS::kSuccess )
	{
		status = mhValue.jumpToElement ( plugIndex );
		if ( status == MS::kSuccess )
		{
			MDataHandle hValue = mhValue.inputValue ( &status );
			if ( status == MS::kSuccess )
				value = hValue.asTime();
		}
	}

	return ( value );
}

