#include <ai.h>

#include <cstring>

#include <sstream>
#include <utility>
#include <sys/stat.h>
#include <boost/regex.hpp>
#include <Partio.h>

AI_PROCEDURAL_NODE_EXPORT_METHODS(partioGeneratorMtd)

namespace {
    template<typename T>
    void getParam(T&, AtNode*, const char*)
    {
        // do nothing
    }

    template<>
    void getParam<int>(int& param, AtNode* node, const char* paramName)
    {
        param = AiNodeGetInt(node, paramName);
    }

    template<>
    void getParam<float>(float& param, AtNode* node, const char* paramName)
    {
        param = AiNodeGetFlt(node, paramName);
    }

    template<>
    void getParam<bool>(bool& param, AtNode* node, const char* paramName)
    {
        param = AiNodeGetBool(node, paramName);
    }

    template<>
    void getParam<std::string>(std::string& param, AtNode* node, const char* paramName)
    {
        param = AiNodeGetStr(node, paramName);
    }

    template<>
    void getParam<AtRGB>(AtRGB& param, AtNode* node, const char* paramName)
    {
        param = AiNodeGetRGB(node, paramName);
    }

    struct PartioData {
        PARTIO::ParticlesData* points;

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

        PartioData()
        {
            points = 0;

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
        }

        ~PartioData()
        {
            if (points) {
                points->release();
            }
        }

        bool init(AtNode* node)
        {
            // check for values on node
            getParam(arg_file, node, "arg_file");
            getParam(arg_overrideRadiusPP, node, "overrideRadiusPP");
            getParam(arg_radius, node, "arg_radius");
            getParam(arg_maxParticleRadius, node, "arg_maxParticleRadius");
            getParam(arg_minParticleRadius, node, "arg_minParticleRadius");
            getParam(arg_radiusMult, node, "arg_radiusMult");
            getParam(arg_renderType, node, "arg_renderType");
            getParam(arg_motionBlurMult, node, "arg_motionBlurMult");
            getParam(arg_velFrom, node, "arg_velFrom");
            getParam(arg_accFrom, node, "arg_accFrom");
            getParam(arg_rgbFrom, node, "arg_rgbFrom");
            getParam(arg_incandFrom, node, "arg_incandFrom");
            getParam(arg_opacFrom, node, "arg_opacFrom");
            getParam(arg_radFrom, node, "arg_radFrom");
            getParam(arg_defaultColor, node, "arg_defaultColor");
            getParam(arg_defaultOpac, node, "arg_defaultOpac");
            getParam(arg_filterSmallParticles, node, "arg_filterSmallParticles");
            getParam(global_motionBlurSteps, node, "global_motionBlurSteps");
            // no need to generate more than one steps
            // when no acceleration data is used for interpolating the data
            getParam(global_fps, node, "global_fps");
            getParam(global_motionByFrame, node, "global_motionByFrame");
            getParam(arg_stepSize, node, "arg_stepSize");
            getParam(arg_extraPPAttrs, node, "arg_extraPPAttrs");

            AiMsgInfo("[partioGenerator] loading cache  %s", arg_file.c_str());

            if (partioCacheExists(arg_file)) {
                cacheExists = true;
                points = PARTIO::read(arg_file.c_str());
                if (points) {
                    return true;
                } else {
                    AiMsgInfo("[partioGenerator] skipping, no points");
                    return false;
                }
            } else {
                AiMsgInfo("[partioGenerator] %s cache doesn't exists.", arg_file.c_str());
                return false;
            }
        }

        static bool isBuiltinAttribute(const std::string& channel)
        {
            return channel == "position" ||
                   channel == "velocity" ||
                   channel == "rgbPP" ||
                   channel == "incandescencePP" ||
                   channel == "opacityPP" ||
                   channel == "radiusPP";
        }

