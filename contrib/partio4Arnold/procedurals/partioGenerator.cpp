/////////////////////////////////////////////////////////////////////////////////////
/// PARTIO GENERATOR  an arnold procedural cache reader that uses the partio Library
/// by John Cassella (redpawfx)  for  Luma Pictures  (c) 2013

#include <ai.h>
#include <cstring>
#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>

template<typename T>
void getParam(T& param, AtNode* node, const char* paramName)
{
    // do nothing
}

template<>
void getParam<int>(int& param, AtNode* node, const char* paramName)
{
    if (AiNodeLookUpUserParameter(node, paramName) != 0)
        param = AiNodeGetInt(node, paramName);
}

template<>
void getParam<float>(float& param, AtNode* node, const char* paramName)
{
    if (AiNodeLookUpUserParameter(node, paramName) != 0)
        param = AiNodeGetFlt(node, paramName);
}

template<>
void getParam<bool>(bool& param, AtNode* node, const char* paramName)
{
    if (AiNodeLookUpUserParameter(node, paramName) != 0)
        param = AiNodeGetBool(node, paramName);
}

template<>
void getParam<std::string>(std::string& param, AtNode* node, const char* paramName)
{
    if (AiNodeLookUpUserParameter(node, paramName) != 0)
        param = AiNodeGetStr(node, paramName);
}

template<>
void getParam<AtRGB>(AtRGB& param, AtNode* node, const char* paramName)
{
    if (AiNodeLookUpUserParameter(node, paramName) != 0)
        param = AiNodeGetRGB(node, paramName);
}

struct PartioData {
    PARTIO::ParticlesData* points;
    AtNode* mynode;

    PARTIO::ParticleAttribute positionAttr;
    PARTIO::ParticleAttribute velocityAttr;
    PARTIO::ParticleAttribute accelerationAttr;
    PARTIO::ParticleAttribute rgbAttr;
    PARTIO::ParticleAttribute opacityAttr;
    PARTIO::ParticleAttribute radiusAttr;
    PARTIO::ParticleAttribute incandAttr;

    std::string arg_velFrom;
    std::string arg_accFrom;
    std::string arg_rgbFrom;
    std::string arg_incandFrom;
    std::string arg_opacFrom;
    std::string arg_radFrom;
    std::string arg_extraPPAttrs;
    std::string arg_file;

    AtRGB arg_defaultColor;

    float arg_radius;
    float arg_maxParticleRadius;
    float arg_minParticleRadius;
    float arg_radiusMult;
    float arg_defaultOpac;
    float arg_stepSize;
    float arg_motionBlurMult;
    float arg_filterSmallParticles;
    float global_motionByFrame;
    float global_fps;

    int arg_renderType;
    int global_motionBlurSteps;

    bool arg_overrideRadiusPP;
    bool cacheExists;
    bool canMotionBlur;
    bool hasRadiusPP;
    bool hasRgbPP;
    bool hasOpacPP;
    bool hasIncandPP;


    PartioData() : arg_rgbFrom(""), arg_incandFrom(""), arg_opacFrom(""),
                   arg_radFrom(""), arg_extraPPAttrs(""), arg_file("")
    {
        points = 0;
        mynode = 0;

        arg_defaultColor = AI_RGB_WHITE;

        arg_radius = 0.01f;
        arg_maxParticleRadius = 1000000.0f;
        arg_minParticleRadius = 0.0f;
        arg_radiusMult = 1.0f;
        arg_defaultOpac = 1.0f;
        arg_stepSize = 0.0f;
        arg_motionBlurMult = 1.0f;
        arg_filterSmallParticles = 8.0f;
        global_motionByFrame = 0.5f;
        global_fps = 24.0f;

        arg_renderType = 0;
        global_motionBlurSteps = 1;

        arg_overrideRadiusPP = false;
        cacheExists = false;
        canMotionBlur = false;
        hasRadiusPP = false;
        hasRgbPP = false;
        hasOpacPP = false;
        hasIncandPP = false;

        positionAttr.type = PARTIO::NONE;
        accelerationAttr.type = PARTIO::NONE;
    }

