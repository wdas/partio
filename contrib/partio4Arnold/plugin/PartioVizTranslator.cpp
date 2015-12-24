#include "PartioVizTranslator.h"

#include "attributes/AttrHelper.h"

#include <ai_msg.h>
#include <ai_nodes.h>
#include <ai_ray.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>
#include <maya/MString.h>
#include <maya/MFloatVector.h>
#include <maya/MFileObject.h>

void* CPartioVizTranslator::creator()
{
    return new CPartioVizTranslator();
}

void CPartioVizTranslator::NodeInitializer(CAbTranslator context)
{
    CExtensionAttrHelper helper(context.maya, "procedural");
    CShapeTranslator::MakeCommonAttributes(helper);

    CAttrData data;

    MStringArray enumNames;
    enumNames.append("points");
    enumNames.append("spheres");
    enumNames.append("quads");
    data.defaultValue.INT = 0;
    data.name = "aiRenderPointsAs";
    data.shortName = "ai_render_points_as";
    data.enums = enumNames;
    helper.MakeInputEnum(data);

    data.defaultValue.BOOL = false;
    data.name = "aiOverrideRadiusPP";
    data.shortName = "ai_override_radiusPP";
    helper.MakeInputBoolean(data);

    data.defaultValue.FLT = 1000000.0f;
    data.name = "aiMaxParticleRadius";
    data.shortName = "ai_max_particle_radius";
    data.hasMin = true;
    data.min.FLT = 0.0f;
    data.hasSoftMax = true;
    data.softMax.FLT = 1000000.0f;
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 0.0f;
    data.name = "aiMinParticleRadius";
    data.shortName = "ai_min_particle_radius";
    data.softMax.FLT = 1.0f;
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 0.2f;
    data.name = "aiRadius";
    data.shortName = "ai_radius";
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 1.0f;
    data.name = "aiRadiusMultiplier";
    data.shortName = "ai_radius_multiplier";
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 1.0f;
    data.name = "aiMotionBlurMultiplier";
    data.shortName = "ai_motion_blur_multiplier";
    data.hasMin = false;
    data.hasSoftMin = true;
    data.softMin.FLT = 0.0f;
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 0.f;
    data.name = "aiStepSize";
    data.shortName = "ai_step_size";
    data.hasMin = true;
    data.min.FLT = 0.f;
    data.hasSoftMax = true;
    data.softMax.FLT = 2.f;
    data.hasSoftMin = false;
    helper.MakeInputFloat(data);

    data.defaultValue.FLT = 8.0f;
    data.name = "aiFilterSmallParticles";
    data.shortName = "ai_filter_small_particles";
    data.hasMin = true;
    data.min.FLT = 0.0f;
    data.hasSoftMax = true;
    data.softMax.FLT = 10.0f;
    data.hasSoftMin = true;
    data.softMin.FLT = 7.0f;
    helper.MakeInputFloat(data);

    data.defaultValue.STR = "";
    data.name = "aiExportAttributes";
    data.shortName = "ai_export_attributes";
    helper.MakeInputString(data);

    data.defaultValue.STR = "";
    data.name = "aiOverrideProcedural";
    data.shortName = "ai_override_procedural";
    helper.MakeInputString(data);
}

AtNode* CPartioVizTranslator::CreateArnoldNodes()
{
    return IsMasterInstance() ? AddArnoldNode("procedural") : AddArnoldNode("ginstance");
}

void CPartioVizTranslator::Export(AtNode* anode)
{
    if (AiNodeIs(anode, "ginstance"))
        ExportInstance(anode, GetMasterInstance());
    else
        ExportProcedural(anode, false);
}

void CPartioVizTranslator::ExportMotion(AtNode* anode, unsigned int step)
{
    ExportMatrix(anode, step);
}