        void exportUserParam(AtNode* currentInstance, const PARTIO::ParticleAttribute& user,
                             const std::vector<uint32_t>& validPointsVector, uint32_t pointCount)
        {
            if (user.type == PARTIO::FLOAT && user.count == 1) {
                AiNodeDeclare(currentInstance, user.name.c_str(), "uniform Float");
                auto* arnoldArray = AiArrayAllocate(pointCount, 1, AI_TYPE_FLOAT);
                for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                    const auto id = validPointsVector[i];
                    const auto* partioFLOAT = points->data<float>(user, id);
                    AiArraySetFlt(arnoldArray, i, *partioFLOAT);
                }
                AiNodeSetArray(currentInstance, user.name.c_str(), arnoldArray);
                AiMsgDebug("[partioGenerator] %s array set", user.name.c_str());
            } else if (user.type == PARTIO::VECTOR || (user.type == PARTIO::FLOAT && user.count == 3)) {
                AiNodeDeclare(currentInstance, user.name.c_str(), "uniform Vector");
                auto* arnoldArray = AiArrayAllocate(pointCount, 1, AI_TYPE_VECTOR);
                for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                    const auto id = validPointsVector[i];
                    const auto* partioVEC = points->data<float>(user, id);
                    const AtVector vecVal(
                        partioVEC[0],
                        partioVEC[1],
                        partioVEC[2]
                    );
                    AiArraySetVec(arnoldArray, i, vecVal);
                }

