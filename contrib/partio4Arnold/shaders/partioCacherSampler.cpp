#include <ai.h>

#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>
#include <set>
#include <vector>
#include <utility>
#include <map>
#include <algorithm>
#include <ai_types.h>
#include <limits>

/// NOTES:
/*
 * THIS SHADER originally was written to save out a partio cache based on  surface color and any  AOVs  that were being 
 * defined.  That would be the "write mode"  which is currently not what I'm doing or using  it for,
 * a lot of the AOV stuff is currently commented out, and it has gotten a bit sloppy while I'm using it to dev some FX, sorry.
 * 
 * Mainly right now  I'm writing out deformation data into a millions of points  bhclassic cache from  houdini
 * that contains a color channel which is  the vector point deform position 
 * This is using the "read mode"  functionality  per sample to pull in the  partio cache,  sort it into a kdtree, and do lookups 
 * for the  displacement value for each sample point. 
 * 
 * The problems I'm having, are based in the multi-sampling,  turning up the search points higher than 1, makes each thread seem to return 
 * different  values than neighboring threads and I get bucket sized "blocks" of different  results, which randomly  change each  time 
 * I render the  same frame.
 * 
 * I also don't know exactly what/how I should be implementing  in the "local_data" 
 * any help with that and explanation would be helpful as well.
 * 
 * CAVEATS:
 *            The sampling  lerp stuff may need a major overhaul but I don't know if thats the root cause of the threading issues.
 * 
 * Please add notes where you see fit, I'd like to learn what I'm doing wrong while we're fixing this.
 * 
 * Is there a way to make one shader have multiple connection  outputs?   a float and  an RGB for example? so one could be connected
 * to the  normalDisplacement  and the rgb could go to the vector displacement,  or do I have to separate it out into 2 separate shaders?
 * Pal: No, there is no stable ways to have multiple outputs at this moment.
 *
 * Pal's Notes.
 * * I removed the AOV code for now, since the uses were disabled anyway, so we can have a clearer working shader.
 *   Feel free to go ahead and add back the code, following the approaches found in the current code.
 * * I refactored things, so now it's using the node's local data, and updates are actually happening in updates.
 *   That means we can use multiple of the same shader now.
 * * I don't think uint is part of the c++ standard. It's probably coming from a linux header, so using
 *   unsigned int instead.
 */


#define ID_ARNOLD_PARTIO_CACHER_SAMPLER       0x00116ED2
AI_SHADER_NODE_EXPORT_METHODS(partioCacherSamplerMtd);

namespace {
    enum Mode {
        MODE_READ = 0,
        MODE_WRITE,
        MODE_PASSTHROUGH,
        MODE_LAST = MODE_PASSTHROUGH
    };

    enum Blend_Mode {
        BLEND_NONE = 0,
        BLEND_ERROR,
        BLEND_ALL
    };

    struct sort_pred {
        bool operator()(const std::pair<int, float>& left, const std::pair<int, float>& right)
        {
            return left.second < right.second;
        }
    };

    const char* gs_ModeNames[] =
        {
            "read",
            "write",
            "passthrough",
            0
        };

    const char* gs_BlendMode[] =
        {
            "Blend None",
            "Blend Error",
            "Blend All",
            0
        };

    enum partioPointCacherParams {
        p_mode,
        p_file,
        p_input,
        p_blend_mode,
        p_search_distance,
        p_search_points,
        p_scale_searchDist_by_aa,
        p_negateX,
        p_negateY,
        p_negateZ,
        p_error_color,
        p_error_disp,
        p_diag,
        p_aov_diag
    };

    struct ThreadData {
        std::vector<AtPoint> P;
        std::vector<AtRGB> RGB;
        unsigned int total_primary_samples;
        unsigned int total_secondary_samples;
        unsigned int missed_primary_samples;
        unsigned int missed_secondary_samples;

        ThreadData() : total_primary_samples(0),
                       total_secondary_samples(0),
                       missed_primary_samples(0),
                       missed_secondary_samples(0)
        {
        }
    };

