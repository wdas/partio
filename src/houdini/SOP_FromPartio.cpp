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
#include <GEO/GEO_PrimPart.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_OperatorTable.h>
#include <UT/UT_Version.h>

#include <Partio.h>

#include "SOP_FromPartio.h"

void
SOP_FromPartio::registerOp(OP_OperatorTable* table)
{
    table->addOperator(new OP_Operator("fromPartio", "FromPartio",
                                       SOP_FromPartio::myConstructor,
                                       SOP_FromPartio::myTemplateList,
                                       0, 0));
}


static PRM_Name	sop_names[] = {
    PRM_Name("inputFile", "InputFile"),
    PRM_Name(0)
};

static PRM_Default fileDef(0, "blah.bgeo");

PRM_Template
SOP_FromPartio::myTemplateList[]=
{
    PRM_Template(PRM_FILE, 1, &sop_names[0], &fileDef, 0, 0, 0, &PRM_SpareData::fileChooserModeRead),
    PRM_Template()
};


OP_Node *
SOP_FromPartio::myConstructor(OP_Network *net,const char *name,OP_Operator *entry)
{
    return new SOP_FromPartio(net, name, entry);
}


SOP_FromPartio::SOP_FromPartio(OP_Network *net, const char *name, OP_Operator *entry) : SOP_Node(net, name, entry)
{
}

SOP_FromPartio::~SOP_FromPartio()
{
}


OP_ERROR
SOP_FromPartio::cookMySop(OP_Context &context)
{
    // Before we do anything, we must lock our inputs.  Before returning,
    //	we have to make sure that the inputs get unlocked.
    if (lockInputs(context) >= UT_ERROR_ABORT)
	return error();

    gdp->clearAndDestroy();

    float t = context.getTime();
    UT_String inputFile;
    evalString(inputFile, "inputFile", 0, t);

    Partio::ParticlesInfo *info = Partio::readHeaders(inputFile.buffer());
    if (!info || !info->numParticles()) {
        unlockInputs();
        return error();
    }
    int numAttr = info->numAttributes();
#if UT_MAJOR_VERSION_INT >= 12
    std::vector<std::pair<Partio::ParticleAttribute, GA_RWAttributeRef> > attrtoref;
    Partio::ParticleAttribute posAttr;
    bool hasPos = false;
    for (int i = 0; i < numAttr; i++) {
        Partio::ParticleAttribute attr;
        info->attributeInfo(i, attr);
        switch (attr.type) {
        case Partio::VECTOR:
            {
                if (attr.name == "position") {
                    posAttr = attr;
                    hasPos = true;
                } else {
                    GA_RWAttributeRef ref = gdp->addFloatTuple(GA_ATTRIB_POINT, attr.name.c_str(), 3, GA_Defaults(0));
                    attrtoref.push_back(std::make_pair(attr, ref));
                }
            }
            break;
        case Partio::FLOAT:
            {
                GA_RWAttributeRef ref = gdp->addFloatTuple(GA_ATTRIB_POINT, attr.name.c_str(), attr.count, GA_Defaults(0));
                attrtoref.push_back(std::make_pair(attr, ref));
            }
            break;
        case Partio::INT:
            {
                GA_RWAttributeRef ref = gdp->addIntTuple(GA_ATTRIB_POINT, attr.name.c_str(), attr.count, GA_Defaults(0));
                attrtoref.push_back(std::make_pair(attr, ref));
            }
            break;
        case Partio::INDEXEDSTR:
            {
                GA_RWAttributeRef ref = gdp->addStringTuple(GA_ATTRIB_POINT, attr.name.c_str(), attr.count);
                attrtoref.push_back(std::make_pair(attr, ref));
            }
            break;
        case Partio::NONE:
        default:
            break;
        }
    }
#else
    std::vector<std::pair<Partio::ParticleAttribute, GB_AttributeRef> > attrtoref;
    Partio::ParticleAttribute posAttr;
    bool hasPos = false;
    for (int i = 0; i < numAttr; i++) {
        Partio::ParticleAttribute attr;
        info->attributeInfo(i, attr);
        switch (attr.type) {
        case Partio::VECTOR:
            {
                if (attr.name == "position") {
                    posAttr = attr;
                    hasPos = true;
                } else {
                    GB_AttributeRef ref = gdp->addPointAttrib(attr.name.c_str(), 3*sizeof(float), GB_ATTRIB_VECTOR, 0);
                    attrtoref.push_back(std::make_pair(attr, ref));
                }
            }
            break;
        case Partio::FLOAT:
            {
                GB_AttributeRef ref = gdp->addPointAttrib(attr.name.c_str(), attr.count*sizeof(float), GB_ATTRIB_FLOAT, 0);
                attrtoref.push_back(std::make_pair(attr, ref));
            }
            break;
        case Partio::INT:
            {
                GB_AttributeRef ref = gdp->addPointAttrib(attr.name.c_str(), attr.count*sizeof(int), GB_ATTRIB_INT, 0);
                attrtoref.push_back(std::make_pair(attr, ref));
            }
            break;
        case Partio::INDEXEDSTR:
            //TODO: add string support
        case Partio::NONE:
        default:
            break;
        }
    }
#endif

    if (!hasPos) {
        std::cerr << "Particles missing position attribute." << std::endl;
        unlockInputs();
        return error();
    }

#if UT_MAJOR_VERSION_INT >= 12
    GEO_PrimParticle *prim = dynamic_cast<GEO_PrimParticle *> (gdp->appendPrimitive(GEO_PRIMPART));
#else
    GEO_PrimParticle *prim = dynamic_cast<GEO_PrimParticle *> (gdp->appendPrimitive(GEOPRIMPART));
#endif

    Partio::ParticlesData *data = Partio::read(inputFile.buffer());
    for (int i = 0; i < data->numParticles(); i++) {
        GEO_Point *ppt = gdp->appendPoint();
        //TODO: appendParticle is deprecated
        prim->appendParticle(ppt);
        const float *pos = data->data<float>(posAttr, i);
        ppt->setPos(pos[0], pos[1], pos[2]);

        for (int j = 0; j < attrtoref.size(); j++) {
            switch ((attrtoref[j]).first.type) {
            case Partio::VECTOR:
                {
                    const float *pv = data->data<float>(attrtoref[j].first, i);
                    ppt->set<float>(attrtoref[j].second, pv, attrtoref[j].first.count);
                }
                break;
            case Partio::FLOAT:
                {
                    const float *pv = data->data<float>(attrtoref[j].first, i);
                    ppt->set<float>(attrtoref[j].second, pv, attrtoref[j].first.count);
                }
                break;
            case Partio::INT:
                {
                    const int *pv = data->data<int>((attrtoref[j]).first, i);
                    ppt->set<int>((attrtoref[j]).second, pv, attrtoref[j].first.count);
                }
                break;
            case Partio::INDEXEDSTR:
                {
                    const std::vector<std::string> indexedStrs = data->indexedStrs(attrtoref[j].first);
                    if (!indexedStrs.empty()) {
                        int index=data->data<int>(attrtoref[j].first,i)[0];
#if UT_MAJOR_VERSION_INT >= 12
                        ppt->setString((attrtoref[j]).second, indexedStrs[index].c_str(), j);
#else
                        //TODO: add string support
#endif
                    }
                }
                break;
            case Partio::NONE:
            default:
                break;
            }
        }
    }

    data->release();

    unlockInputs();
    return error();
}