                AiNodeSetArray(currentInstance, user.name.c_str(), arnoldArray);
                AiMsgDebug("[partioGenerator] %s array set", user.name.c_str());
            }
        }

        AtNode* getNode()
        {
            auto* currentInstance = AiNode("points"); // initialize node
            if (!cacheExists || points == 0) {
                return currentInstance;
            }

            PARTIO::ParticleAttribute positionAttr;

            ////////////////////////////////////////////////////////////////////////////////
            /// We at least have to have  position, I mean come on throw us a bone here....
            if (!points->attributeInfo("position", positionAttr) && !points->attributeInfo("Position", positionAttr)) {
                AiMsgInfo("[partioGenerator] Could not find position attr maybe you should do something about that");
                return currentInstance;
            }

            PARTIO::ParticleAttribute velocityAttr;
            PARTIO::ParticleAttribute accelerationAttr;
            PARTIO::ParticleAttribute radiusAttr;

            auto pointCountI = points->numParticles();
            if (pointCountI <= 0) {
                return nullptr;
            }
            auto pointCount = static_cast<uint32_t>(pointCountI);
            AiMsgInfo("[partioGenerator] loaded %u points", pointCountI);

            auto canMotionBlur = false;
            auto hasRadiusPP = false;

            // first we filter invalid particles properly
            // then we try avoid very small ones
            // to do that we need radius and position info
            // this is a legacy bug in 4.2.x series, but we keep it just in case something goes wrong again
            const auto singleRadius = arg_radius * arg_radiusMult;

            ///////////////////////////////////////////////////////////////////////////////////
            /// RADIUS by default  if "none" is defined it will look for  radiusPP or  radius
            if (!arg_overrideRadiusPP) {
                if (((arg_radFrom.length() > 0) && points->attributeInfo(arg_radFrom.c_str(), radiusAttr))) {
                    AiMsgInfo("[partioGenerator] found radius attr...%s", arg_radFrom.c_str());
                    hasRadiusPP = true;
                } else if ((points->attributeInfo("radiusPP", radiusAttr) ||
                            points->attributeInfo("radius", radiusAttr))) {
                    AiMsgInfo("[partioGenerator] found radius attr...");
                    hasRadiusPP = true;
                }
            }

            std::vector<AtVector> pointsVector;
            std::vector<uint32_t> validPointsVector;
            std::vector<float> radiiVector;

            pointsVector.reserve(pointCount);
            validPointsVector.reserve(pointCount);
            if (hasRadiusPP) {
                radiiVector.reserve(pointCount);
            }

            const auto filterSmallParticles = powf(10.0f, arg_filterSmallParticles);

            for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                const auto* partioPositions = points->data<float>(positionAttr, i);
                if (AiIsFinite(partioPositions[0]) && AiIsFinite(partioPositions[1]) &&
                    AiIsFinite(partioPositions[2])) {
                    const AtVector position(partioPositions[0], partioPositions[1], partioPositions[2]);
                    // doing the radiuses here is a temporary workaround
                    // and an experiment to see how easily we can filter
                    // very small particles
                    auto rad = 0.0f;
                    if (hasRadiusPP) {
                        // const float* partioRadius = points->data<float>(radiusAttr, id);
                        const float* partioRadius = points->data<float>(radiusAttr, i);
                        if (radiusAttr.count == 1) {
                            rad = partioRadius[0];
                        } else {
                            rad = std::abs(
                                (partioRadius[0] * 0.2126f) + (partioRadius[1] * 0.7152f) + (partioRadius[2] * .0722f));
                        }

                        rad *= arg_radius * arg_radiusMult;
                    } else {
                        rad = singleRadius;
                    }

                    // clamp the radius to maxParticleRadius just in case we have rogue particles
                    rad = AiClamp(rad, arg_minParticleRadius, arg_maxParticleRadius);

                    // According to Thiago Ize, we have to filter particles
                    // where the ratio of the position from the origin
                    // and radius is bigger than 10^7
                    if ((AiV3Length(position) / rad) > filterSmallParticles) {
                        continue;
                    }

                    //AiArraySetFlt(radarr , i, rad);
                    if (hasRadiusPP) {
                        radiiVector.push_back(rad);
                    }
                    pointsVector.push_back(position);
                    validPointsVector.push_back(i);
                }
            }

            const auto newPointCount = static_cast<uint32_t>(pointsVector.size());
            AiMsgInfo("[partioGenerator] Filtered %u of %u particles", pointCount - newPointCount, pointCount);
            pointCount = newPointCount;

            if (pointCount == 0) {
                return currentInstance;
            }

            AtArray* pointarr = nullptr;
            AtArray* radarr = nullptr;

            auto can_accelerate = false;

            ////////////////
            /// Velocity
            if ((global_motionBlurSteps > 1) &&
                ((arg_velFrom.length() > 0 && points->attributeInfo(arg_velFrom.c_str(), velocityAttr)) ||
                 points->attributeInfo("velocity", velocityAttr) ||
                 points->attributeInfo("Velocity", velocityAttr) ||
                 points->attributeInfo("V", velocityAttr)) &&
                (velocityAttr.count > 2) &&
                (velocityAttr.type == PARTIO::FLOAT || velocityAttr.type == PARTIO::VECTOR)) {
                AiMsgInfo("[partioGenerator] found velocity attr, motion blur is a GO!!");
                canMotionBlur = true;

                ////////////////////
                /// Acceleration
                if (points->attributeInfo(arg_accFrom.c_str(), accelerationAttr) && (accelerationAttr.count > 2) &&
                    (accelerationAttr.type == PARTIO::FLOAT || accelerationAttr.type == PARTIO::VECTOR)) {
                    AiMsgInfo("[partioGenerator] found acceleration attr, motion blur is a GO!!");
                    can_accelerate = true;
                }
            }

            if (canMotionBlur) {
                if (can_accelerate) {
                    pointarr = AiArrayAllocate(pointCount,
                                               static_cast<uint8_t>(global_motionBlurSteps), AI_TYPE_VECTOR);
                } else {
                    pointarr = AiArrayAllocate(pointCount, 2, AI_TYPE_VECTOR);
                }
            } else {
                pointarr = AiArrayConvert(pointCount, 1, AI_TYPE_VECTOR, &pointsVector[0]);
            }

            if (hasRadiusPP) {
                radarr = AiArrayConvert(pointCount, 1, AI_TYPE_FLOAT, &radiiVector[0]);
            } else {
                radarr = AiArrayAllocate(1, 1, AI_TYPE_FLOAT);
                AiArraySetFlt(radarr, 0, singleRadius);
            }

            const auto motion_blur_time = (1.0f / global_fps) * global_motionByFrame * arg_motionBlurMult;
            std::vector<double> motion_step_times;
            if (can_accelerate) {
                motion_step_times.resize(static_cast<size_t>(global_motionBlurSteps), 0.0f);
                for (int st = 0; st < global_motionBlurSteps; ++st) {
                    motion_step_times[st] =
                        -0.5 + ((double)st / ((double)global_motionBlurSteps - 1.0)) * motion_blur_time;
                }
            }

            if (canMotionBlur) {
                if (can_accelerate) {
                    for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                        const auto id = validPointsVector[i];

                        const auto point = pointsVector[i];
                        const auto* partioAcc = points->data<float>(accelerationAttr, id);
                        const AtVector acceleration = {partioAcc[0], partioAcc[1], partioAcc[2]};
                        const auto* partioVelo = points->data<float>(velocityAttr, id);
                        const AtVector velocity = {partioVelo[0], partioVelo[1], partioVelo[2]};

                        for (decltype(global_motionBlurSteps) st = 0; st < global_motionBlurSteps; ++st) {
                            const float current_time = static_cast<float>(motion_step_times[st]);
                            AiArraySetVec(pointarr, i + static_cast<uint32_t>(st) * pointCount,
                                          point + velocity * current_time +
                                          acceleration * current_time * current_time / 2.0);
                        }
                    }
                } else {
                    for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                        const auto id = validPointsVector[i];
                        const auto point = pointsVector[i];
                        const float* partioVelo = points->data<float>(velocityAttr, id);
                        AtVector velocity = {partioVelo[0], partioVelo[1], partioVelo[2]};
                        velocity *= motion_blur_time * 0.5f;
                        // global fps inverse, motion by frame
                        // motion blur multiplier and half to have the original
                        // particle position as center

                        AiArraySetVec(pointarr, i, point - velocity);
                        AiArraySetVec(pointarr, i + pointCount, point + velocity);
                    }
                }
            }

            PARTIO::ParticleAttribute rgbAttr;
            /// RGBPP
            if ((arg_rgbFrom.length() > 0) && points->attributeInfo(arg_rgbFrom.c_str(), rgbAttr)) {
                AiNodeDeclare(currentInstance, "rgbPP", "uniform RGB");
                AiMsgInfo("[partioGenerator] found rgbPP attr...");

                auto* rgbArr = AiArrayAllocate(pointCount, 1, AI_TYPE_RGB);
                for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                    const auto id = validPointsVector[i];
                    const auto* partioRGB = points->data<float>(rgbAttr, id);
                    AtRGB color;
                    if (rgbAttr.count > 2) {
                        color.r = partioRGB[0];
                        color.g = partioRGB[1];
                        color.b = partioRGB[2];
                    } else {
                        color.r = partioRGB[0];
                        color.g = partioRGB[0];
                        color.b = partioRGB[0];
                    }
                    AiArraySetRGB(rgbArr, i, color);
                }

                AiNodeSetArray(currentInstance, "rgbPP", rgbArr);
                AiMsgDebug("[partioGenerator] rgbPP array set");
            } else {
                AiNodeDeclare(currentInstance, "rgbPP", "constant RGB");
                AiNodeSetRGB(currentInstance, "rgbPP", arg_defaultColor.r, arg_defaultColor.g, arg_defaultColor.b);
            }

            /// INCANDESCENCE
            PARTIO::ParticleAttribute incandAttr;
            if ((arg_incandFrom.length() > 0) && points->attributeInfo(arg_incandFrom.c_str(), incandAttr)) {
                AiNodeDeclare(currentInstance, "incandescencePP", "uniform RGB");
                AiMsgInfo("[partioGenerator] found incandescencePP attr...");
                auto* incandArr = AiArrayAllocate(pointCount, 1, AI_TYPE_RGB);
                for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                    const auto id = validPointsVector[i];
                    const auto* partioRGB = points->data<float>(incandAttr, id);
                    AtRGB incand;
                    if (incandAttr.count > 2) {
                        incand.r = partioRGB[0];
                        incand.g = partioRGB[1];
                        incand.b = partioRGB[2];
                    } else {
                        incand.r = partioRGB[0];
                        incand.g = partioRGB[0];
                        incand.b = partioRGB[0];
                    }
                    AiArraySetRGB(incandArr, i, incand);
                }

                AiNodeSetArray(currentInstance, "incandescencePP", incandArr);
                AiMsgDebug("[partioGenerator] incandescencePP array set");
            } else {
                AiNodeDeclare(currentInstance, "incandescencePP", "constant RGB");
                AiNodeSetRGB(currentInstance, "incandescencePP", 0.0f, 0.0f, 0.0f);
            }

            /// opacityPP
            PARTIO::ParticleAttribute opacityAttr;
            if ((arg_opacFrom.length() > 0) && points->attributeInfo(arg_opacFrom.c_str(), opacityAttr)) {
                AiNodeDeclare(currentInstance, "opacityPP", "uniform Float");
                AiMsgInfo("[partioGenerator] found opacityPP attr...");

                AtArray* opacityArr = AiArrayAllocate(pointCount, 1, AI_TYPE_FLOAT);

                for (auto i = decltype(pointCount){0}; i < pointCount; ++i) {
                    const auto id = validPointsVector[i];
                    const auto* partioOpac = points->data<float>(opacityAttr, id);
                    const auto opac = opacityAttr.count > 2 ?
                                      (partioOpac[0] * 0.2126f) + (partioOpac[1] * 0.7152f) + (partioOpac[2] * .0722f) :
                                      partioOpac[0];
                    AiArraySetFlt(opacityArr, i, opac);
                }

                AiNodeSetArray(currentInstance, "opacityPP", opacityArr);
                AiMsgDebug("[partioGenerator] OpacityPP array set");
            } else {
                AiNodeDeclare(currentInstance, "opacityPP", "constant Float");
                AiNodeSetFlt(currentInstance, "opacityPP", arg_defaultOpac);
            }

            std::istringstream os(arg_extraPPAttrs);
            std::string part;
            while (os >> part) {
                auto is_glob = false;
                for (size_t it = part.find('*'); it != std::string::npos; it = part.find('*', it + 2)) {
                    part.replace(it, it + 1, ".*");
                    is_glob = true;
                }
                PARTIO::ParticleAttribute user;
                if (is_glob) {
                    boost::regex reg(part);
                    const auto numAttributes = points->numAttributes();
                    for (auto i = decltype(numAttributes){0}; i < numAttributes; ++i) {
                        if (points->attributeInfo(i, user)) {
                            if (isBuiltinAttribute(user.name)) {
                                continue;
                            }
                            if (!boost::regex_match(user.name, reg)) {
                                continue;
                            }
                            if (AiNodeLookUpUserParameter(currentInstance, user.name.c_str()) != nullptr) {
                                continue;
                            }
                            exportUserParam(currentInstance, user, validPointsVector,
                                            pointCount);
                        }
                    }
                } else if (points->attributeInfo(part.c_str(), user)) {
                    // we don't want to double export these
                    if (!isBuiltinAttribute(part)) {
                        if (AiNodeLookUpUserParameter(currentInstance, part.c_str()) != 0) {
                            continue;
                        }
                        exportUserParam(currentInstance, user, validPointsVector, pointCount);
                    } else {
                        AiMsgWarning("[partioGenerator] caught double export call for %s, skipping....", part.c_str());
                    }
                } else {
                    AiMsgWarning("[partioGenerator] export ppAttr %s skipped, it doesn't exist  in the cache",
                                 part.c_str());
                }
            }

            AiMsgDebug("[partioGenerator] Done looping thru particles");

            AiNodeSetArray(currentInstance, "radius", radarr);
            AiMsgDebug("[partioGenerator] radiusPP array set");

            /// these  will always be here
            AiNodeSetArray(currentInstance, "points", pointarr);
            AiMsgDebug("[partioGenerator] Points array set with %i points", AiArrayGetNumElements(pointarr));

            AiNodeSetInt(currentInstance, "mode", arg_renderType);
            AiNodeSetBool(currentInstance, "opaque", false);

            if (arg_stepSize > 0.0f) {
                AiNodeSetFlt(currentInstance, "step_size", arg_stepSize);
            }

            AiMsgDebug("[partioGenerator] Done with partioGeneration! ");
            return currentInstance;
        }

        bool partioCacheExists(const std::string& fileName)
        {
            struct stat fileInfo;
            return stat(fileName.c_str(), &fileInfo) == 0;
        }
    };
}

