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

#include "partioExport.h"

#define  kAttributeFlagS	"-atr"
#define  kAttributeFlagL	"-attribute"
#define  kMinFrameFlagS		"-mnf"
#define  kMinFrameFlagL		"-minFrame"
#define  kMaxFrameFlagS		"-mxf"
#define  kMaxFrameFlagL		"-maxFrame"
#define  kHelpFlagS			"-h"
#define  kHelpFlagL			"-help"
#define  kPathFlagS			"-p"
#define  kPathFlagL			"-path"
#define  kFlipFlagS			"-flp"
#define  kFlipFlagL			"-flip"
#define  kFormatFlagS		"-f"
#define  kFormatFlagL		"-format"
#define  kFilePrefixFlagS	"-fp"
#define  kFilePrefixFlagL	"-filePrefix"
#define  kPerFrameFlagS     "-pf"
#define  kPerFrameFlagL   	"-perFrame"


using namespace std;



bool PartioExport::hasSyntax()
{
    return true;
}

////// Has Syntax Similar to DynExport  to be a drop in replacement

MSyntax PartioExport::createSyntax()
{

    MSyntax syntax;

    syntax.addFlag(kHelpFlagS, kHelpFlagL,  MSyntax::kNoArg);
    syntax.addFlag(kPathFlagS, kPathFlagL,  MSyntax::kString);
    syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse( kAttributeFlagS );
    syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
    syntax.addFlag(kFormatFlagS, kFormatFlagL, MSyntax::kString);
    syntax.addFlag(kMinFrameFlagS,kMinFrameFlagL, MSyntax::kLong);
    syntax.addFlag(kMaxFrameFlagS, kMaxFrameFlagL, MSyntax::kLong);
    syntax.addFlag(kFilePrefixFlagS,kFilePrefixFlagL, MSyntax::kString);
	syntax.addFlag(kPerFrameFlagS,kPerFrameFlagL, MSyntax::kString);
    syntax.addArg(MSyntax::kString);
    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}


void* PartioExport::creator()
{
    return new PartioExport;
}


