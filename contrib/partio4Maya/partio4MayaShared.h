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

#ifndef PARTIO4MAYASHARED
#define PARTIO4MAYASHARED
#pragma once


#ifdef WIN32
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <sstream>
#include <maya/MString.h>
#include <maya/MDataBlock.h>
#include <maya/MVector.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>


#ifdef OSMac_MachO_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif  WIN32
#include <gl/GLU.h>
#include <gl/GL.h>
#include <GL/glext.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#endif

#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue


#define TABLE_SIZE 256

extern const int kTableMask;
#define MODPERM(x) permtable[(x)&kTableMask]

class partio4Maya
{
public:

	static bool 	partioCacheExists(const char* fileName);
	static MStringArray partioGetBaseFileName(MString inFileName);
	static void 	updateFileName (MString cacheFile, MString cacheDir,
									 bool cacheStatic, int cacheOffset,
									 short cacheFormat, int integerTime,
									 int &cachePadding, MString &formatExt,
									 MString &outputFramePath, MString &outputRenderPath);

	static MString 	setExt(short extNum);
	static void 	buildSupportedExtensionList(std::map<short,MString> &formatExtMap,bool write);
	static void 	drawPartioLogo(float multiplier);
	static MVector 	jitterPoint(int id, float freq, float offset, float jitterMag);
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
};


//// INLINES
inline float partio4Maya::value( int x, float table[] ) {
	return table[MODPERM( x )];
}

#endif
