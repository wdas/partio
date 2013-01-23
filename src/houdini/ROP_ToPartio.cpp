/*
Copyright Disney Enterprises, Inc. All rights reserved.

This license governs use of the accompanying software. If you use the software, you
accept this license. If you do not accept the license, do not use the software.

1. Definitions
The terms "reproduce," "reproduction," "derivative works," and "distribution" have
the same meaning here as under U.S. copyright law. A "contribution" is the original
software, or any additions or changes to the software. A "contributor" is any person
that distributes its contribution under this license. "Licensed patents" are a
contributor's patent claims that read directly on its contribution.

2. Grant of Rights
(A) Copyright Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free copyright license to reproduce its contribution, prepare
derivative works of its contribution, and distribute its contribution or any derivative
works that you create.
(B) Patent Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free license under its licensed patents to make, have made,
use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the
software or derivative works of the contribution in the software.

3. Conditions and Limitations
(A) No Trademark License- This license does not grant you rights to use any
contributors' name, logo, or trademarks.
(B) If you bring a patent claim against any contributor over patents that you claim
are infringed by the software, your patent license from such contributor to the
software ends automatically.
(C) If you distribute any portion of the software, you must retain all copyright,
patent, trademark, and attribution notices that are present in the software.
(D) If you distribute any portion of the software in source code form, you may do
so only under this license by including a complete copy of this license with your
distribution. If you distribute any portion of the software in compiled or object code
form, you may only do so under a license that complies with this license.
(E) The software is licensed "as-is." You bear the risk of using it. The contributors
give no express warranties, guarantees or conditions. You may have additional
consumer rights under your local laws which this license cannot change.
To the extent permitted under your local laws, the contributors exclude the
implied warranties of merchantability, fitness for a particular purpose and non-
infringement.
*/
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_OperatorTable.h>
#include <SOP/SOP_Node.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <UT/UT_Version.h>

#include <Partio.h>

#include "ROP_ToPartio.h"

static PRM_Name sopPathName("soppath", "SOP Path");
static PRM_Name theFileName("file", "Output File");
static PRM_Default theFileDefault(0, "$HIP/$F4.bgeo");

static PRM_Template partioTemplates[] = {
    PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &sopPathName, 0, 0, 0, 0, &PRM_SpareData::sopPath),
    PRM_Template(PRM_FILE,1, &theFileName, &theFileDefault,0, 0, 0, &PRM_SpareData::fileChooserModeWrite),
    PRM_Template()
};

static PRM_Template *
getTemplates()
{
    static PRM_Template	*theTemplate = 0;

    if (theTemplate)
	return theTemplate;

    theTemplate = new PRM_Template[20];
    theTemplate[0] = theRopTemplates[ROP_RENDER_TPLATE];
    theTemplate[1] = theRopTemplates[ROP_RENDERDIALOG_TPLATE];
    theTemplate[2] = theRopTemplates[ROP_TRANGE_TPLATE];
    theTemplate[3] = theRopTemplates[ROP_FRAMERANGE_TPLATE];
    theTemplate[4] = theRopTemplates[ROP_TAKENAME_TPLATE];
    theTemplate[5] = partioTemplates[0];
    theTemplate[6] = partioTemplates[1];
    theTemplate[7] = theRopTemplates[ROP_TPRERENDER_TPLATE];
    theTemplate[8] = theRopTemplates[ROP_PRERENDER_TPLATE];
    theTemplate[9] = theRopTemplates[ROP_LPRERENDER_TPLATE];
    theTemplate[10] = theRopTemplates[ROP_TPREFRAME_TPLATE];
    theTemplate[11] = theRopTemplates[ROP_PREFRAME_TPLATE];
    theTemplate[12] = theRopTemplates[ROP_LPREFRAME_TPLATE];
    theTemplate[13] = theRopTemplates[ROP_TPOSTFRAME_TPLATE];
    theTemplate[14] = theRopTemplates[ROP_POSTFRAME_TPLATE];
    theTemplate[15] = theRopTemplates[ROP_LPOSTFRAME_TPLATE];
    theTemplate[16] = theRopTemplates[ROP_TPOSTRENDER_TPLATE];
    theTemplate[17] = theRopTemplates[ROP_POSTRENDER_TPLATE];
    theTemplate[18] = theRopTemplates[ROP_LPOSTRENDER_TPLATE];
    theTemplate[19] = PRM_Template();

    UT_ASSERT(PRM_Template::countTemplates(theTemplate) == 19);

    return theTemplate;
}

