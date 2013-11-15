/* partio4Houdini  5/01/2013, Miles Green

 PARTIO Import/Export
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


// Notes: This is the main body of code for the SOP partio node: this node can read and write partio formats - (it tries to emulate th eregular fiel node)

#include <string>
#include <limits.h>

#include <SOP/SOP_Node.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <CH/CH_Manager.h>

#include "SOP_partio.h"
#include "partio_houdini.h"


using namespace partio;


//
// Help is stored in a "wiki" style text file.  This text file should be copied
// to $HOUDINI_PATH/help/nodes/sop/star.txt
//
// See the sample_install.sh file for an example.
//

///
/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case we add ourselves
/// to the specified operator table.
///
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(
        new OP_Operator("partio",			// Internal name
                        "partio",			// UI name
                        SOP_partio::myConstructor,	// How to build the SOP
                        SOP_partio::myTemplateList,	// My parameters
                        0,				// Min # of sources
                        1,				// Max # of sources
                        SOP_partio::myVariables,	// Local variables
                        OP_FLAG_GENERATOR)		// Flag it as generator
    );
}
static PRM_Default	theFileDefault(0, "");
static PRM_Name		theioTypes[] =
{
    PRM_Name("readfiles", "Read Files"),
    PRM_Name("writefiles", "Write Files"),
    PRM_Name()
};
static PRM_Name		thedebugTypes[] =
{
    PRM_Name("None", "None"),
    PRM_Name("level 1", "level 1"),
    PRM_Name("level 2", "level 2"),
    PRM_Name("level 3", "level 3"),
    PRM_Name("level 4", "level 4"),
    PRM_Name()
};
static PRM_ChoiceList theioTypeMenu(PRM_CHOICELIST_SINGLE, theioTypes);
static PRM_ChoiceList theioDebugMenu(PRM_CHOICELIST_SINGLE, thedebugTypes);


static PRM_Name iotypeName("filemode", "File Mode");
static PRM_Name FileName("file", "File Path");
static PRM_Name fileVar("fileVar", "Filename varible"); //dummy attr to eval filename
static PRM_Name	debugName("dbug", "Debug to Shell");
static PRM_Name	refreshName("refresh", "Reload Geometry");

//				     ^^^^^^^^    ^^^^^^^^^^^^^^^
//				     internal    descriptive version

static PRM_Default	fileVarDefault(0,"$FN");		// set frame to $FN = filename
static PRM_Default	debugDefault(0);		// set debug to off
static PRM_Default	refreshDefault(0);		// set debug to off


PRM_Template
SOP_partio::myTemplateList[] = {
    PRM_Template(PRM_ORD, 1, &iotypeName, 0,&theioTypeMenu),
    PRM_Template(PRM_FILE, 1, &FileName,&theFileDefault),
    PRM_Template(PRM_ORD, 1, &debugName, 0,&theioDebugMenu),
    PRM_Template(PRM_CALLBACK, 1, &refreshName, 0, 0, 0, &refreshStatic),
    PRM_Template()
};


// Here's how we define local variables for the SOP.
enum {
    VAR_CP,	  // current point
};

CH_LocalVariable
SOP_partio::myVariables[] = {
    { "CP", VAR_CP,0},
    { 0,0,0}
};



bool SOP_partio::matchExtension(UT_String &fileName)
{

    if (!fileName.isstring())
    {
        // No string entered exit
        return false;
    }
    std::string supportedExtensions[] = {".bgeo", ".geo", ".pda", ".pdb", ".pdb.gz", ".pdc", ".bin", ".rpc", ".prt", ".ptc", ".pts", ".xyz", ".pcd", ".mc",".icecache", ".rib", };

    for (int i = 0; i < sizeof(supportedExtensions); ++i)
    {

        if (fileName.endsWith(supportedExtensions[i].c_str()))
        {
            return true;
        }
    }

    // Did not find suported extensions
    return false;

}


int
SOP_partio::refreshStatic(void *op, int, fpreal, const PRM_Template *)
{

    SOP_partio *sop = (SOP_partio *)op;
    sop->refresh();
    return 1;
}

void
SOP_partio::refresh()
{
    forceRecook();
}

void SOP_partio::opChanged(OP_EventType reason, void *data)
{
    SOP_Node::opChanged(reason, data);
}


OP_Node *
SOP_partio::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_partio(net, name, op);
}

SOP_partio::SOP_partio(OP_Network *net, const char *name, OP_Operator *op)
        : SOP_Node(net, name, op)
{

    myCurrentFileName = "";

}

SOP_partio::~SOP_partio() {}



bool
SOP_partio::getHDKHelp(UT_String &str) const
{
    str.harden("This is some help for the partio SOP\n\n This node can read partio suported formats:\n bgeo,geo\n pda,pdb,pdb.gz,pdc\nbin,rpc\nprt\nptc\npts,xyz,pcd\nmc,icecache,rib caches\n");
    return true;
}



OP_ERROR
SOP_partio::cookMySop(OP_Context &context)
{



    fpreal now = context.getTime();
    int iomode = IOMODE(0);
    int verbosity = VERBOSITY(0);
    UT_String filename;
    FILEPATH(filename, now);
    UT_Interrupt        *boss;
    bool  successReadWrite = 0;

    // Check we have valid file name and extension
    if (!matchExtension(filename))
    {
        gdp->clearAndDestroy();
        addError(SOP_MESSAGE, "filename empty or filetype is not one of:\nbgeo,geo\npda,pdb,pdb.gz,pdc\nbin,rpc\nprt\nptc\npts,xyz,pcd\nmc,icecache,rib");
        gdp->notifyCache(GU_CACHE_ALL);
        return error();
    }

  myCurrentFileName = filename;
    // Check to see that there hasn't been a critical error in cooking the SOP.
    if (error() < UT_ERROR_ABORT)
    {
        boss = UTgetInterrupt();

        // Start the interrupt server
        if (boss->opStart("Building partio"))
        {
            if (iomode==0)
            {

                gdp->clearAndDestroy();
                successReadWrite = partioLoad(filename, gdp, verbosity);

                if (successReadWrite==0)
                {
                    addError(SOP_MESSAGE, "File not valid, unable to load file: ");
                }

            }
            else
            {
                if (lockInputs(context) >= UT_ERROR_ABORT)
                    return error();

                duplicateSource(0, context);
                flags().timeDep = 1;

                successReadWrite = partioSave(filename, gdp, verbosity);

                unlockInputs();

                if (successReadWrite==0)
                {
                    addError(SOP_MESSAGE, "Error, unable to save file: ");
                }
            }
        }

        // Tell the interrupt server that we've completed. Must do this
        // regardless of what opStart() returns.
        boss->opEnd();
    }

    gdp->notifyCache(GU_CACHE_ALL);
    return error();
}


/*unsigned
SOP_partio::disableParms()
{

    int     changed = 0;
    bool    off = false;
	bool    on = true;
	float   t = CHgetEvalTime();

	// If no input disable writing
	if(getInput (0)==0)
	{
	changed  = enableParm("filemode", off);
	setInt("filemode", 0, t,0);
	}
	else
	{
	changed  = enableParm("filemode", on);
	}

	error();
    return changed;




}*/



