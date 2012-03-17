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

#include <map>
#include "partio4MayaShared.h"
#include "iconArrays.h"
#include "Partio.h"

using namespace Partio;
using namespace std;

//////////////////////////////////////////////////
bool partio4Maya::partioCacheExists(const char* fileName)
{

    struct stat fileInfo;
    bool statReturn;
    int intStat;

    intStat = stat(fileName, &fileInfo);
    if (intStat == 0)
    {
        statReturn = true;
    }
    else
    {
        statReturn = false;
    }

    return(statReturn);

}

//////////////////////////////////////////////////////////////////////
MString partio4Maya::getBaseName(MString pattern, MString file) {

	MString basename;

	// we're assuming there can be  periods in the file name and  that the file has at least one period
	//  fileName.ext  or fileName.#.ext

	int idx = file.rindexW(pattern);

	if (idx != -1)
	{
		basename = file.substring(0, idx-1);
		idx = basename.rindexW(pattern);
		if (idx != -1)
		{
			basename = basename.substring(0, idx-1);
		}
		return basename;
	}
	else
	{
		return file;
	}

}

MString partio4Maya::updateFileName (MString cacheFile, MString cacheDir, bool cacheStatic, int cacheOffset, int cachePadding,
						short cacheFormat,int integerTime, MString &formatExt)
{

	formatExt = setExt(cacheFormat);

	int cacheFrame =  integerTime + cacheOffset;

	MString formatString =  "%s%s.%0";
	// special case for PDCs and maya nCache files because of the funky naming convention  TODO: support substepped/retiming  caches
	if (formatExt == "pdc")
	{
		cacheFrame *= (int)(6000 / 24);
		cachePadding = 1;
	}

	else if (formatExt == "mc")
	{
		cachePadding = 1;
		formatString = "%s%sFrame%0";
		int idx = cacheFile.rindexW("Frame");

		if (idx != -1)
		{
			cacheFile = cacheFile.substring(0, idx-1);
		}
	}

	char fileName[512] = "";

	formatString += cachePadding;
	formatString += "d.%s";

	const char* fmt = formatString.asChar();

	MString cachePrefix = getBaseName(".", cacheFile);

	MString  newCacheFile;

	if (!cacheStatic)
	{
		sprintf(fileName, fmt, cacheDir.asChar(), cachePrefix.asChar(), cacheFrame, formatExt.asChar());
		newCacheFile = fileName;
	}
	else
	{
		newCacheFile = cacheFile;
	}
	return newCacheFile;
}
////////////////////////////////////////////////////////////////
MString partio4Maya::setExt(short extEnum)
{
	std::map<short,MString> formatExtMap;
	buildSupportedExtensionList(formatExtMap, false);  // eventually this will be replaced with something from partio
	return MString(formatExtMap[extEnum]);
}

///////////////////////////////////////////////////////////
// eventually this will be replaced with something from partio directly
void partio4Maya::buildSupportedExtensionList(std::map<short,MString> &formatExtMap,bool write = false)
{

	formatExtMap[0] = "bgeo";
	formatExtMap[1] = "geo";
	formatExtMap[2] = "pda";
	formatExtMap[3] = "pdb";
	formatExtMap[4] = "pdc";
	formatExtMap[5] = "mc";
	formatExtMap[6] = "bin";
	formatExtMap[7] = "prt";
	formatExtMap[8] = "ptc";
	formatExtMap[9] = "pts";
	if(write)
	{
		formatExtMap[10] = "rib";
		formatExtMap[11] = "ass";
	}
}


/////////////////////////////////////////
void partio4Maya::drawPartioLogo(float multiplier)
{
    glBegin ( GL_LINES );
	glColor4f(0.0,0.0,0.0,1.0);
	int i,d;

    int last = P1Count - 1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( P1[i][0] * multiplier,
                     P1[i][1] * multiplier,
                     P1[i][2] * multiplier );
        glVertex3f ( P1[i+1][0] * multiplier,
                     P1[i+1][1] * multiplier,
                     P1[i+1][2] * multiplier );
    }
    last = P2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( P2[i][0] * multiplier,
                     P2[i][1] * multiplier,
                     P2[i][2] * multiplier );
        glVertex3f ( P2[i+1][0] * multiplier,
                     P2[i+1][1] * multiplier,
                     P2[i+1][2] * multiplier );
    }
    last = a1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( a1[i][0] * multiplier,
                     a1[i][1] * multiplier,
                     a1[i][2] * multiplier );
        glVertex3f ( a1[i+1][0] * multiplier,
                     a1[i+1][1] * multiplier,
                     a1[i+1][2] * multiplier );
    }
    last = a2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( a2[i][0] * multiplier,
                     a2[i][1] * multiplier,
                     a2[i][2] * multiplier );
        glVertex3f ( a2[i+1][0] * multiplier,
                     a2[i+1][1] * multiplier,
                     a2[i+1][2] * multiplier );
    }
    last = rCount -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( r[i][0] * multiplier,
                     r[i][1] * multiplier,
                     r[i][2] * multiplier );
        glVertex3f ( r[i+1][0] * multiplier,
                     r[i+1][1] * multiplier,
                     r[i+1][2] * multiplier );
    }
    last = tCount -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( t[i][0] * multiplier,
                     t[i][1] * multiplier,
                     t[i][2] * multiplier );
        glVertex3f ( t[i+1][0] * multiplier,
                     t[i+1][1] * multiplier,
                     t[i+1][2] * multiplier );
    }
    last = i1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( i1[i][0] * multiplier,
                     i1[i][1] * multiplier,
                     i1[i][2] * multiplier );
        glVertex3f ( i1[i+1][0] * multiplier,
                     i1[i+1][1] * multiplier,
                     i1[i+1][2] * multiplier );
    }
    last = i2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( i2[i][0] * multiplier,
                     i2[i][1] * multiplier,
                     i2[i][2] * multiplier );
        glVertex3f ( i2[i+1][0] * multiplier,
                     i2[i+1][1] * multiplier,
                     i2[i+1][2] * multiplier );
    }
    last = o1Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( o1[i][0] * multiplier,
                     o1[i][1] * multiplier,
                     o1[i][2] * multiplier );
        glVertex3f ( o1[i+1][0] * multiplier,
                     o1[i+1][1] * multiplier,
                     o1[i+1][2] * multiplier );
    }
    last = o2Count -1;
    for ( i = 0; i < last; ++i )
    {
        glVertex3f ( o2[i][0] * multiplier,
                     o2[i][1] * multiplier,
                     o2[i][2] * multiplier );
        glVertex3f ( o2[i+1][0] * multiplier,
                     o2[i+1][1] * multiplier,
                     o2[i+1][2] * multiplier );
    }

    for ( d = 0; d < debrisCount; d++ )
    {
        for ( i = 0; i < ( debrisPointCount-1 ); ++i )
        {
            glVertex3f ( circles[d][i][0] * multiplier,
                         circles[d][i][1] * multiplier,
                         circles[d][i][2] * multiplier );
            glVertex3f ( circles[d][i+1][0] * multiplier,
                         circles[d][i+1][1] * multiplier,
                         circles[d][i+1][2] * multiplier );
        }
    }
    glEnd();

}