OP_TemplatePair *
ROP_ToPartio::getTemplatePair()
{
    static OP_TemplatePair *ropPair = 0;
    if (!ropPair) {
	ropPair = new OP_TemplatePair(getTemplates());
    }
    return ropPair;
}

OP_Node *
ROP_ToPartio::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new ROP_ToPartio(net, name, entry);
}

ROP_ToPartio::ROP_ToPartio(OP_Network *net, const char *name, OP_Operator *entry) : ROP_Node(net, name, entry)
{
}

ROP_ToPartio::~ROP_ToPartio()
{
}

//------------------------------------------------------------------------------
// The startRender(), renderFrame(), and endRender() render methods are
// invoked by Houdini when the ROP runs.

int
ROP_ToPartio::startRender(int /*nframes*/, fpreal tstart, fpreal tend)
{
    myEndTime = tend;
    if (error() < UT_ERROR_ABORT) {
	if (!executePreRenderScript(tstart))
	    return 0;
    }
    return 1;
}

ROP_RENDER_CODE
ROP_ToPartio::renderFrame(fpreal time, UT_Interrupt *)
{
    SOP_Node *sop;
    UT_String soppath, savepath;

    if (!executePreFrameScript(time))
	return ROP_ABORT_RENDER;

    // From here, establish the SOP that will be rendered, if it cannot
    // be found, return 0.
    // This is needed to be done here as the SOPPATH may be time
    // dependent (ie: OUT$F) or the perframe script may have
    // rewired the input to this node.

    sop = CAST_SOPNODE(getInput(0));
    if( sop ) {
	// If we have an input, get the full path to that SOP.
	sop->getFullPath(soppath);
    } else {
	// Otherwise get the SOP Path from our parameter.
	SOPPATH(soppath, time);
    }

    if (!soppath.isstring())  {
	addError(ROP_MESSAGE, "Invalid SOP path");
	return ROP_ABORT_RENDER;
    }

    sop = getSOPNode(soppath, 1);
    if (!sop) {
	addError(ROP_COOK_ERROR, (const char *)soppath);
	return ROP_ABORT_RENDER;
    }
    OP_Context context(time);
    GU_DetailHandle gdh;
    gdh = sop->getCookedGeoHandle(context);

    GU_DetailHandleAutoReadLock gdl(gdh);
    const GU_Detail *gdp = gdl.getGdp();

    if (!gdp) {
	addError(ROP_COOK_ERROR, (const char *)soppath);
	return ROP_ABORT_RENDER;
    }

    OUTPUT(savepath, time);

    partio_fileSave(gdp, (const char *) savepath, time);

    if (error() < UT_ERROR_ABORT) {
	if( !executePostFrameScript(time) )
	    return ROP_ABORT_RENDER;
    }

    return ROP_CONTINUE_RENDER;
}

ROP_RENDER_CODE
ROP_ToPartio::endRender()
{
    if (error() < UT_ERROR_ABORT) {
	if (!executePostRenderScript(myEndTime))
	    return ROP_ABORT_RENDER;
    }
    return ROP_CONTINUE_RENDER;
}