    /// A good rule of thumb to order the variables by their primitive size
    /// that helps to minimize the data structure size. That way padding will be minimal.
    /// A different strategy would be to organize data layout by access frequency, and make sure
    /// that the most frequently accessed datas are on the same cache lines.
    struct ShaderData {
        ThreadData threadData[AI_MAX_THREADS];
        std::string file;
        AtNode* node;
        PARTIO::ParticlesDataMutable* readPoints;
        PARTIO::ParticleAttribute rgbAttr;
        Mode mode;
        float searchDist;
        int blendMode;
        int searchPoints;
        bool diag;
        bool negX;
        bool negY;
        bool negZ;

        ShaderData() : node(0), readPoints(0),
                       mode(MODE_PASSTHROUGH), searchDist(0.0f),
                       blendMode(BLEND_NONE), searchPoints(0),
                       diag(false), negX(false), negY(false), negZ(false)
        {
        }

        ~ShaderData()
        {
            AiMsgInfo("[luma.partioCacherSampler] Deleting shader data for : %s", AiNodeGetName(node));

            if (mode == MODE_WRITE) // write
            {
                unsigned int threadCounter = 0;
                /// To avoid constant memory reallocations we first count the number of particles to be merged together.
                /// And we try to avoid creating one huge memory segment.
                size_t num_particles = 0;

                for (unsigned int t = 0; t < AI_MAX_THREADS; ++t)
                {
                    if (threadData[t].P.size())
                    {
                        threadCounter++;
                        num_particles += threadData[t].P.size();
                    }
                }

                AiMsgInfo("[luma.partioCacherSampler] gathered %d points from %i threads.", static_cast<int>(num_particles), threadCounter);

                PARTIO::ParticlesDataMutable* out_points = PARTIO::createInterleave();

                const static size_t max_particle_count = static_cast<size_t>(std::numeric_limits<int>::max());
                /// Again Partio is kinda fucked here. It takes an integer as an input for the addParticles
                /// so we have to clamp against the maximum value of int here.
                /// This is one of the things I want to fix in Partio...
                if (num_particles > max_particle_count)
                {
                    num_particles = max_particle_count;
                    AiMsgWarning("[luma.partioCacherSampler] There are more than 2^31 particles!");
                }

                out_points->addParticles(static_cast<int>(num_particles));

                PARTIO::ParticleAttribute position_attr = out_points->addAttribute("position", PARTIO::VECTOR, 3);
                PARTIO::ParticleAttribute id_attr = out_points->addAttribute("id", PARTIO::INT, 1);
                PARTIO::ParticleAttribute rgb_attr = out_points->addAttribute("rgbPP", PARTIO::VECTOR, 3);

                PARTIO::ParticlesDataMutable::iterator it = out_points->begin();
                PARTIO::ParticleAccessor position_access(position_attr);
                PARTIO::ParticleAccessor id_access(id_attr);
                PARTIO::ParticleAccessor rgb_access(rgb_attr);
                it.addAccessor(position_access);
                it.addAccessor(id_access);
                it.addAccessor(rgb_access);

                int idCounter = 0;
                for (unsigned int t = 0; t < AI_MAX_THREADS; ++t)
                {
                    const ThreadData& tdata = threadData[t];
                    const size_t num_points = tdata.P.size();

                    for (size_t ii = 0; ii < num_points && it != out_points->end(); ++it, ++ii, ++idCounter)
                    {
                        PARTIO::Data<float, 3>& P = position_access.data<PARTIO::Data<float, 3> >(it);
                        PARTIO::Data<int, 1>& id = id_access.data<PARTIO::Data<int, 1> >(it);
                        PARTIO::Data<float, 3>& color = rgb_access.data<PARTIO::Data<float, 3> >(it);

                        P[0] = tdata.P[ii].x;
                        P[1] = tdata.P[ii].y;
                        P[2] = tdata.P[ii].z;

                        id[0] = idCounter;

                        color[0] = tdata.RGB[ii].r;
                        color[1] = tdata.RGB[ii].g;
                        color[2] = tdata.RGB[ii].b;
                    }
                }

                AiMsgDebug("[luma.partioCacherSampler] writing %d points to \"%s\"", static_cast<int>(num_particles), file.c_str());

                // WRITE OUT HERE
                write(file.c_str(), *out_points);
                out_points->release();
                AiMsgDebug("[luma.partioCacherSampler] released  memory");
            }
            else if (mode == MODE_READ) /// read
            {
                uint missed_primary = 0;
                uint missed_secondary = 0;
                uint total_primary = 0;
                uint total_secondary = 0;
                for (int i = 0; i < AI_MAX_THREADS; ++i)
                {
                    missed_primary += threadData[i].missed_primary_samples;
                    missed_secondary += threadData[i].missed_secondary_samples;
                    total_primary += threadData[i].total_primary_samples;
                    total_secondary += threadData[i].total_secondary_samples;
                }

                AiMsgInfo("[luma.partioCacherSampler] primary samples: %d total, %d missed (%.02f%%)", total_primary,
                          missed_primary,
                          (total_primary == 0) ? 0.0f : (missed_primary / (float)total_primary * 100.0f));

                AiMsgInfo("[luma.partioCacherSampler] secondary samples: %d total, %d missed (%.02f%%)", total_secondary,
                          missed_secondary,
                          (total_secondary == 0) ? 0.0f : (missed_secondary / (float)total_secondary * 100.0f));

                if (readPoints)
                    readPoints->release();
                AiMsgDebug("[luma.partioCacherSampler] Released memory");
            }
        }