    ~PartioData()
    {
        if (points)
            points->release();
    }

    bool init(AtNode* node)
    {
        mynode = node;
        // check for values on node
        getParam(arg_file, mynode, "arg_file");
        getParam(arg_overrideRadiusPP, mynode, "overrideRadiusPP");
        getParam(arg_radius, mynode, "arg_radius");
        getParam(arg_maxParticleRadius, mynode, "arg_maxParticleRadius");
        getParam(arg_minParticleRadius, mynode, "arg_minParticleRadius");
        getParam(arg_radiusMult, mynode, "arg_radiusMult");
        getParam(arg_renderType, mynode, "arg_renderType");
        getParam(arg_motionBlurMult, mynode, "arg_motionBlurMult");
        getParam(arg_velFrom, mynode, "arg_velFrom");
        getParam(arg_accFrom, mynode, "arg_accFrom");
        getParam(arg_rgbFrom, mynode, "arg_rgbFrom");
        getParam(arg_incandFrom, mynode, "arg_incandFrom");
        getParam(arg_opacFrom, mynode, "arg_opacFrom");
        getParam(arg_radFrom, mynode, "arg_radFrom");
        getParam(arg_defaultColor, mynode, "arg_defaultColor");
        getParam(arg_defaultOpac, mynode, "arg_defaultOpac");
        getParam(arg_filterSmallParticles, mynode, "arg_filterSmallParticles");
        getParam(global_motionBlurSteps, mynode, "global_motionBlurSteps");
        // no need to generate more than one steps
        // when no acceleration data is used for interpolating the data
        getParam(global_fps, mynode, "global_fps");
        getParam(global_motionByFrame, mynode, "global_motionByFrame");
        getParam(arg_stepSize, mynode, "arg_stepSize");
        getParam(arg_extraPPAttrs, mynode, "arg_extraPPAttrs");

        AiMsgInfo("[partioGenerator] loading cache  %s", arg_file.c_str());

        if (partioCacheExists(arg_file))
        {
            cacheExists = true;
            points = PARTIO::read(arg_file.c_str());
            if (points)
                return true;
            else
            {
                AiMsgInfo("[partioGenerator] skipping, no points");
                return false;
            }
        }
        else
        {
            AiMsgInfo("[partioGenerator] %s cache doesn't exists.", arg_file.c_str());
            return false;
        }
    }

