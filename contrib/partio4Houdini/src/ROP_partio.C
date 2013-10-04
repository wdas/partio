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


// Notes: This is the main body of code for the ROP node: this node emulates the regular geometry ROP node, but just exports to partio formats

//#include <UT/UT_DSOVersion.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <SOP/SOP_Node.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <UT/UT_IOTable.h>
#include "ROP_partio.h"
#include "partio_houdini.h"


using namespace	HDK_Sample;

// Parm Names
static PRM_Name		sopPathName("soppath",	"SOP Path");
static PRM_Name		theFileName("sopoutput", "Output File");
static PRM_Name		verbosityName("verbosity","Verbosity");
static PRM_Name		tRangeName("f",	"Start/End/inc");
static PRM_Name		theVerbosityTypes[] =
{
    PRM_Name("errors", "Errors only"),
    PRM_Name("level1", "Level 1"),
    PRM_Name("level2", "Level 2"),
    PRM_Name("level3", "Level 3"),
    PRM_Name("level4", "Level 4"),
    PRM_Name()
};

// Parm Defaults
static PRM_ChoiceList 	theVerbosityMenu(PRM_CHOICELIST_SINGLE, theVerbosityTypes);
static PRM_Default	 	theFileDefault(0, "$HIP/partio.$F.pdc");
static PRM_Name			alfprogressName("alfprogress", "Alfred Style Progress");
static PRM_Default		verbosityDefault(0);		// set verbosity to 0


// Parm Template
static PRM_Template	 f3dTemplates[] = {
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &sopPathName,0, 0, 0, 0, &PRM_SpareData::sopPath),
    PRM_Template(PRM_FILE,	1, &theFileName, &theFileDefault,0,	0, 0, &PRM_SpareData::fileChooserModeWrite),
    PRM_Template(PRM_TOGGLE, 1, &alfprogressName, PRMzeroDefaults),
    PRM_Template(PRM_ORD, 1, &verbosityName, &verbosityDefault,&theVerbosityMenu)
};


static PRM_Template *
getTemplates()
{
    static PRM_Template	*theTemplate = 0;

    if (theTemplate)
        return theTemplate;

    theTemplate = new PRM_Template[ROP_PIO_MAXPARMS+1];
    theTemplate[ROP_PIO_RENDER] = theRopTemplates[ROP_RENDER_TPLATE];
    theTemplate[ROP_PIO_RENDER_CTRL] = theRopTemplates[ROP_RENDERDIALOG_TPLATE];
    theTemplate[ROP_PIO_TRANGE] = theRopTemplates[ROP_TRANGE_TPLATE];
    theTemplate[ROP_PIO_FRANGE] = theRopTemplates[ROP_FRAMERANGE_TPLATE];
    theTemplate[ROP_PIO_TAKE] = theRopTemplates[ROP_TAKENAME_TPLATE];
    theTemplate[ROP_PIO_SOPPATH] = f3dTemplates[0];
    theTemplate[ROP_PIO_SOPOUTPUT] = f3dTemplates[1];
    theTemplate[ROP_PIO_VERBOSITY] = f3dTemplates[3];
    theTemplate[ROP_PIO_INITSIM] = theRopTemplates[ROP_INITSIM_TPLATE];
    theTemplate[ROP_PIO_ALFPROGRESS] = f3dTemplates[2];
    theTemplate[ROP_PIO_TPRERENDER] = theRopTemplates[ROP_TPRERENDER_TPLATE];
    theTemplate[ROP_PIO_PRERENDER] = theRopTemplates[ROP_PRERENDER_TPLATE];
    theTemplate[ROP_PIO_LPRERENDER] = theRopTemplates[ROP_LPRERENDER_TPLATE];
    theTemplate[ROP_PIO_TPREFRAME] = theRopTemplates[ROP_TPREFRAME_TPLATE];
    theTemplate[ROP_PIO_PREFRAME] = theRopTemplates[ROP_PREFRAME_TPLATE];
    theTemplate[ROP_PIO_LPREFRAME] = theRopTemplates[ROP_LPREFRAME_TPLATE];
    theTemplate[ROP_PIO_TPOSTFRAME] = theRopTemplates[ROP_TPOSTFRAME_TPLATE];
    theTemplate[ROP_PIO_POSTFRAME] = theRopTemplates[ROP_POSTFRAME_TPLATE];
    theTemplate[ROP_PIO_LPOSTFRAME] = theRopTemplates[ROP_LPOSTFRAME_TPLATE];
    theTemplate[ROP_PIO_TPOSTRENDER] = theRopTemplates[ROP_TPOSTRENDER_TPLATE];
    theTemplate[ROP_PIO_POSTRENDER] = theRopTemplates[ROP_POSTRENDER_TPLATE];
    theTemplate[ROP_PIO_LPOSTRENDER] = theRopTemplates[ROP_LPOSTRENDER_TPLATE];
    theTemplate[ROP_PIO_MAXPARMS] = PRM_Template();

    UT_ASSERT(PRM_Template::countTemplates(theTemplate) == ROP_PIO_MAXPARMS);

    return theTemplate;
}

OP_TemplatePair *
ROP_partio::getTemplatePair()
{
    static OP_TemplatePair *ropPair = 0;
    if (!ropPair)
    {
        ropPair = new OP_TemplatePair(getTemplates());
    }
    return ropPair;
}