        void update(AtNode* _node)
        {
            // this grabs the  shader's supported AOVs
            AiMsgDebug("[luma.partioCacherSampler] Updating node data.");

            node = _node;
            mode = static_cast<Mode>(CLAMP(AiNodeGetInt(node, "mode"), 0, static_cast<int>(MODE_LAST))); /// Being a bit over-cautious

            negX = AiNodeGetBool(node, "negate_X");
            negY = AiNodeGetBool(node, "negate_Y");
            negZ = AiNodeGetBool(node, "negate_Z");

            AtNode* options = AiUniverseGetOptions();
            const std::string current_file = AiNodeGetStr(node, "file");
            /// Checking for swatch renders.
            Mode mode = static_cast<Mode>(AiNodeGetInt(node, "mode"));
            if (current_file == "" || (AiNodeGetInt(options, "xres") == 64 && AiNodeGetInt(options, "yres") == 64))
            {
                AiMsgDebug("[luma.partioCacherSampler] Shader is not active.");
                return;
            }

            if (mode == MODE_PASSTHROUGH)
            {
                AiMsgDebug("[luma.partioCacherSampler] Passthrough mode.");
                return;
            }

            if (mode == MODE_WRITE)
            {
                file = current_file;
                AiMsgDebug("[luma.partioCacherSampler] Write mode (\"%s\")", file.c_str());
            }
            else if (mode == MODE_READ)
            {
                AiMsgDebug("[luma.partioCacherSampler] Read mode (\"%s\")", file.c_str());

                diag = AiNodeGetBool(node, "show_diagnostic");
                uint AASamples = static_cast<unsigned int>(std::max(AiNodeGetInt(AiUniverseGetOptions(), "AA_samples"), 0));

                blendMode = AiNodeGetInt(node, "blend_mode");
                searchDist = AiNodeGetFlt(node, "max_search_distance");

                /// I'm not exactly sure why the sqrt of the AA_samples was used here originally,
                /// I think it should be square instead. Feel free to correct it.
                if (AiNodeGetBool(node, "scale_searchDist_by_AA_samples"))
                    searchDist *= static_cast<float>(AASamples * AASamples);

                searchPoints = AiNodeGetInt(node, "search_points");
                if (searchPoints <= 0)
                    searchPoints = 1;

                /// Don't reload file, this is important for IPR
                if (current_file != file)
                {
                    file = current_file;
                    if (readPoints)
                        readPoints->release();

                    readPoints = PARTIO::read(file.c_str());
                }

                if (readPoints)
                {
                    /// We can do an early exit here, so no need to check this at every iteration.
                    if (!readPoints->attributeInfo("Cd", rgbAttr))
                    {
                        AiMsgError("[luma.partioCacherSampler] Input cache does not have a Cd attribute!");
                        readPoints->release();
                        readPoints = 0;
                        mode = MODE_PASSTHROUGH;
                    }
                    else
                    {
                        AiMsgDebug("[luma.partioCacherSampler] Sorting points");
                        readPoints->sort();
                        AiMsgDebug("[luma.partioCacherSampler] KDtree Created");
                        AiMsgDebug("[luma.partioCacherSampler] Loaded %d points", (int)readPoints->numParticles());
                    }
                }
            }
        }

