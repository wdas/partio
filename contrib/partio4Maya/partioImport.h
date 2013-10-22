/* partio4Maya  7/24/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Import
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

#ifndef Partio4MayaImport_H
#define Partio4MayaImport_H

#ifdef WIN32
	#include <shlobj.h>
#endif

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <zlib.h>
#include <sys/stat.h>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MArgDatabase.h>
#include <maya/MAnimControl.h>
#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MVectorArray.h>

#include <maya/MPxCommand.h>

#include <maya/MFnParticleSystem.h>

#include "Partio.h"
#include "partio4MayaShared.h"

class PartioImport : public MPxCommand 
{
	public:
		PartioImport(){};
		virtual ~PartioImport(){};
		
		static void* creator();
		
		virtual bool hasSyntax();
		static  MSyntax createSyntax();
		
		MStatus doIt(const MArgList&);
		void printUsage();
};

#endif