node_parameters
{
    AiParameterStr("arg_file", "");
    AiParameterBool("overrideRadiusPP", false);
    AiParameterFlt("arg_radius", 0.01f);
    AiParameterFlt("arg_maxParticleRadius", 1000000.0f);
    AiParameterFlt("arg_minParticleRadius", 0.0f);
    AiParameterFlt("arg_radiusMult", 1.0f);
    AiParameterInt("arg_renderType", 0);
    AiParameterFlt("arg_motionBlurMult", 1.0f);
    AiParameterStr("arg_velFrom", "");
    AiParameterStr("arg_accFrom", "");
    AiParameterStr("arg_rgbFrom", "");
    AiParameterStr("arg_incandFrom", "");
    AiParameterStr("arg_opacFrom", "");
    AiParameterStr("arg_radFrom", "");
    AiParameterStr("arg_defaultColor", "");
    AiParameterStr("arg_defaultOpac", "");
    AiParameterFlt("arg_filterSmallParticles", 8.0f);
    AiParameterInt("global_motionBlurSteps", 1);
    AiParameterFlt("global_fps", 24.0f);
    AiParameterFlt("global_motionByFrame", 0.5f);
    AiParameterFlt("arg_stepSize", 0.0f);
    AiParameterStr("arg_extraPPAttrs", "");

}

procedural_init
{
    auto* data = new PartioData();
    *user_ptr = data;
    return data->init(node);
}

procedural_num_nodes
{
    return 1;
}

procedural_get_node
{
    auto* data = reinterpret_cast<PartioData*>(user_ptr);
    return data->getNode();
}

procedural_cleanup
{
    delete reinterpret_cast<PartioData*>(user_ptr);
    return 1;
}

node_loader
{
    if (i > 0) {
        return false;
    } else {
        node->methods = partioGeneratorMtd;
        node->name = "partioGenerator";
        node->node_type = AI_NODE_SHAPE_PROCEDURAL;
        sprintf(node->version, AI_VERSION);
        return true;
    }
}