        static void* operator new(size_t count)
        {
            return AiMalloc(static_cast<unsigned int>(count));
        }

        static void operator delete(void* d)
        {
            AiFree(d);
        }
    };
};

node_parameters
{
    AiParameterENUM("mode", 0, gs_ModeNames);
    AiParameterSTR("file", NULL);
    AiParameterRGB("input", 0, 0, 0);
    AiParameterENUM("blend_mode", 0, gs_BlendMode);
    AiParameterFLT("max_search_distance", 1.0f);
    AiParameterINT("search_points", 1);
    AiParameterBOOL("scale_searchDist_by_AA_samples", false);
    AiParameterBOOL("negate_X", false);
    AiParameterBOOL("negate_Y", false);
    AiParameterBOOL("negate_Z", false);
    AiParameterRGB("diagnostic_color", 1, 1, 0);
    AiParameterVec("diagnostic_disp", 0, 1, 0);
    AiParameterBOOL("show_diagnostic", false);

    AiParameterSTR("aov_diagnostic", "diagnostic");
    AiMetaDataSetInt(mds, "aov_diagnostic", "aov.type", AI_TYPE_FLOAT);

    AiMetaDataSetStr(mds, 0, "maya.classification", "shader/surface");
    AiMetaDataSetInt(mds, NULL, "maya.id", ID_ARNOLD_PARTIO_CACHER_SAMPLER);
}

node_initialize
{
    ShaderData* data = new ShaderData();
    AiNodeSetLocalData(node, data);
}

node_update
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
    data->update(node);
}

node_finish
{
    delete reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
}

