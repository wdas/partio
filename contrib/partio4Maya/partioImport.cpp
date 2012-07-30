/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Import
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

#include "partioImport.h"

#include <maya/MStatus.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MIntArray.h>

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>

#ifdef WIN32
#include <zlib.h>
#include <shlobj.h>
#endif

static const char *kAttributeFlagS	= "-atr";
static const char *kAttributeFlagL  = "-attribute";
static const char *kFlipFlagS		= "-flp";
static const char *kFlipFlagL		= "-flip";
static const char *kHelpFlagS 		= "-h";
static const char *kHelpFlagL 		= "-help";
static const char *kParticleL		= "-particle";
static const char *kParticleS		= "-p";

using namespace std;
using namespace Partio;

bool PartioImport::hasSyntax()
{
	return true;
}

MSyntax PartioImport::createSyntax()
{

	MSyntax syntax;

	syntax.addFlag(kParticleS, kParticleL ,  MSyntax::kString);
	syntax.addFlag(kHelpFlagS, kHelpFlagL ,  MSyntax::kNoArg);
	syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
	syntax.makeFlagMultiUse( kAttributeFlagS );
	syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
	syntax.addArg(MSyntax::kString);
	syntax.enableQuery(false);
	syntax.enableEdit(false);

	return syntax;
}



void* PartioImport::creator()
{
	return new PartioImport;
}