MStatus PartioExport::doIt(const MArgList& Args)
{

    MStatus status;
    MArgDatabase argData(syntax(), Args, &status);

    if ( argData.isFlagSet(kHelpFlagL))
    {
        printUsage();
        return MStatus::kFailure;
    }


    if ( Args.length() < 3)
    {
        MGlobal::displayError("PartioExport-> need the EXPORT PATH and a PARTICLESHAPE's NAME, and at least one ATTRIBUTE's NAME you want to export." );
        printUsage();
        return MStatus::kFailure;
    }

    MString Path;   // directory path
    MString Format;
    MString fileNamePrefix;
    bool hasFilePrefix = false;
	bool perFrame = false;


    if (argData.isFlagSet(kPathFlagL))
    {
        argData.getFlagArgument(kPathFlagL, 0, Path);
    }
    if (argData.isFlagSet(kFormatFlagL))
    {
        argData.getFlagArgument(kFormatFlagL, 0, Format);
    }
    if (argData.isFlagSet(kFilePrefixFlagL))
    {
        argData.getFlagArgument(kFilePrefixFlagL, 0, fileNamePrefix);
        if (fileNamePrefix.length() > 0)
        {
            hasFilePrefix = true;
        }
    }

    Format = Format.toLowerCase();

    if (Format != "pda" &&
            Format != "pdb" &&
            Format != "pdc" &&
            Format != "prt" &&
            Format != "bin" &&
            Format != "bgeo" &&
            Format != "geo" &&
            Format != "ptc" &&
            Format != "mc" &&
            Format != "rib" &&
            Format != "pts" &&
            Format != "xyz" &&
            Format != "pcd" &&
            Format != "rib" )
    {
        MGlobal::displayError("PartioExport-> format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,rib,ass");
        return MStatus::kFailure;
    }

    bool startFrameSet = false;
    bool endFrameSet = false;
    int  startFrame, endFrame;
    if (argData.isFlagSet(kMinFrameFlagL))
    {
        argData.getFlagArgument(kMinFrameFlagL, 0, startFrame);
        startFrameSet = true;
    }
    else
    {
        startFrame = -123456;
    }

    if (argData.isFlagSet(kMaxFrameFlagL))
    {
        argData.getFlagArgument(kMaxFrameFlagL, 0, endFrame);
        endFrameSet = true;
    }
    else
    {
        endFrame = -123456;
    }


    bool swapUP = false;


    if (argData.isFlagSet(kFlipFlagL))
    {
        swapUP = true;
    }

    if (argData.isFlagSet(kPerFrameFlagL))
	{
		perFrame = true;
	}

    MString PSName; // particle shape name
    argData.getCommandArgument(0, PSName);
    MSelectionList list;
    list.add(PSName);
    MObject objNode;
    list.getDependNode(0, objNode);
    MFnDependencyNode depNode(objNode, &status);

    if ( objNode.apiType() != MFn::kParticle && objNode.apiType() != MFn::kNParticle )
    {
        MGlobal::displayError("PartioExport-> can't find your PARTICLESHAPE.");
        return MStatus::kFailure;
    }

    /// parse attribute flags
    unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );

    /// loop thru the rest of the attributes given
    MStringArray  attrNames;
    attrNames.append(MString("id"));
    attrNames.append(MString("position"));

    bool worldVeloCheck = false;

    for ( unsigned int i = 0; i < numUses; i++ )
    {
        MArgList argList;
        status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
        if ( !status ) return status;

        MString AttrName = argList.asString( 0, &status );
        if ( !status ) return status;

        if ( AttrName == "position" || AttrName == "worldPosition"  || AttrName == "id" || AttrName == "particleId") {}
        else if ( AttrName == "worldVelocity" || AttrName == "velocity" )
        {
            if (!worldVeloCheck)
            {
                attrNames.append("velocity");
                worldVeloCheck = true;
            }
        }
        else
        {
            attrNames.append(AttrName);
        }

    }
    /// ARGS PARSED

    MComputation computation;
    computation.beginComputation();

    MFnParticleSystem PS(objNode);

    int outFrame= -123456;

    for  (int frame = startFrame; frame<=endFrame; frame++)
    {
		MTime dynTime;
		dynTime.setValue(frame);
		if (frame == startFrame && startFrame < endFrame)
		{
			PS.evaluateDynamics(dynTime,true);
		}
		else
		{
			PS.evaluateDynamics(dynTime,false);
		}

		/// why is this being done AFTER the evaluate dynamics stuff?
        if (startFrameSet && endFrameSet && startFrame < endFrame)
        {
            MGlobal::viewFrame(frame);
            outFrame = frame;
        }
        else
        {
            outFrame = (int)MAnimControl::currentTime().as(MTime::kFilm);
        }

        char padNum [10];

        // temp usage for this..  PDC's  are counted by 250s..  TODO:  implement  "substeps"  setting

        if (Format == "pdc")
        {
            int substepFrame = outFrame;
            substepFrame = outFrame*250;
            sprintf(padNum, "%d", substepFrame);
        }
        else
        {
            sprintf(padNum, "%04d", outFrame);
        }

        MString  outputPath =  Path;
        outputPath += "/";

        // if we have supplied a fileName prefix, then use it instead of the particle shape name
        if (hasFilePrefix)
        {
            outputPath += fileNamePrefix;
        }
        else
        {
            outputPath += PSName;
        }
        outputPath += ".";
        outputPath += padNum;
        outputPath += ".";
        outputPath += Format;

        MGlobal::displayInfo("PartioExport-> exporting: "+ outputPath);


        unsigned int particleCount = PS.count();

        Partio::ParticlesDataMutable* p = Partio::createInterleave();
        p->addParticles((const int)particleCount);
        Partio::ParticlesDataMutable::iterator it=p->begin();

        if (particleCount > 0)
        {

            for (unsigned int i = 0; i< attrNames.length(); i++)
            {
                // you must reset the iterator before adding new attributes or accessors
                it=p->begin();
                MString  attrName =  attrNames[i];
                //cout << attrName ;

                ///  INT Attribute found
                if (PS.isPerParticleIntAttribute(attrName) || attrName == "id" || attrName == "particleId")
                {

                    //cout << "-> INT " << endl;
                    MIntArray IA;

                    if ( attrName == "id" || attrName == "particleId" )
                    {
                        PS.particleIds( IA );
                        attrName = "id";
                        if (Format == "pdc")
                        {
                            attrName = "particleId";
                        }
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , IA, &status);
                    }

                    Partio::ParticleAttribute  intAttribute = p->addAttribute(attrName.asChar(), Partio::INT, 1);
                    //cout <<  "partio add Int attribute" << endl;

                    Partio::ParticleAccessor intAccess(intAttribute);
                    it.addAccessor(intAccess);
                    int a = 0;

                    for (it=p->begin();it!=p->end();++it)
                    {
                        Partio::Data<int,1>& intAttr=intAccess.data<Partio::Data<int,1> >(it);
                        intAttr[0] = IA[a];
                        a++;
                    }

                }/// add INT attribute

                /// DOUBLE Attribute found
                else if  (PS.isPerParticleDoubleAttribute(attrName))
                {
                    //cout << "-> FLOAT " << endl;
                    MDoubleArray DA;

                    if ( attrName == "radius" || attrName  == "radiusPP")
                    {
                        attrName = "radiusPP";
                        PS.radius( DA );
                    }
                    if ( attrName == "age" )
                    {
                        PS.age( DA );
                    }
                    else if ( attrName == "opacity" || attrName == "opacityPP")
                    {
                        attrName = "opacityPP";
                        PS.opacity( DA );
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , DA, &status);
                    }

                    Partio::ParticleAttribute  floatAttribute = p->addAttribute(attrName.asChar(), Partio::FLOAT, 1);
                    Partio::ParticleAccessor floatAccess(floatAttribute);
                    it.addAccessor(floatAccess);
                    int a = 0;

                    for (it=p->begin();it!=p->end();++it)
                    {

                        Partio::Data<float,1>& floatAttr=floatAccess.data<Partio::Data<float,1> >(it);
                        floatAttr[0] = float(DA[a]);
                        a++;

                    }

                } /// add double attr

                /// VECTOR Attribute found
                else if (PS.isPerParticleVectorAttribute(attrName))
                {

                    //cout << "-> VECTOR " << endl;
                    MVectorArray VA;

                    if ( attrName == "position" )
                    {
                        PS.position( VA );
                    }
                    else if ( attrName == "velocity")
                    {
                        PS.velocity( VA );
                    }
                    else if (attrName == "acceleration" )
                    {
                        PS.acceleration( VA );
                    }
                    else if (attrName == "rgbPP" )
                    {
                        PS.rgb( VA );
                    }
                    else if (attrName == "incandescencePP")
                    {
                        PS.emission( VA );
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , VA, &status);
                    }

                    Partio::ParticleAttribute  vectorAttribute = p->addAttribute(attrName.asChar(), Partio::VECTOR, 3);

                    Partio::ParticleAccessor vectorAccess(vectorAttribute);

                    it.addAccessor(vectorAccess);

                    int a = 0;

                    for (it=p->begin();it!=p->end();++it)
                    {

                        Partio::Data<float,3>& vecAttr=vectorAccess.data<Partio::Data<float,3> >(it);

                        MVector P = VA[a];
                        vecAttr[0] = (float)P.x;

                        if (swapUP && (attrName == "position" || attrName == "velocity" || attrName == "acceleration" ))
                        {
                            vecAttr[1] = -(float)P.z;
                            vecAttr[2] = (float)P.y;
                        }
                        else
                        {
                            vecAttr[1] = (float)P.y;
                            vecAttr[2] = (float)P.z;
                        }
                        a++;
                    }

                } /// add vector attr

            } /// loop attributes

            // check if directory exists, and if not, create it
            struct stat st;
            if (stat(Path.asChar(),&st) < 0)
            {
#ifdef WIN32
                HWND hwnd = NULL;
                const SECURITY_ATTRIBUTES *psa = NULL;
                SHCreateDirectoryEx(hwnd, Path.asChar(), psa);
#else
				mode_t userMask = umask(0);
				umask(userMask);
				mode_t DIR_MODE = ((0777) ^ userMask);
                mkdir (Path.asChar(), DIR_MODE );
#endif
            }

            Partio::write(outputPath.asChar(), *p );
            //cout << "partioCount" << endl;
            //cout << "end FRAME: " << outFrame << endl;
            //cout << "num particles" << p->numParticles() << endl;
            //cout << "num Attributes" << p->numAttributes() << endl;

            p->release();
            //cout << "released  memory" << endl;
        } /// if particle count > 0

        // support escaping early  in export command
        if  (computation.isInterruptRequested())
		{	MGlobal::displayWarning("PartioExport detected escape being pressed, ending export early!" ) ;
			break;
		}

	} /// loop frames

    return MStatus::kSuccess;
}

void PartioExport::printUsage()
{

    MString usage = "\n-----------------------------------------------------------------------------\n";
    usage += "\tpartioExport [Options]  node \n";
    usage += "\n";
    usage += "\t[Options]\n";
    usage += "\t\t-mnf/minFrame <int> \n";
    usage += "\t\t-mxf/maxFrame <int> \n";
    usage += "\t\t-f/format <string> (format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,pts,xyz,pcd,rib,ass)\n";
    usage += "\t\t-atr/attribute (multi use)  <PP attribute name>\n";
    usage += "\t\t     (position/id) are always exported \n";
    usage += "\t\t-p/path	 <directory file path> \n";
    usage += "\t\t-fp/filePrefix <fileNamePrefix>  \n";
    usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
    usage += "\n";
    usage += "\tExample:\n";
    usage += "\n";
    usage += "partioExport  -mnf 1 -mxf 10 -f prt -atr position -atr rgbPP -at opacityPP -fp \"SmokeShapepart1of20\" -p \"/file/path/to/output/directory\"  particleShape1 \n\n";

    MGlobal::displayInfo(usage);

}