shader_evaluate
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
    const AtRGB input = AiShaderEvalParamRGB(p_input);
    if (data->mode == MODE_PASSTHROUGH || (data->mode == MODE_WRITE && sg->Rt != AI_RAY_CAMERA)) /// PASSTHRU MODE
    {
        sg->out.RGB = input;
        return;
    }
    else if (data->mode == MODE_WRITE) /// WRITE MODE
    {
        ThreadData& tdata = data->threadData[sg->tid];
        // beauty: this must happen first to trigger downstream evaluation of AOVs
        tdata.RGB.push_back(input);
        // point
        tdata.P.push_back(sg->P);
        sg->out.RGB = input;
    } /// end WRITE MODE
    else if (data->readPoints != 0) /// READ MODE
    {
        ThreadData& tdata = data->threadData[sg->tid];
        // apparently  ray type for  displacement is  AI_RAY_UNDEFINED?
        if (sg->sc == AI_CONTEXT_DISPLACEMENT)
        {
            if (sg->Rt == AI_RAY_UNDEFINED)
                tdata.total_primary_samples += 1;
            else
                tdata.total_secondary_samples += 1;
        }
        else if (sg->sc == AI_CONTEXT_SURFACE)
        {
            if (sg->Rt == AI_RAY_CAMERA)
                tdata.total_primary_samples += 1;
            else
                tdata.total_secondary_samples += 1;
        }

        const float* inputPoint = reinterpret_cast<const float*>(&sg->P);

        std::vector<PARTIO::ParticleIndex> indexes;
        std::vector<float> pointDistanceSquared;

        const float maxSearchDistance = data->searchDist;
        int firstSearchPoints = data->searchPoints;
        float firstSearchDistance = maxSearchDistance;

        if (data->blendMode == BLEND_NONE || data->blendMode == BLEND_ERROR)
        {
            firstSearchPoints = 1;
            firstSearchDistance = maxSearchDistance * .01f;
            /// bit of a magic number but  should suffice to allow for  searching for  points that are nearly at the search position
        }

        // this also does the find  points for BLEND_ALL
        /// not sure if this is the best use of LocalData? 
        PARTIO::ParticlesDataMutable* readPoints = (PARTIO::ParticlesDataMutable*)AiNodeGetLocalData(node);
        readPoints->findNPoints(inputPoint, firstSearchPoints, firstSearchDistance, indexes, pointDistanceSquared);

        // if no points within the first search distance /count
        if (indexes.empty())
        {
            if (data->blendMode == BLEND_ERROR)
                readPoints->findNPoints(inputPoint, data->searchPoints, maxSearchDistance, indexes, pointDistanceSquared);

            // if no points within the final search distance /count
            if (indexes.empty())
            {
                sg->out.RGB = input;

                if (data->diag)
                {
                    AtColor errorColor = AiShaderEvalParamRGB(p_error_color);
                    AtVector errorDir = AiShaderEvalParamVec(p_error_disp);

                    //AiMsgInfo("failed to get closest point");
                    if (sg->sc == AI_CONTEXT_SURFACE)
                        sg->out.RGB = errorColor;
                    else if (sg->sc == AI_CONTEXT_DISPLACEMENT)
                        sg->out.VEC = errorDir;
                }

                AiAOVSetFlt(sg, AiShaderEvalParamStr(p_aov_diag), 1.0f);
                if (sg->sc == AI_CONTEXT_DISPLACEMENT)
                {
                    if (sg->Rt == AI_RAY_UNDEFINED)
                        tdata.missed_primary_samples += 1;
                    else
                        tdata.missed_secondary_samples += 1;
                }
                else if (sg->sc == AI_CONTEXT_SURFACE)
                {
                    if (sg->Rt == AI_RAY_CAMERA)
                        tdata.missed_primary_samples += 1;
                    else
                        tdata.missed_secondary_samples += 1;
                }
                return;
            }
        }

        /// need to sort the arrays.. partio does not return an ordered list of nearest neighbors.... yet
        std::vector<std::pair<unsigned int, float> > distanceSorted;
        distanceSorted.reserve(indexes.size());
        for (uint i = 0; i < indexes.size(); i++)
        {
            std::pair<int, float> entry;
            entry.first = static_cast<unsigned int>(indexes[i]); /// Partio is quite fucked up with this
            /// they are using 64 bit indices, but in reality the library can't access
            /// more than 32 bit worth of particles, because the access functions use integers
            entry.second = pointDistanceSquared[i];
            distanceSorted.push_back(entry);
        }
        std::sort(distanceSorted.begin(), distanceSorted.end(), sort_pred());

        // const float minDist = std::isnan(distanceSorted.front().second) ? 0.0f : distanceSorted.front().second;
        // const float maxDist = std::isnan(distanceSorted.back().second) ? maxSearchDistance : distanceSorted.back().second;
        /// End is after the last point! Back is the last element
        /// I'm using front here, so it'll rhyme better with back. However front is not valid
        /// if there are no elements, but we already checked for that earlier, so no issues there.

        AtVector finalValue = AI_V3_ZERO;
        uint counter = 0;
        for (std::vector<std::pair<unsigned int, float> >::iterator it = distanceSorted.begin(); it != distanceSorted.end(); ++it)
        {
            if (std::isnan(it->second))
            {
                AiMsgWarning("[luma.partioCacherSampler] Found NAN inbetween min/max values!");
                continue;
            }

            counter++;

            const float* rgbVal = readPoints->data<float>(data->rgbAttr, it->first);
            finalValue.x += data->negX ? -rgbVal[0] : rgbVal[0];
            finalValue.y += data->negY ? -rgbVal[1] : rgbVal[1];
            finalValue.z += data->negZ ? -rgbVal[2] : rgbVal[2];
        } // searchPoints loop

        if (counter > 1)
            finalValue *= 1.0f / static_cast<float>(counter);
        sg->out.VEC = finalValue;
        /// out.RGB and out.VEC are pointing to the same memory, as both are part of the same union.
        /// There is no need to split the logic.
    } /// end READ MODE
}