void CPartioVizTranslator::Update(AtNode* anode)
{
    if (AiNodeIs(anode, "ginstance"))
        ExportInstance(anode, GetMasterInstance());
    else
        ExportProcedural(anode, true);
}

void CPartioVizTranslator::UpdateMotion(AtNode* anode, unsigned int step)
{
    ExportMatrix(anode, step);
}

// Deprecated : Arnold support procedural instance, but it's not safe.
//
AtNode* CPartioVizTranslator::ExportInstance(AtNode* instance, const MDagPath& masterInstance)
{
    AtNode* masterNode = AiNodeLookUpByName(masterInstance.partialPathName().asChar());

    AiNodeSetStr(instance, "name", m_dagPath.partialPathName().asChar());

    ExportMatrix(instance, 0);

    AiNodeSetPtr(instance, "node", masterNode);
    AiNodeSetBool(instance, "inherit_xform", false);

    int visibility = AiNodeGetInt(masterNode, "visibility");
    AiNodeSetInt(instance, "visibility", visibility);

    m_DagNode.setObject(masterInstance);

    ExportPartioVizShaders(instance);

    return instance;
}

void CPartioVizTranslator::ExportShaders()
{
    AiMsgWarning("[mtoa] Shaders untested with new multitranslator and standin code.");
    /// TODO: Test shaders with standins.
    ExportPartioVizShaders(GetArnoldRootNode());
}

void CPartioVizTranslator::ExportPartioVizShaders(AtNode* procedural)
{
    int instanceNum = m_dagPath.isInstanced() ? m_dagPath.instanceNumber() : 0;

    std::vector<AtNode*> meshShaders;

    MPlug shadingGroupPlug = GetNodeShadingGroup(m_dagPath.node(), instanceNum);
    if (!shadingGroupPlug.isNull())
    {
        AtNode* shader = ExportNode(shadingGroupPlug);
        if (shader != 0)
        {
            AiNodeSetPtr(procedural, "shader", shader);
            meshShaders.push_back(shader);
        }
        else
        {
            AiMsgWarning("[mtoa] [translator %s] ShadingGroup %s has no surfaceShader input",
                         GetTranslatorName().asChar(), MFnDependencyNode(shadingGroupPlug.node()).name().asChar());
            /*AiMsgWarning("[mtoa] ShadingGroup %s has no surfaceShader input.",
                  fnDGNode.name().asChar());*/
            AiNodeSetPtr(procedural, "shader", 0);
        }
    }
}

// THIS MAY NOT REALLY BE NEEDED ANYMORE BUT LEAVING IT FOR NOW
void CPartioVizTranslator::ExportBoundingBox(AtNode* procedural)
{
    MBoundingBox boundingBox = m_DagNode.boundingBox();
    MPoint bbMin = boundingBox.min();
    MPoint bbMax = boundingBox.max();

    float minCoords[4];
    float maxCoords[4];

    bbMin.get(minCoords);
    bbMax.get(maxCoords);

    AiNodeSetPnt(procedural, "min", minCoords[0], minCoords[1], minCoords[2]);
    AiNodeSetPnt(procedural, "max", maxCoords[0], maxCoords[1], maxCoords[2]);

}