OP_VariablePair *
ROP_partio::getVariablePair()
{
    static OP_VariablePair *pair = 0;
    if (!pair)
        pair = new OP_VariablePair(ROP_Node::myVariableList);
    return pair;
}

OP_Node *
ROP_partio::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{

    return new ROP_partio(net, name, op);
}

ROP_partio::ROP_partio(OP_Network *net, const char *name, OP_Operator *entry)
        : ROP_Node(net, name, entry)
{
}


ROP_partio::~ROP_partio()
{
}

//------------------------------------------------------------------------------
// The startRender(), renderFrame(), and endRender() render methods are
// invoked by Houdini when the ROP runs.

int
ROP_partio::startRender(int nframes, fpreal tstart, fpreal tend)
{
    int	rcode = 1;
    myEndTime = tend;
    myStartTime = tstart;
    UT_String filename;


    OUTPUTRAW(filename, 0);
    if (filename.isstring()==0)
    {
        addError(ROP_MESSAGE, "ROP can't write to invalid path");
        return ROP_ABORT_RENDER;
    }

    if (INITSIM())
    {
        initSimulationOPs();
        OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(1);
    }

    if (error() < UT_ERROR_ABORT)
    {
        if ( !executePreRenderScript(tstart) )
            return 0;
    }

    return rcode;
}

ROP_RENDER_CODE
ROP_partio::renderFrame(fpreal time, UT_Interrupt *)
{
    SOP_Node		*sop;
    UT_String		 soppath, savepath;


    if ( !executePreFrameScript(time) )
    {
        return ROP_ABORT_RENDER;
    }

    // From here, establish the SOP that will be rendered, if it cannot
    // be found, return 0.
    // This is needed to be done here as the SOPPATH may be time
    // dependent (ie: OUT$F) or the perframe script may have
    // rewired the input to this node.
    sop = CAST_SOPNODE(getInput(0));
    if ( sop )
    {
        // If we have an input, get the full path to that SOP.
        sop->getFullPath(soppath);
    }
    else
    {
        // Otherwise get the SOP Path from our parameter.
        SOPPATH(soppath, time);
    }

    if ( !soppath.isstring() )
    {
        UT_String filename;
        addError(ROP_MESSAGE, "Invalid SOP path");
        return ROP_ABORT_RENDER;
    }

    sop = getSOPNode(soppath, 1);
    if (!sop)
    {
        addError(ROP_COOK_ERROR, (const char *)soppath);
        return ROP_ABORT_RENDER;
    }

    // Get the gdp
    OP_Context		context(time);
    GU_DetailHandle gdh;
    gdh = sop->getCookedGeoHandle(context);
    GU_DetailHandleAutoReadLock	 gdl(gdh);
    const GU_Detail		*gdp = gdl.getGdp();
    if (!gdp)
    {
        addError(ROP_COOK_ERROR, (const char *)soppath);
        return ROP_ABORT_RENDER;
    }

    OUTPUT(savepath, time);
    UT_String filename;
    OUTPUT(filename, time);
    int verbosity = VERBOSITY();
    int successWrite = partioSave(filename, gdp, verbosity);
    if (!successWrite)
    {
        addError(ROP_MESSAGE, "ROP can't write to invalid path");
        return ROP_ABORT_RENDER;
    }

    //update progess
    if (ALFPROGRESS() && (myEndTime != myStartTime))
    {
        float		fpercent = (time - myStartTime) / (myEndTime - myStartTime);
        int		percent = (int)SYSrint(fpercent * 100);
        percent = SYSclamp(percent, 0, 100);
        fprintf(stdout, "ALF_PROGRESS %d%%\n", percent);
        fflush(stdout);
    }

    if (error() < UT_ERROR_ABORT)
    {
        if ( !executePostFrameScript(time) )
        {
            return ROP_ABORT_RENDER;
        }
    }

    return ROP_CONTINUE_RENDER;
}



ROP_RENDER_CODE
ROP_partio::endRender()
{
    if (INITSIM())
        OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(-1);

    if (error() < UT_ERROR_ABORT)
    {
        if ( !executePostRenderScript(myEndTime) )
            return ROP_ABORT_RENDER;
    }
    return ROP_CONTINUE_RENDER;
}

void
newDriverOperator(OP_OperatorTable *table)
{
    //Field3D::initIO();

    table->addOperator(new OP_Operator("partio",
                                       "partio",
                                       ROP_partio::myConstructor,
                                       ROP_partio::getTemplatePair(),
                                       1,
                                       0,
                                       ROP_partio::getVariablePair(),
                                       OP_FLAG_GENERATOR));

    // Note due to the just-in-time loading of GeometryIO, the f3d
    // won't be added until after your first f3d save/load.
    // Thus this is replicated in the newDriverOperator.
    UT_ExtensionList		*geoextension;
    geoextension = UTgetGeoExtensions();
    if (!geoextension->findExtension("pdb"))
        geoextension->addExtension("pdb");
    if (!geoextension->findExtension("ptc"))
        geoextension->addExtension("ptc");
    if (!geoextension->findExtension("pdc"))
        geoextension->addExtension("pdc");
    if (!geoextension->findExtension("mc"))
        geoextension->addExtension("mc");
    if (!geoextension->findExtension("bin"))
        geoextension->addExtension("bin");
}

