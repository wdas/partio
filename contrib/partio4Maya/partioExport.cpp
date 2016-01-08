/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
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

#include "partioExport.h"

#include <limits>

#define  kAttributeFlagS    "-atr"
#define  kAttributeFlagL    "-attribute"
#define  kMinFrameFlagS     "-mnf"
#define  kMinFrameFlagL     "-minFrame"
#define  kMaxFrameFlagS     "-mxf"
#define  kMaxFrameFlagL     "-maxFrame"
#define  kHelpFlagS         "-h"
#define  kHelpFlagL         "-help"
#define  kPathFlagS         "-p"
#define  kPathFlagL         "-path"
#define  kFlipFlagS         "-flp"
#define  kFlipFlagL         "-flip"
#define  kFormatFlagS       "-f"
#define  kFormatFlagL       "-format"
#define  kFilePrefixFlagS   "-fp"
#define  kFilePrefixFlagL   "-filePrefix"
#define  kPerFrameFlagS     "-pf"
#define  kPerFrameFlagL     "-perFrame"

bool PartioExport::hasSyntax()
{
    return true;
}

////// Has Syntax Similar to DynExport  to be a drop in replacement

MSyntax PartioExport::createSyntax()
{

    MSyntax syntax;

    syntax.addFlag(kHelpFlagS, kHelpFlagL, MSyntax::kNoArg);
    syntax.addFlag(kPathFlagS, kPathFlagL, MSyntax::kString);
    syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAttributeFlagS);
    syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
    syntax.addFlag(kFormatFlagS, kFormatFlagL, MSyntax::kString);
    syntax.addFlag(kMinFrameFlagS, kMinFrameFlagL, MSyntax::kLong);
    syntax.addFlag(kMaxFrameFlagS, kMaxFrameFlagL, MSyntax::kLong);
    syntax.addFlag(kFilePrefixFlagS, kFilePrefixFlagL, MSyntax::kString);
    syntax.addFlag(kPerFrameFlagS, kPerFrameFlagL, MSyntax::kString);
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

    if (argData.isFlagSet(kHelpFlagL))
    {
        printUsage();
        return MStatus::kFailure;
    }

    if (Args.length() < 3)
    {
        MGlobal::displayError(
                "PartioExport-> need the EXPORT PATH and a PARTICLESHAPE's NAME, and at least one ATTRIBUTE's NAME you want to export.");
        printUsage();
        return MStatus::kFailure;
    }

    MString Path;   // directory path
    MString Format;
    MString fileNamePrefix;
    bool hasFilePrefix = false;
    bool perFrame = false;


    if (argData.isFlagSet(kPathFlagL))
        argData.getFlagArgument(kPathFlagL, 0, Path);
    if (argData.isFlagSet(kFormatFlagL))
        argData.getFlagArgument(kFormatFlagL, 0, Format);
    if (argData.isFlagSet(kFilePrefixFlagL))
    {
        argData.getFlagArgument(kFilePrefixFlagL, 0, fileNamePrefix);
        if (fileNamePrefix.length() > 0)
            hasFilePrefix = true;
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
        Format != "rpc" &&
        //Format != "icecache" &&
        Format != "rib")
    {
        //MGlobal::displayError("PartioExport-> format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,icecache,rib,ass");
        MGlobal::displayError("PartioExport-> format is one of: pda,pdb,pdc,prt,bin,rpc,bgeo,geo,ptc,mc,rib,ass");
        return MStatus::kFailure;
    }

    bool startFrameSet = false;
    bool endFrameSet = false;
    int startFrame, endFrame;
    if (argData.isFlagSet(kMinFrameFlagL))
    {
        argData.getFlagArgument(kMinFrameFlagL, 0, startFrame);
        startFrameSet = true;
    }
    else
        startFrame = -123456;

    if (argData.isFlagSet(kMaxFrameFlagL))
    {
        argData.getFlagArgument(kMaxFrameFlagL, 0, endFrame);
        endFrameSet = true;
    }
    else
        endFrame = -123456;

    const bool swapUP = argData.isFlagSet(kFlipFlagL);

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

    if (objNode.apiType() != MFn::kParticle && objNode.apiType() != MFn::kNParticle)
    {
        MGlobal::displayError("PartioExport-> can't find your PARTICLESHAPE.");
        return MStatus::kFailure;
    }

    /// parse attribute flags
    unsigned int numUses = argData.numberOfFlagUses(kAttributeFlagL);

    /// loop thru the rest of the attributes given
    MStringArray attrNames;
    attrNames.append(MString("id"));
    attrNames.append(MString("position"));

    bool worldVeloCheck = false;

    for (unsigned int i = 0; i < numUses; i++)
    {
        MArgList argList;
        status = argData.getFlagArgumentList(kAttributeFlagL, i, argList);
        if (!status)
            return status;

        MString AttrName = argList.asString(0, &status);
        if (!status)
            return status;

        if (AttrName == "position" || AttrName == "worldPosition" || AttrName == "id" || AttrName == "particleId")
        {
        }
        else if (AttrName == "worldVelocity" || AttrName == "velocity")
        {
            if (!worldVeloCheck)
            {
                attrNames.append("velocity");
                worldVeloCheck = true;
            }
        }
        else
            attrNames.append(AttrName);
    }
    /// ARGS PARSED

    MComputation computation;
    computation.beginComputation();

    MFnParticleSystem PS(objNode);
    MFnParticleSystem DPS(objNode);

    if (PS.isDeformedParticleShape())
    {
        MObject origObj = PS.originalParticleShape();
        PS.setObject(origObj);
    }
    else
    {
        MObject defObj = PS.deformedParticleShape(&status);
        DPS.setObject(defObj);
    }

    //std::cout << "Orig Particle system -> " << PS.name() << std::endl;
    //std::cout << "Deformed Particle system -> " << DPS.name() << std::endl;

    int outFrame = -123456;

    for (int frame = startFrame; frame <= endFrame; frame++)
    {
        MTime dynTime;
        dynTime.setValue(frame);
        if (frame == startFrame && startFrame < endFrame)
        {
            PS.evaluateDynamics(dynTime, true);
            DPS.evaluateDynamics(dynTime, true);
        }
        else
        {
            PS.evaluateDynamics(dynTime, false);
            DPS.evaluateDynamics(dynTime, false);
        }

        /// why is this being done AFTER the evaluate dynamics stuff?
        if (startFrameSet && endFrameSet && startFrame < endFrame)
        {
            MGlobal::viewFrame(frame);
            outFrame = frame;
        }
        else
            outFrame = (int)MAnimControl::currentTime().as(MTime::uiUnit());

        char padNum[10];

        // temp usage for this..  PDC's  are counted by 250s..  TODO:  implement  "substeps"  setting

        if (Format == "pdc")
        {
            int substepFrame = outFrame;
            substepFrame = outFrame * 250;
            sprintf(padNum, "%d", substepFrame);
        }
        else
            sprintf(padNum, "%04d", outFrame);

        MString outputPath = Path;
        outputPath += "/";

        // if we have supplied a fileName prefix, then use it instead of the particle shape name
        if (hasFilePrefix)
            outputPath += fileNamePrefix;
        else
            outputPath += PSName;
        outputPath += ".";
        outputPath += padNum;
        outputPath += ".";
        outputPath += Format;

        MGlobal::displayInfo("PartioExport-> exporting: " + outputPath);
        const unsigned int particleCount = PS.count();

        const unsigned int numAttributes = attrNames.length();

        // In some cases, especially whenever particles are dying
        // the length of the attribute vector returned
        // from maya is smaller than the total number of particles.
        // So we have to first read all the attributes, then
        // determine the minimum amount of particles that all have valid data
        // then write the data out for them in one go. This introduces
        // code complexity, but still better than zeroing out the data
        // wherever we don't have valid data. Using zeroes could potentially
        // create popping artifacts especially if the particle system is
        // used for an instancer.
        if ((particleCount > 0) && (numAttributes > 0))
        {
            Partio::ParticlesDataMutable* p = Partio::createInterleave();

            std::vector<std::pair<MIntArray*, Partio::ParticleAttribute> > intAttributes;
            std::vector<std::pair<MDoubleArray*, Partio::ParticleAttribute> > doubleAttributes;
            std::vector<std::pair<MVectorArray*, Partio::ParticleAttribute> > vectorAttributes;

            unsigned int minParticleCount = particleCount;
            intAttributes.reserve(numAttributes);
            doubleAttributes.reserve(numAttributes);
            vectorAttributes.reserve(numAttributes);

            for (unsigned int i = 0; (i < numAttributes) && (minParticleCount > 0); ++i)
            {
                MString attrName = attrNames[i];
                ///  INT Attribute found
                if (PS.isPerParticleIntAttribute(attrName) || attrName == "id" || attrName == "particleId")
                {
                    MIntArray* IA = new MIntArray();
                    if (attrName == "id" || attrName == "particleId")
                    {
                        PS.particleIds(*IA);
                        attrName = "id";
                        if (Format == "pdc")
                            attrName = "particleId";
                    }
                    else
                        PS.getPerParticleAttribute(attrName, *IA, &status);
                    Partio::ParticleAttribute intAttribute = p->addAttribute(attrName.asChar(), Partio::INT, 1);
                    intAttributes.push_back(std::make_pair(IA, intAttribute));
                    minParticleCount = std::min(minParticleCount, IA->length());
                }
                else if (PS.isPerParticleDoubleAttribute(attrName)) // DOUBLE Attribute found
                {
                    MDoubleArray* DA = new MDoubleArray();

                    if (attrName == "radius" || attrName == "radiusPP")
                    {
                        attrName = "radiusPP";
                        PS.radius(*DA);
                    }
                    if (attrName == "age")
                        PS.age(*DA);
                    else if (attrName == "opacity" || attrName == "opacityPP")
                    {
                        attrName = "opacityPP";
                        PS.opacity(*DA);
                    }
                    else
                        PS.getPerParticleAttribute(attrName, *DA, &status);

                    Partio::ParticleAttribute floatAttribute = p->addAttribute(attrName.asChar(), Partio::FLOAT, 1);
                    doubleAttributes.push_back(std::make_pair(DA, floatAttribute));
                    minParticleCount = std::min(minParticleCount, DA->length());
                } // add double attr                
                else if (PS.isPerParticleVectorAttribute(attrName)) // VECTOR Attribute found
                {
                    MVectorArray* VA = new MVectorArray();
                    if (attrName == "position")
                        DPS.position(*VA);
                    else if (attrName == "velocity")
                        PS.velocity(*VA);
                    else if (attrName == "acceleration")
                        PS.acceleration(*VA);
                    else if (attrName == "rgbPP")
                        PS.rgb(*VA);
                    else if (attrName == "incandescencePP")
                        PS.emission(*VA);
                    else
                        PS.getPerParticleAttribute(attrName, *VA, &status);

                    Partio::ParticleAttribute vectorAttribute = p->addAttribute(attrName.asChar(), Partio::VECTOR, 3);
                    vectorAttributes.push_back(std::make_pair(VA, vectorAttribute));
                    minParticleCount = std::min(minParticleCount, VA->length());
                }
            }

            // check if directory exists, and if not, create it
            struct stat st;
            if (stat(Path.asChar(), &st) < 0)
            {
#ifdef WIN32
                HWND hwnd = NULL;
                const SECURITY_ATTRIBUTES *psa = NULL;
                SHCreateDirectoryEx(hwnd, Path.asChar(), psa);
#else
                mode_t userMask = umask(0);
                umask(userMask);
                mode_t DIR_MODE = ((0777) ^ userMask);
                mkdir(Path.asChar(), DIR_MODE);
#endif
            }

            if (minParticleCount > 0)
            {
                p->addParticles(minParticleCount);

                for (std::vector<std::pair<MIntArray*, Partio::ParticleAttribute> >::iterator it = intAttributes.begin();
                     it != intAttributes.end(); ++it)
                {
                    Partio::ParticleIterator<false> pit = p->begin();
                    Partio::ParticleAccessor intAccessor(it->second);
                    pit.addAccessor(intAccessor);
                    for (int id = 0; pit != p->end(); ++pit)
                        intAccessor.data<Partio::Data<int, 1> >(pit)[0] = it->first->operator[](id++);
                }

                for (std::vector<std::pair<MDoubleArray*, Partio::ParticleAttribute> >::iterator it = doubleAttributes.begin();
                     it != doubleAttributes.end(); ++it)
                {
                    Partio::ParticleIterator<false> pit = p->begin();
                    Partio::ParticleAccessor doubleAccessor(it->second);
                    pit.addAccessor(doubleAccessor);
                    for (int id = 0; pit != p->end(); ++pit)
                        doubleAccessor.data<Partio::Data<float, 1> >(pit)[0] = static_cast<float>(it->first->operator[](
                                id++));
                }

                for (std::vector<std::pair<MVectorArray*, Partio::ParticleAttribute> >::iterator it = vectorAttributes.begin();
                     it != vectorAttributes.end(); ++it)
                {
                    Partio::ParticleIterator<false> pit = p->begin();
                    Partio::ParticleAccessor vectorAccessor(it->second);
                    pit.addAccessor(vectorAccessor);
                    const bool doSwap = swapUP &&
                                        ((it->second.name == "position") || (it->second.name == "velocity") ||
                                         (it->second.name == "acceleration"));
                    for (int id = 0; pit != p->end(); ++pit)
                    {
                        Partio::Data<float, 3>& vecAttr = vectorAccessor.data<Partio::Data<float, 3> >(pit);
                        const MVector& P = it->first->operator[](id++);
                        vecAttr[0] = static_cast<float>(P.x);

                        if (doSwap)
                        {
                            vecAttr[1] = -static_cast<float>(P.z);
                            vecAttr[2] = static_cast<float>(P.y);
                        }
                        else
                        {
                            vecAttr[1] = static_cast<float>(P.y);
                            vecAttr[2] = static_cast<float>(P.z);
                        }
                    }
                }

                Partio::write(outputPath.asChar(), *p);
            }
            else
                std::cout << "[partioExport] There are no particles with valid data." << std::endl;

            for (std::vector<std::pair<MIntArray*, Partio::ParticleAttribute> >::iterator it = intAttributes.begin();
                 it != intAttributes.end(); ++it)
                delete it->first;

            for (std::vector<std::pair<MDoubleArray*, Partio::ParticleAttribute> >::iterator it = doubleAttributes.begin();
                 it != doubleAttributes.end(); ++it)
                delete it->first;

            for (std::vector<std::pair<MVectorArray*, Partio::ParticleAttribute> >::iterator it = vectorAttributes.begin();
                 it != vectorAttributes.end(); ++it)
                delete it->first;

            p->release();
        }

        // support escaping early  in export command
        if (computation.isInterruptRequested())
        {
            MGlobal::displayWarning("PartioExport detected escape being pressed, ending export early!");
            break;
        }
    }

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
    //usage += "\t\t-f/format <string> (format is one of: pda,pdb,pdc,prt,bin,bgeo,geo,ptc,mc,pts,xyz,pcd,icecache,rib,ass)\n";
    usage += "\t\t-f/format <string> (format is one of: pda,pdb,pdc,prt,bin,rpc,bgeo,geo,ptc,mc,pts,xyz,pcd,rib,ass)\n";
    usage += "\t\t-atr/attribute (multi use)  <PP attribute name>\n";
    usage += "\t\t     (position/id) are always exported \n";
    usage += "\t\t-p/path    <directory file path> \n";
    usage += "\t\t-fp/filePrefix <fileNamePrefix>  \n";
    usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
    usage += "\n";
    usage += "\tExample:\n";
    usage += "\n";
    usage += "partioExport  -mnf 1 -mxf 10 -f prt -atr position -atr rgbPP -at opacityPP -fp \"SmokeShapepart1of20\" -p \"/file/path/to/output/directory\"  particleShape1 \n\n";

    MGlobal::displayInfo(usage);
}