MStatus PartioImport::doIt(const MArgList& Args)
{
	bool makeParticle = false;

	MStatus status;
	MArgDatabase argData(syntax(), Args, &status);

	if (status == MStatus::kFailure)
	{
		MGlobal::displayError("Error parsing arguments" );
	}

	if( argData.isFlagSet(kHelpFlagL) )
	{
		printUsage();
		return MStatus::kFailure;
	}

	MString particleShape;
	if (argData.isFlagSet(kParticleL) )
	{
		argData.getFlagArgument(kParticleL, 0, particleShape );
		if (particleShape == "")
		{
			printUsage();
			MGlobal::displayError("Please supply particleShape argument" );
			return MStatus::kFailure;
		}
	}
	else
	{
		makeParticle = true;
	}

	if( argData.isFlagSet(kFlipFlagL) || argData.isFlagSet(kFlipFlagS))
	{
	}

	/// parse attribute  flags
	unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );

	/// loop thru the rest of the attributes given
	MStringArray  attrNames;

	//attrNames.append(MString("id"));
	//attrNames.append(MString("position"));

	bool worldVeloCheck = false;

	for( unsigned int i = 0; i < numUses; i++ )
	{
		MArgList argList;
		status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
		if( !status ) return status;

		MString AttrName = argList.asString( 0, &status );
		if( !status ) return status;

		if( AttrName == "position" || AttrName == "worldPosition"  ||
			AttrName == "id" || AttrName == "particleId")
		{}

		else if( AttrName == "worldVelocity" || AttrName == "velocity" )
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

	MString particleCache; // particleCache file
	argData.getCommandArgument(0, particleCache);

	if(!partio4Maya::partioCacheExists(particleCache.asChar()))
	{
		MGlobal::displayError("Particle Cache Does not exist");
		return MStatus::kFailure;
	}

	if (makeParticle)
	{
		MStringArray foo;
		MGlobal::executeCommand("particle -n partioImport", foo);
		particleShape = foo[1];
	}

	MSelectionList list;
	list.add(particleShape);
	MObject objNode;
	list.getDependNode(0, objNode);

	if( objNode.apiType() != MFn::kParticle && objNode.apiType() != MFn::kNParticle )
	{
		MGlobal::displayError("PartioImport-> can't find your PARTICLESHAPE.");
		return MStatus::kFailure;
	}

	MStatus stat;
	MFnParticleSystem partSys(objNode, &stat);
	MString partName = partSys.particleName();

	if (stat == MStatus::kSuccess) // particle object was found and attached to
	{
		Partio::ParticlesDataMutable* particles;
		Partio::ParticleAttribute positionAttr;
		Partio::ParticleAttribute velocityAttr;
		particles=read(particleCache.asChar());
		bool hasVelo = true;

		if (!particles || particles->numParticles() <=0)
		{
			MGlobal::displayError("Particle Cache cannot be read, or does not Contain any particles");
			return MStatus::kFailure;
		}

		char partCount[50];
		sprintf (partCount, "%d", particles->numParticles());
		MGlobal::displayInfo(MString ("PartioImport-> LOADING: ") + partCount + MString (" particles"));

		if (!particles->attributeInfo("position",positionAttr) &&
			!particles->attributeInfo("Position",positionAttr))
		{
			MGlobal::displayError("PartioImport->Failed to find position attribute ");
			return ( MS::kFailure );
		}
		if (!particles->attributeInfo("velocity",velocityAttr) &&
			!particles->attributeInfo("Velocity",velocityAttr) &&
			!particles->attributeInfo("vel",velocityAttr) &&
			!particles->attributeInfo("Vel",velocityAttr))
		{
			MGlobal::displayWarning("PartioImport->Failed to find Velocity attribute ");
			hasVelo = false;
		}

		MPointArray positons;
		MVectorArray velocities;
		std::map<std::string,  MVectorArray  > vectorAttrArrays;
		std::map<std::string,  MDoubleArray  > doubleAttrArrays;

		for (unsigned int i=0;i<attrNames.length();i++)
		{
			cout << attrNames[i] << endl;
			Partio::ParticleAttribute testAttr;
			if (particles->attributeInfo(attrNames[i].asChar(), testAttr))
			{

				if (testAttr.count == 3)
				{
					if (!partSys.isPerParticleVectorAttribute(attrNames[i]))
					{
						MGlobal::displayInfo(MString("partioImport->adding ppAttr " + attrNames[i]) );
						MString command;
						command += "pioEmAddPPAttr ";
						command += attrNames[i];
						command += " vectorArray ";
						command += partName;
						command += ";";
						MGlobal::executeCommandOnIdle(command);
					}

					MVectorArray vAttribute;
					vectorAttrArrays[attrNames[i].asChar()] = vAttribute;
				}
				else if (testAttr.count == 1)
				{
					if (!partSys.isPerParticleDoubleAttribute(attrNames[i]))
					{
						MGlobal::displayInfo(MString("PartioEmiter->adding ppAttr " + attrNames[i]));
						MString command;
						command += "pioEmAddPPAttr ";
						command += attrNames[i];
						command += " doubleArray ";
						command += partName;
						command += ";";
						MGlobal::executeCommandOnIdle(command);
					}

					MDoubleArray dAttribute;
					doubleAttrArrays[attrNames[i].asChar()] = dAttribute;
				}
				else
				{
					MGlobal::displayError(MString("PartioEmitter->skipping attr: " + MString(attrNames[i])));
				}
			}
		}

////////////////////////////////////////////////
///  final particle loop
		for (int i=0;i<particles->numParticles();i++)
		{
			const float * partioPositions = particles->data<float>(positionAttr,i);
			MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
			positons.append(pos);
			if (hasVelo)
			{
				const float * partioVelocities = particles->data<float>(velocityAttr,i);
				MPoint vel (partioVelocities[0], partioVelocities[1], partioVelocities[2]);
				velocities.append(vel);
			}
			else
			{
				velocities.setLength(particles->numParticles());
			}
		}
////////////////////////////////////////////////

		if (particles)
		{
			particles->release();
		}
		partSys.emit(positons,velocities);
		partSys.saveInitialState();
	}
	else
	{
		return stat;
	}

	return MStatus::kSuccess;
}

void PartioImport::printUsage()
{

	MString usage = "\n-----------------------------------------------------------------------------\n";
	usage += "\tpartioImport -p/particle <particleShapeName> [Options] </full/path/to/particleCacheFile> \n";
	usage += "\n";
	usage += "\t[Options]\n";
	usage += "\t\t-atr/attribute (multi use)  <PP attribute name>\n";
	usage += "\t\t     (position/velocity/id) are always imported \n";
	usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
	usage += "\n";
	usage += "\tExample:\n";
	usage += "\n";
	usage += "partioImport -p particleShape1 -atr rgbPP -at opacityPP  \"/file/path/to/fooBar.0001.prt\"  \n\n";

	MGlobal::displayInfo(usage);

}