    AtNode* getNode()
    {
        AtNode* currentInstance = AiNode("points"); // initialize node
        if (!cacheExists)
            return currentInstance;

        ////////////////////////////////////////////////////////////////////////////////
        /// We at least have to have  position, I mean come on throw us a bone here....
        if (!points->attributeInfo("position", positionAttr) && !points->attributeInfo("Position", positionAttr))
        {
            AiMsgInfo("[partioGenerator] Could not find position attr maybe you should do something about that");
            return currentInstance;
        }

        int pointCount = static_cast<int>(points->numParticles());
        AiMsgInfo("[partioGenerator] loaded %d points", pointCount);

        // first we filter invalid particles properly
        // then we try avoid very small ones
        // to do that we need radius and position info

        const float singleRadius = arg_radius * arg_radiusMult;

        ///////////////////////////////////////////////////////////////////////////////////
        /// RADIUS by default  if "none" is defined it will look for  radiusPP or  radius
        if (!arg_overrideRadiusPP)
        {
            if (((arg_radFrom.length() > 0) && points->attributeInfo(arg_radFrom.c_str(), radiusAttr)))
            {
                AiMsgInfo("[partioGenerator] found radius attr...%s", arg_radFrom.c_str());
                hasRadiusPP = true;
            }
            else if ((points->attributeInfo("radiusPP", radiusAttr) || points->attributeInfo("radius", radiusAttr)))
            {
                AiMsgInfo("[partioGenerator] found radius attr...");
                hasRadiusPP = true;
            }
        }

        std::vector<AtVector> pointsVector;
        std::vector<int> validPointsVector;
        std::vector<float> radiiVector;

        pointsVector.reserve(pointCount);
        validPointsVector.reserve(pointCount);
        if (hasRadiusPP)
            radiiVector.reserve(pointCount);

        const float filterSmallParticles = powf(10.0f, arg_filterSmallParticles);

        for (int i = 0; i < pointCount; ++i)
        {
            const float* partioPositions = points->data<float>(positionAttr, i);
            if (AiIsFinite(partioPositions[0]) && AiIsFinite(partioPositions[1]) && AiIsFinite(partioPositions[2]))
            {
                const AtVector position = {partioPositions[0], partioPositions[1], partioPositions[2]};
                // doing the radiuses here is a temporary workaround
                // and an experiment to see how easily we can filter
                // very small particles
                float rad = 0.0f;
                if (hasRadiusPP)
                {
                    // const float* partioRadius = points->data<float>(radiusAttr, id);
                    const float* partioRadius = points->data<float>(radiusAttr, i);
                    if (radiusAttr.count == 1)
                        rad = partioRadius[0];
                    else
                        rad = ABS(float((partioRadius[0] * 0.2126f) + (partioRadius[1] * 0.7152f) +
                                        (partioRadius[2] * .0722f)));

                    rad *= arg_radius * arg_radiusMult;
                }
                else
                    rad = singleRadius;

                // clamp the radius to maxParticleRadius just in case we have rogue particles
                rad = CLAMP(rad, arg_minParticleRadius, arg_maxParticleRadius);

                // According to Thiago Ize, we have to filter particles
                // where the ratio of the position from the origin
                // and radius is bigger than 10^7
                if ((AiV3Length(position) / rad) > filterSmallParticles)
                    continue;

                //AiArraySetFlt(radarr , i, rad);
                if (hasRadiusPP)
                    radiiVector.push_back(rad);
                pointsVector.push_back(position);
                validPointsVector.push_back(i);
            }
        }

        const int newPointCount = static_cast<int>(pointsVector.size());
        AiMsgInfo("[partioGenerator] Filtered %i of %i particles", pointCount - newPointCount, pointCount);
        pointCount = newPointCount;

        if (pointCount == 0)
            return currentInstance;

        AtArray* pointarr = 0;
        AtArray* radarr = 0;
        AtArray* rgbArr = 0;
        AtArray* opacityArr = 0;
        AtArray* incandArr = 0;
        AtArray* floatArr = 0;
        AtArray* vecArr = 0;

        ////////////////
        /// Velocity
        if ((global_motionBlurSteps > 1) && (arg_velFrom.length() > 0) && (points->attributeInfo(arg_velFrom.c_str(), velocityAttr) ||
            points->attributeInfo("velocity", velocityAttr) || points->attributeInfo("Velocity", velocityAttr)) && (velocityAttr.count > 2) &&
            (velocityAttr.type == PARTIO::FLOAT || velocityAttr.type == PARTIO::VECTOR))
        {
            AiMsgInfo("[partioGenerator] found velocity attr, motion blur is a GO!!");
            canMotionBlur = true;

            ////////////////////
            /// Acceleration
            if (points->attributeInfo(arg_accFrom.c_str(), accelerationAttr) && (accelerationAttr.count > 2) &&
                                      (accelerationAttr.type == PARTIO::FLOAT || accelerationAttr.type == PARTIO::VECTOR))
            {
                AiMsgInfo("[partioGenerator] found acceleration attr, motion blur is a GO!!");
                canMotionBlur = true;
            }
        }


        ////////////
        /// RGB
        if ((arg_rgbFrom.length() > 0) && points->attributeInfo(arg_rgbFrom.c_str(), rgbAttr))
        {
            AiNodeDeclare(currentInstance, "rgbPP", "uniform RGB");
            AiMsgInfo("[partioGenerator] found rgbPP attr...");
            hasRgbPP = true;
            rgbArr = AiArrayAllocate(pointCount, 1, AI_TYPE_RGB);
        }
        else
        {
            AiNodeDeclare(currentInstance, "rgbPP", "constant RGB");
            AiNodeSetRGB(currentInstance, "rgbPP", arg_defaultColor.r, arg_defaultColor.g, arg_defaultColor.b);
        }

        ////////////
        /// Incandescence
        if ((arg_incandFrom.length() > 0) && points->attributeInfo(arg_incandFrom.c_str(), incandAttr))
        {
            AiNodeDeclare(currentInstance, "incandescencePP", "uniform RGB");
            AiMsgInfo("[partioGenerator] found incandescencePP attr...");
            hasIncandPP = true;
            incandArr = AiArrayAllocate(pointCount, 1, AI_TYPE_RGB);
        }
        else
        {
            AiNodeDeclare(currentInstance, "incandescencePP", "constant RGB");
            AiNodeSetRGB(currentInstance, "incandescencePP", 0.0f, 0.0f, 0.0f);
        }

        //////////////
        /// OPACITY
        if ((arg_opacFrom.length() > 0) && points->attributeInfo(arg_opacFrom.c_str(), opacityAttr))
        {
            AiNodeDeclare(currentInstance, "opacityPP", "uniform Float");
            AiMsgInfo("[partioGenerator] found opacityPP attr...");
            hasOpacPP = true;
            opacityArr = AiArrayAllocate(pointCount, 1, AI_TYPE_FLOAT);
        }
        else
        {
            AiNodeDeclare(currentInstance, "opacityPP", "constant Float");
            AiNodeSetFlt(currentInstance, "opacityPP", arg_defaultOpac);
        }

        const bool can_accelerate = accelerationAttr.type != PARTIO::NONE;

        if (canMotionBlur)
        {
            if (can_accelerate)
                pointarr = AiArrayAllocate(pointCount, global_motionBlurSteps, AI_TYPE_POINT);
            else
                pointarr = AiArrayAllocate(pointCount, 2, AI_TYPE_POINT);
        }
        else
            pointarr = AiArrayConvert(pointCount, 1, AI_TYPE_POINT, &pointsVector[0]);

        if (hasRadiusPP)
            radarr = AiArrayConvert(pointCount, 1, AI_TYPE_FLOAT, &radiiVector[0]);
        else
        {
            radarr = AiArrayAllocate(1, 1, AI_TYPE_FLOAT);
            AiArraySetFlt(radarr, 0, singleRadius);
        }


        // now parse and include any extra attrs
        std::vector<PARTIO::ParticleAttribute> extraAttrs;
        std::vector<AtArray*> arnoldArrays;

        char extraAttrStr[1000];
        strncpy(extraAttrStr, arg_extraPPAttrs.c_str(), sizeof(extraAttrStr));
        char* parts[100] = {0};
        unsigned int index = 0;
        parts[index] = strtok(extraAttrStr, " ");
        while (parts[index] != 0)
        {
            PARTIO::ParticleAttribute user;
            if (points->attributeInfo(parts[index], user))
            {
                // we don't want to double export these 
                if (user.name != "position" &&
                    user.name != "velocity" &&
                    user.name != "rgbPP" &&
                    user.name != "incandescencePP" &&
                    user.name != "opacityPP" &&
                    user.name != "radiusPP")
                {
                    if (user.type == PARTIO::FLOAT && user.count == 1)
                    {
                        AiNodeDeclare(currentInstance, parts[index], "uniform Float");
                        floatArr = AiArrayAllocate(pointCount, 1, AI_TYPE_FLOAT);
                        arnoldArrays.push_back(floatArr);
                    }
                    else if (user.type == PARTIO::VECTOR || (user.type == PARTIO::FLOAT && user.count == 3))
                    {
                        AiNodeDeclare(currentInstance, parts[index], "uniform Vector");
                        vecArr = AiArrayAllocate(pointCount, 1, AI_TYPE_VECTOR);
                        arnoldArrays.push_back(vecArr);
                    }
                    extraAttrs.push_back(user);
                }
                else
                    AiMsgWarning("[partioGenerator] caught double export call for %s, skipping....", parts[index]);
            }
            else
                AiMsgWarning("[partioGenerator] export ppAttr %s skipped, it doesn't exist  in the cache",
                             parts[index]);

            ++index;
            parts[index] = strtok(0, " ");
        }

        const float motion_blur_time = (1.0f / global_fps) * global_motionByFrame * arg_motionBlurMult;
        std::vector<double> motion_step_times;
        if (can_accelerate)
        {
            motion_step_times.resize(global_motionBlurSteps, 0.0f);
            for (int st = 0; st < global_motionBlurSteps; ++st)
            {
                motion_step_times[st] = static_cast<float>(-0.5 + ((double)st / ((double)global_motionBlurSteps - 1.0))) * motion_blur_time;
            }
        }

        AiMsgDebug("[partioGenerator] about to final loop thru the particles");

        ///////////////////////////////////////////////
        /// LOOP particles
        for (int i = 0; i < pointCount; ++i)
        {
            const int id = validPointsVector[i];

            if (canMotionBlur)
            {
                if (can_accelerate)
                {
                    const AtVector point = pointsVector[i];
                    const float* partioAcc = points->data<float>(accelerationAttr, id);
                    const AtVector acceleration = {partioAcc[0], partioAcc[1], partioAcc[2]};
                    const float* partioVelo = points->data<float>(velocityAttr, id);
                    AtVector velocity = {partioVelo[0], partioVelo[1], partioVelo[2]};

                    for (int st = 0; st < global_motionBlurSteps; ++st)
                    {
                        const float current_time = motion_step_times[st];
                        AiArraySetPnt(pointarr, i + st * pointCount, point + velocity * current_time + acceleration * current_time * current_time / 2.0);
                    }
                }
                else
                {
                    const AtVector point = pointsVector[i];
                    const float* partioVelo = points->data<float>(velocityAttr, id);
                    AtVector velocity = {partioVelo[0], partioVelo[1], partioVelo[2]};
                    velocity *= motion_blur_time * 0.5f;
                    // global fps inverse, motion by frame
                    // motion blur multiplier and half to have the original
                    // particle position as center

                    AiArraySetPnt(pointarr, i, point - velocity);
                    AiArraySetPnt(pointarr, i + pointCount, point + velocity);
                }
            }

            /// RGBPP
            if (hasRgbPP)
            {
                const float* partioRGB = points->data<float>(rgbAttr, id);
                AtRGB color;
                if (rgbAttr.count > 2)
                {
                    color.r = partioRGB[0];
                    color.g = partioRGB[1];
                    color.b = partioRGB[2];
                }
                else
                {
                    color.r = partioRGB[0];
                    color.g = partioRGB[0];
                    color.b = partioRGB[0];
                }
                AiArraySetRGB(rgbArr, i, color);
            }
            /// INCANDESCENCE 
            if (hasIncandPP)
            {
                const float* partioRGB = points->data<float>(incandAttr, id);
                AtRGB incand;
                if (incandAttr.count > 2)
                {
                    incand.r = partioRGB[0];
                    incand.g = partioRGB[1];
                    incand.b = partioRGB[2];
                }
                else
                {
                    incand.r = partioRGB[0];
                    incand.g = partioRGB[0];
                    incand.b = partioRGB[0];
                }
                AiArraySetRGB(incandArr, i, incand);
            }

            /// opacityPP
            if (hasOpacPP)
            {
                const float* partioOpac = points->data<float>(opacityAttr, id);
                float opac;
                if (opacityAttr.count > 2)
                    opac = float((partioOpac[0] * 0.2126f) + (partioOpac[1] * 0.7152f) + (partioOpac[2] * .0722f));
                else
                    opac = partioOpac[0];


                AiArraySetFlt(opacityArr, i, opac);
            }
            for (size_t x = 0; x < extraAttrs.size(); ++x)
            {
                if (extraAttrs[x].type == PARTIO::FLOAT && extraAttrs[x].count == 1)
                {

                    const float* partioFLOAT = points->data<float>(extraAttrs[x], id);
                    float floatVal = partioFLOAT[0];
                    AiArraySetFlt(arnoldArrays[x], i, floatVal);
                }
                else if (extraAttrs[x].type == PARTIO::VECTOR || (extraAttrs[x].type == PARTIO::FLOAT && extraAttrs[x].count == 3))
                {
                    const float* partioVEC = points->data<float>(extraAttrs[x], id);
                    AtVector vecVal;
                    vecVal.x = partioVEC[0];
                    vecVal.y = partioVEC[1];
                    vecVal.z = partioVEC[2];
                    AiArraySetVec(arnoldArrays[x], i, vecVal);
                }
            }
        } // for loop per particle

        AiMsgDebug("[partioGenerator] Done looping thru particles");

        if (hasRgbPP)
        {
            AiNodeSetArray(currentInstance, "rgbPP", rgbArr);
            AiMsgDebug("[partioGenerator] rgbPP array set");
        }

        if (hasIncandPP)
        {
            AiNodeSetArray(currentInstance, "incandescencePP", incandArr);
            AiMsgDebug("[partioGenerator] incandescencePP array set");
        }

        if (hasOpacPP)
        {
            AiNodeSetArray(currentInstance, "opacityPP", opacityArr);
            AiMsgDebug("[partioGenerator] OpacityPP array set");
        }

        AiNodeSetArray(currentInstance, "radius", radarr);
        AiMsgDebug("[partioGenerator] radiusPP array set");

        for (size_t x = 0; x < arnoldArrays.size(); ++x)
        {
            AiNodeSetArray(currentInstance, extraAttrs[x].name.c_str(), arnoldArrays[x]);
            AiMsgDebug("[partioGenerator] %s array set", extraAttrs[x].name.c_str() );
        }

        /// these  will always be here
        AiNodeSetArray(currentInstance, "points", pointarr);
        AiMsgDebug("[partioGenerator] Points array set with %i points", pointarr->nelements );
        //AiNodeSetArray ( currentInstance, "position",pointarr );  // we want to enable this when they fix some stuff on particles

        AiNodeSetInt(currentInstance, "mode", arg_renderType);
        AiNodeSetBool(currentInstance, "opaque", false);

        if (arg_stepSize > 0.0f)
            AiNodeSetFlt(currentInstance, "step_size", arg_stepSize);

        AiMsgDebug("[partioGenerator] Done with partioGeneration! ");
        return currentInstance;
    }

    bool partioCacheExists(const std::string& fileName)
    {
        struct stat fileInfo;

        const int intStat = stat(fileName.c_str(), &fileInfo);
        if (intStat == 0)
            return true;
        else
            return false;
    }
};

// read parameter values from node and load partio cache if it exists
static int MyInit(AtNode* mynode, void** user_ptr)
{
    PartioData* data = new PartioData();
    *user_ptr = data;
    return data->init(mynode);
}

// close  partio cache
static int MyCleanup(void* user_ptr)
{
    PartioData* data = reinterpret_cast<PartioData*>(user_ptr);
    delete data;
    return true;
}

static int MyNumNodes(void* user_ptr)
{
    return 1;
}


// call function to copy values from cache read into AI-Arrays
static AtNode* MyGetNode(void* user_ptr, int i)
{
    return reinterpret_cast<PartioData*>(user_ptr)->getNode();
}

// vtable passed in by proc_loader macro define
proc_loader
{
    vtable->Init = MyInit;
    vtable->Cleanup = MyCleanup;
    vtable->NumNodes = MyNumNodes;
    vtable->GetNode = MyGetNode;
    strcpy(vtable->version, AI_VERSION);
    return true;
}