AtNode* CPartioVizTranslator::ExportProcedural(AtNode* procedural, bool update)
{
    m_DagNode.setObject(m_dagPath.node());

    AiNodeSetStr(procedural, "name", m_dagPath.partialPathName().asChar());

    ExportMatrix(procedural, 0);
    ProcessRenderFlags(procedural);

    ExportPartioVizShaders(procedural);


    if (!update)
    {
        /// TODO: figure out how to  use a  env variable to path just the .so name correctly
        //MFileObject envProcFilePath;
        //envProcFilePath.setRawPath("${MTOA_PROCEDURALS_PATH}");
        //MString envProcPath =envProcFilePath.resolvedPath();
        //MString dso = envProcPath+(MString("/partioGenerator.so"));

        MString dso = "partioGenerator.so";

        // we add this here so we can add in a custom   particle reading procedural instead of the default one
        MString overrideProc = m_DagNode.findPlug("aiOverrideProcedural").asString();

        if (overrideProc.length() > 0)
            dso = overrideProc;

        MString formattedName = m_DagNode.findPlug("renderCachePath").asString();
        int frameNum = m_DagNode.findPlug("time").asInt();
        int frameOffset = m_DagNode.findPlug("cacheOffset").asInt();
        int byFrame = m_DagNode.findPlug("byFrame").asInt();

        int finalFrame = (frameNum + frameOffset) * byFrame;

        /// TODO:  this is only temporary
        /// need to make the node actually update itself properly
        MString formatExt = formattedName.substring((formattedName.length() - 3), (formattedName.length() - 1));
        int cachePadding = 4;

        MString formatString = "%0";
        // special case for PDCs and maya nCache files because of the funky naming convention  TODO: support substepped/retiming  caches
        if (formatExt == "pdc")
        {
            finalFrame *= 6000 / 24;
            cachePadding = 1;
        }

        char frameString[10] = "";

        formatString += cachePadding;
        formatString += "d";

        const char* fmt = formatString.asChar();

        sprintf(frameString, fmt, finalFrame);

        MString frameNumFormattedName;

        MStringArray foo;

        formattedName.split('<', foo);
        MString newFoo;
        if (foo.length() > 1)
        {
            foo[1] = foo[1].substring(6, foo[1].length() - 1);
            newFoo = foo[0] + MString(frameString) + foo[1];
        }
        else
            newFoo = foo[0];

        if (!fileCacheExists(newFoo.asChar()))
        {
            AiMsgWarning("[mtoa] PartioVisualizer %s being skipped, can't find cache file.", m_DagNode.name().asChar());
            return 0;
        }

        // TODO: change these attributes to FindMayaPlug to support MtoA override nodes

        AiNodeSetStr(procedural, "dso", dso.asChar());
        AiNodeSetBool(procedural, "load_at_init", true);

        AiNodeDeclare(procedural, "arg_file", "constant STRING");
        AiNodeSetStr(procedural, "arg_file", newFoo.asChar());

        AiNodeDeclare(procedural, "arg_radius", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_radius", m_DagNode.findPlug("defaultRadius").asFloat());

        AiNodeDeclare(procedural, "arg_maxParticleRadius", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_maxParticleRadius", m_DagNode.findPlug("aiMaxParticleRadius").asFloat());

        AiNodeDeclare(procedural, "arg_minParticleRadius", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_minParticleRadius", m_DagNode.findPlug("aiMinParticleRadius").asFloat());

        AiNodeDeclare(procedural, "arg_filterSmallParticles", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_filterSmallParticles", m_DagNode.findPlug("aiFilterSmallParticles").asFloat());

        AiNodeDeclare(procedural, "overrideRadiusPP", "constant BOOL");
        AiNodeSetBool(procedural, "overrideRadiusPP", m_DagNode.findPlug("aiOverrideRadiusPP").asBool());

        AiNodeDeclare(procedural, "global_motionBlurSteps", "constant INT");
        AiNodeSetInt(procedural, "global_motionBlurSteps", (int)GetNumMotionSteps());

        AiNodeDeclare(procedural, "global_motionByFrame", "constant FLOAT");
        AiNodeSetFlt(procedural, "global_motionByFrame", (float)GetMotionByFrame());

        MTime oneSec(1.0, MTime::kSeconds);
        float fps = (float)oneSec.asUnits(MTime::uiUnit());
        AiNodeDeclare(procedural, "global_fps", "constant FLOAT");
        AiNodeSetFlt(procedural, "global_fps", fps);

        AiNodeDeclare(procedural, "arg_motionBlurMult", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_motionBlurMult", m_DagNode.findPlug("aiMotionBlurMultiplier").asFloat());

        AiNodeDeclare(procedural, "arg_renderType", "constant INT");
        AiNodeSetInt(procedural, "arg_renderType", m_DagNode.findPlug("aiRenderPointsAs").asInt());

        AiNodeDeclare(procedural, "arg_radiusMult", "constant FLOAT");
        AiNodeSetFlt(procedural, "arg_radiusMult", m_DagNode.findPlug("aiRadiusMultiplier").asFloat());

        AiNodeDeclare(procedural, "arg_velFrom", "constant STRING");
        AiNodeDeclare(procedural, "arg_accFrom", "constant STRING");
        AiNodeDeclare(procedural, "arg_rgbFrom", "constant STRING");
        AiNodeDeclare(procedural, "arg_opacFrom", "constant STRING");
        AiNodeDeclare(procedural, "arg_radFrom", "constant STRING");
        AiNodeDeclare(procedural, "arg_incandFrom", "constant STRING");

        MPlug partioAttrs = m_DagNode.findPlug("partioCacheAttributes");

        AiNodeSetStr(procedural, "arg_velFrom", m_DagNode.findPlug("velocityFrom").asString().asChar());
        AiNodeSetStr(procedural, "arg_accFrom", m_DagNode.findPlug("accelerationFrom").asString().asChar());
        AiNodeSetStr(procedural, "arg_rgbFrom", m_DagNode.findPlug("colorFrom").asString().asChar());
        AiNodeSetStr(procedural, "arg_opacFrom", m_DagNode.findPlug("opacityFrom").asString().asChar());
        AiNodeSetStr(procedural, "arg_radFrom", m_DagNode.findPlug("radiusFrom").asString().asChar());
        AiNodeSetStr(procedural, "arg_incandFrom", m_DagNode.findPlug("incandescenceFrom").asString().asChar());

        MFloatVector defaultColor;

        MPlug defaultColorPlug = m_DagNode.findPlug("defaultPointColor");

        MPlug defaultColorComp = defaultColorPlug.child(0);
        defaultColorComp.getValue(defaultColor.x);
        defaultColorComp = defaultColorPlug.child(1);
        defaultColorComp.getValue(defaultColor.y);
        defaultColorComp = defaultColorPlug.child(2);
        defaultColorComp.getValue(defaultColor.z);

        float defaultOpac = m_DagNode.findPlug("defaultAlpha").asFloat();
        AiNodeDeclare(procedural, "arg_defaultColor", "constant RGB");
        AiNodeDeclare(procedural, "arg_defaultOpac", "constant FLOAT");

        AiNodeSetRGB(procedural, "arg_defaultColor", defaultColor.x, defaultColor.y, defaultColor.z);
        AiNodeSetFlt(procedural, "arg_defaultOpac", defaultOpac);

        /////////////////////////////////////////
        // support  exporting volume step size
        float stepSize = m_DagNode.findPlug("aiStepSize").asFloat();
        if (stepSize > 0)
        {
            AiNodeDeclare(procedural, "arg_stepSize", "constant FLOAT");
            AiNodeSetFlt(procedural, "arg_stepSize", stepSize);
        }

        m_customAttrs = m_DagNode.findPlug("aiExportAttributes").asString();

        if (m_customAttrs.length() > 0)
        {
            AiNodeDeclare(procedural, "arg_extraPPAttrs", "constant STRING");
            AiNodeSetStr(procedural, "arg_extraPPAttrs", m_customAttrs.asChar());
        }

        /// right now because we're using  load at init, we don't need to export the bounding box
        //ExportBoundingBox(procedural);

    }
    return procedural;
}


bool CPartioVizTranslator::fileCacheExists(const char* fileName)
{
    struct stat fileInfo;
    bool statReturn;
    int intStat;

    intStat = stat(fileName, &fileInfo);
    if (intStat == 0)
        statReturn = true;
    else
        statReturn = false;

    return (statReturn);
}