bool
ROP_ToPartio::partio_fileSave(const GU_Detail *gdp, const char *fname, float t)
{
    Partio::ParticlesDataMutable *data = Partio::create();

    // create position attribute
    Partio::ParticleAttribute posAttr = data->addAttribute("position", Partio::VECTOR,3);

    // create other attributes
#if UT_MAJOR_VERSION_INT >= 12
    std::vector<std::pair<GA_ROAttributeRef, Partio::ParticleAttribute> > reftoattr;
    GA_Attribute *attr;
    GA_FOR_ALL_POINT_ATTRIBUTES(gdp, attr) {
        if (attr->getScope() != GA_SCOPE_PUBLIC) continue;
        if (!strcmp(attr->getName(), "P")) continue;
        GA_ROAttributeRef ref = gdp->findPointAttrib(*attr);
        Partio::ParticleAttributeType type;
        int tupleSize = 1;
        if (attr->getStorageClass() == GA_STORECLASS_FLOAT) {
            type = Partio::FLOAT;
            tupleSize = attr->getTupleSize();
        } else if (attr->getStorageClass() == GA_STORECLASS_INT) {
            type = Partio::INT;
        } else if (attr->getStorageClass() == GA_STORECLASS_STRING) {
            type = Partio::INDEXEDSTR;
        } else {
            std::cerr << "Attribute " << attr->getName() << " type " << attr->getType().getTypeName() << " not supported." << std::endl;
            continue;
        }
        Partio::ParticleAttribute pa = data->addAttribute(attr->getName(), type, tupleSize);
        reftoattr.push_back(std::make_pair(ref, pa));
    }
#else
    std::vector<std::pair<GB_AttributeRef, Partio::ParticleAttribute> > reftoattr;
    GB_Attribute *attr;
    FOR_ALL_POINT_ATTRIBUTES(gdp, attr) {
        GB_AttributeRef ref = gdp->findPointAttrib(attr);
        Partio::ParticleAttributeType type;
        int size;
        switch (attr->getType()) {
        case GB_ATTRIB_FLOAT:
            type = Partio::FLOAT;
            size = sizeof(float);
            break;
        case GB_ATTRIB_INT:
            type = Partio::INT;
            size = sizeof(int);
            break;
        case GB_ATTRIB_VECTOR:
            type = Partio::VECTOR;
            size = sizeof(float);
            break;
        //TODO: add string support
        case GB_ATTRIB_MIXED:
            continue;
        default:
            std::cerr << "Attribute " << attr->getName() << " type " << attr->getType() << " not supported." << std::endl;
            continue;
        }
        Partio::ParticleAttribute pa = data->addAttribute(attr->getName(), type, attr->getSize()/size);
        reftoattr.push_back(std::make_pair(ref, pa));
    }
#endif

    GEO_Point *ppt;
#if UT_MAJOR_VERSION_INT >= 12
    GA_FOR_ALL_GPOINTS(const_cast<GU_Detail *> (gdp), ppt) {
#else
    FOR_ALL_GPOINTS(const_cast<GU_Detail *> (gdp), ppt) {
#endif
        UT_Vector4 position = ppt->getPos();

        Partio::ParticleIndex index = data->addParticle();
        float *pos = data->dataWrite<float>(posAttr, index);
        pos[0] = position.x();
        pos[1] = position.y();
        pos[2] = position.z();
        for (int i = 0; i < reftoattr.size(); i++) {
            switch ((reftoattr[i]).second.type) {
            case Partio::VECTOR:
                {
                    UT_Vector3 value = ppt->getValue<UT_Vector3>((reftoattr[i]).first);
                    float *pv = data->dataWrite<float>((reftoattr[i]).second, index);
                    pv[0] = value[0];
                    pv[1] = value[1];
                    pv[2] = value[2];
                }
                break;
            case Partio::FLOAT:
                {
                    float *pv = data->dataWrite<float>((reftoattr[i]).second, index);
                    for (int j = 0; j < (reftoattr[i]).second.count; j++) {
                        pv[j] = ppt->getValue<float>((reftoattr[i]).first, j);
                    }
                }
                break;
            case Partio::INT:
                {
                    int *pv = data->dataWrite<int>((reftoattr[i]).second, index);
                    for (int j = 0; j < (reftoattr[i]).second.count; j++) {
                        pv[j] = ppt->getValue<int>((reftoattr[i]).first, j);
                    }
                }
                break;
            case Partio::INDEXEDSTR:
                {
#if UT_MAJOR_VERSION_INT >= 12
                    int index = ppt->getValue<int>(reftoattr[i].first,0);
                    const char *str = ppt->getString(reftoattr[i].first, index);
                    int token = data->registerIndexedStr(reftoattr[i].second, str);
                    int *pv = data->dataWrite<int>((reftoattr[i]).second, 0);
                    pv[0] = token;
#else
                    //TODO: add string support
#endif
                }
                break;
            case Partio::NONE:
            default:
                break;
            }
        }
    }
    Partio::write(fname, *data);
    data->release();
    return true;
}

void
newDriverOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("toPartio", "ToPartio", ROP_ToPartio::myConstructor, ROP_ToPartio::getTemplatePair(), 1, 1));
}
