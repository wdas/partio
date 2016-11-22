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
 *            I know I haven't switched over the   initialize stuff to  "update"
 *            The sampling  lerp stuff may need a major overhaul but I don't know if thats the root cause of the threading issues. 
 * 
 *Please add notes where you see fit, I'd like to learn what I'm doing wrong while we're fixing this.
 * 
 *  Is there a way to make one shader have multiple connection  outputs?   a float and  an RGB for example? so one could be connected 
 * to the  normalDisplacement  and the rgb could go to the vector displacement,  or do I have to separate it out into 2 separate shaders? 
 */


//#define MAX_THREADS 64
#define ID_ARNOLD_PARTIO_CACHER_SAMPLER       0x00116ED2

AI_SHADER_NODE_EXPORT_METHODS(partioCacherSamplerMtd);

namespace
{

//using namespace Partio;
using namespace std;

enum Mode
{
    MODE_READ = 0,
    MODE_WRITE,
    MODE_PASSTHROUGH
};

// not sure if  i need to bother with this possibly can just internally switch based on shading context instead 
// currently not being used
enum shadingMode
{
    SHAD_SURFACE = 0,
    SHAD_DISPLACEMENT
};

enum Blend_Mode
{
    BLEND_NONE = 0,
    BLEND_ERROR,
    BLEND_ALL
};

struct sort_pred {
    bool operator()(const std::pair<int,float> &left, const std::pair<int,float> &right) {
        return left.second < right.second;
    }
};


const char *gs_ModeNames[] =
{
    "read",
    "write",
    "passthrough",
    NULL
};

// not sure if  i need to bother with this possibly can just internally switch based on shading context instead 
const char *gs_shadModeNames[] =
{
    "SurfaceColor",
    "Displacement",
    NULL
};

const char *gs_BlendMode[] =
{
    "Blend None",
    "Blend Error",
    "Blend All",
    NULL
};

enum partioPointCacherParams
{
    p_shadMode,
    p_mode,
    p_file,
    p_input,
    p_blend_mode,
    p_search_distance,
    p_search_points,
    p_scale_searchDist_by_aa,
    p_error_color,
    p_error_disp,
    p_diag,
    p_aov_diag
};

};

inline float lerp(float t, float a, float b)
{
    return  a+(t*(b-a));
}

template <class T>
T Clamp(T value, T low, T high)
{
    return value < low ? low : (value > high ? high : value );
}

node_parameters
{
    AiParameterENUM("shadingMode", 0, gs_shadModeNames);
    AiParameterENUM("mode", 0, gs_ModeNames);
    AiParameterSTR("file", NULL);
    AiParameterRGB("input", 0, 0, 0);
    AiParameterENUM("blend_mode",0, gs_BlendMode);
    AiParameterFLT("max_search_distance", 1.0f);
    AiParameterINT("search_points", 1);
    AiParameterBOOL("scale_searchDist_by_AA_samples", false);
    AiParameterRGB("diagnostic_color", 1,1,0);
    AiParameterVec("diagnostic_disp",  0,1,0);
    AiParameterBOOL("show_diagnostic", false);

    AiParameterSTR("aov_diagnostic", "diagnostic");
    AiMetaDataSetInt(mds, "aov_diagnostic", "aov.type", AI_TYPE_FLOAT);

    //AiMetaDataSetBool(mds, NULL, "maya.hide", true);
    AiMetaDataSetStr(mds, 0, "maya.classification", "shader/surface");
    AiMetaDataSetInt(mds, NULL, "maya.id", ID_ARNOLD_PARTIO_CACHER_SAMPLER);
}

struct AOVData
{
    uint type;
    char name;
    std::vector<AtVector>* dataA;  // vector3 types
    std::vector<AtRGBA>* dataB;  // vector4 types
    std::vector<float>* dataC;       // float types
};

typedef std::map<std::string, AOVData> AOVDataMap;

struct ThreadData
{
    std::vector<AtPoint>* P;
    std::vector<AtRGB>* RGB;
    std::vector<AtVector>* dispVec;
    AOVDataMap* AOVs;
    uint total_primary_samples;
    uint total_secondary_samples;
    uint missed_primary_samples;
    uint missed_secondary_samples;
    ThreadData() : total_primary_samples(0),
        total_secondary_samples(0),
        missed_primary_samples(0),
        missed_secondary_samples(0) {}
};

struct NodeAOVData
{
    std::string name;
    uint type;
    bool operator<(const NodeAOVData &other) const
    {
        return (name < other.name);
    }
};

static std::string file;
static int mode;
static int blendMode;
static int shadMode;
static float searchDist;
static bool initialized = false;
static bool finalized = false;
static bool diag = false;
static int  searchPoints;
static std::set<NodeAOVData> allAOVs;
static std::map<std::string, uint> AOVTypes;
static ThreadData threadData[AI_MAX_THREADS];


inline bool AiAOVGet(AtShaderGlobals *sg, const char *name, AtColor& val)
{
    return AiAOVGetRGB(sg, name, val);
}

inline bool AiAOVGet(AtShaderGlobals *sg, const char *name, AtRGBA& val)
{
    return AiAOVGetRGBA(sg, name, val);
}

inline bool AiAOVGet(AtShaderGlobals *sg, const char *name, AtVector& val)
{
    if (std::string(name) == "N") {
        val = sg->N;
    }
    else
    {
        return AiAOVGetVec(sg, name, val);
    }
    return true;
}

inline bool AiAOVGet(AtShaderGlobals *sg, const char *name, float& val)
{
    return AiAOVGetFlt(sg, name, val);
}


void cacheGlobalAOVs()
{
    AiMsgInfo("[luma.partioCacherSampler] cacheGlobalAOVs");
    // save out AOV names to static variable
    AtArray* outputs = AiNodeGetArray(AiUniverseGetOptions(), "outputs");
    for (uint i=0; i < outputs->nelements; ++i)
    {
        std::string output(AiArrayGetStr(outputs, i));
        size_t index = output.find_first_of(' ');
        std::string aov = output.substr(0, index);

        // filter standard aovs out since they are always  computed anyway except for normal since it may be usefull as a utility
        // P is always exported so we're also filtering that out here as well
        if (aov == "P" || aov == "Z" || aov == "opacity" || aov == "motionvector" ||
                aov == "Pref" || aov == "raycount" || aov == "cputime" || aov == "RGB" || aov == "RGBA")
        {
            AiMsgWarning("[luma.partioCacherSampler] AOV: \"%s\" is not being cached)",aov.c_str());
            continue;
        }
        // TODO: handle same AOV with different types (possible)?
        size_t index2 = output.find_first_of(' ', index+1);
        std::string typeName = output.substr(index+1, index2 - (index+1));
        AiMsgInfo("[luma.partioCacherSampler] AOV: \"%s\" (\"%s\")", aov.c_str(), typeName.c_str());
        uint type = AI_TYPE_RGBA;
        if (typeName == "RGB")
            type = AI_TYPE_RGB;
        else if (typeName == "VECTOR")
            type = AI_TYPE_VECTOR;
        else if (typeName == "POINT")
            type = AI_TYPE_POINT;
        else if (typeName == "RGBA")
            type = AI_TYPE_RGBA;
        else if (typeName == "FLOAT")
            type = AI_TYPE_FLOAT;
        else
            AiMsgError("[luma.partioCacherSampler] AOV \"%s\" has unsupported data type", aov.c_str());
        NodeAOVData d;
        d.name = aov;
        d.type = type;
        allAOVs.insert(d);
        AOVTypes[aov] = type;
    }
}


// I know most of  this needs to go to update
node_initialize
{

    // this grabs the  shader's supported AOVs
    AiMsgInfo("[luma.partioCacherSampler] initialize...");
    // pre-cache a pointer to aov array, so we don't have to call AiNodeGetArray in evaluate
    AtArray* aovs = AiNodeGetArray(node, "mtoa_aovs");
    if (aovs != NULL)
    {
        AtArray** paovs = (AtArray**)AiMalloc(sizeof(AtArray*));

        *paovs = aovs;
        //node->local_data = paovs;

        if (aovs->nelements)
        {
            AiMsgInfo("[luma.partioCacherSampler] %s AOVs:", AiNodeGetName(node));
            for (uint i=0; i < aovs->nelements; ++i)
            {
                const char* aovName = AiArrayGetStr(aovs, i);
                AiMsgInfo("[luma.partioCacherSampler]   \"%s\"", aovName);
            }
        }
        else
            AiMsgInfo("[luma.partioCacherSampler] %s has no AOVs", AiNodeGetName(node));
    }
    if (initialized)
    {
        if (mode != AiNodeGetInt(node, "mode"))
            AiMsgError("[luma.partioCacherSampler] all nodes do not agree on mode");
        if (shadMode != AiNodeGetInt(node, "shadingMode"))
            AiMsgError("[luma.partioCacherSampler] all nodes do not agree on shadingMode");
        return;
    }

    initialized = true;
    AtNode* options = AiUniverseGetOptions();
    file = AiNodeGetStr(node, "file");
    // hacky check for swatches
    if (file == "" || (AiNodeGetInt(options, "xres") == 64 && AiNodeGetInt(options, "yres") == 64))
    {
        mode = MODE_PASSTHROUGH;
        shadMode = SHAD_DISPLACEMENT;
        AiMsgInfo("not active");
        return;
    }

    mode = AiNodeGetInt(node, "mode");
    shadMode = AiNodeGetInt(node,"shadingMode");


    if (mode == MODE_PASSTHROUGH)
    {
        AiMsgInfo("[luma.partioCacherSampler] passthrough mode");
        return;
    }

    // here we grab any other aov's we support from the globals... for now  just  N if it is enabled.
    cacheGlobalAOVs();

    if (mode == MODE_WRITE)
    {
        AiMsgInfo("[luma.partioCacherSampler] write mode (\"%s\")", file.c_str());
        for (uint t=0; t < AI_MAX_THREADS; ++t)
        {
            threadData[t].P = new std::vector<AtPoint>;
            threadData[t].RGB = new std::vector<AtRGB>;
            threadData[t].dispVec = new std::vector<AtVector>;

/*
            AOVDataMap* map = new AOVDataMap;
            std::set<NodeAOVData>::iterator it;
            for (it=allAOVs.begin(); it != allAOVs.end(); ++it)
            {
                AOVData d;
                d.type = it->type;

                switch (it->type)
                {
                case AI_TYPE_RGB:
                case AI_TYPE_VECTOR:
                case AI_TYPE_POINT:
                    d.dataA = new std::vector<AtVector> ;
                    (*map)[it->name] = d;
                    break;
                case AI_TYPE_RGBA:
                    d.dataB = new std::vector<AtRGBA>;
                    (*map)[it->name] = d;
                    break;
                case AI_TYPE_FLOAT:
                    d.dataC = new std::vector<float>;
                    (*map)[it->name] = d;
                    break;
                default:
                    AiMsgError("[luma.partioCacherSampler] AOV \"%s\" has unsupported data type", it->name.c_str());
                    return;
                }
            }
            threadData[t].AOVs = map;
*/
        }
    }


    else if (mode == MODE_READ)
    {
        AiMsgInfo("[luma.partioCacherSampler] read mode (\"%s\")", file.c_str());

        diag = AiNodeGetBool(node, "show_diagnostic") == true;

        uint AASamples =  AiNodeGetInt(AiUniverseGetOptions(), "AA_samples") ;

        blendMode = AiNodeGetInt(node,"blend_mode");

        searchDist = AiNodeGetFlt(node, "max_search_distance");

        if (AiNodeGetBool(node, "scale_searchDist_by_AA_samples"))
        {
            searchDist *= sqrt(AASamples);
        }

        searchPoints = AiNodeGetInt(node,"search_points");
        if(searchPoints <=0)
        {
            searchPoints = 1;
        }
        PARTIO::ParticlesDataMutable* readPoints;
        readPoints=PARTIO::read(file.c_str());
        if (readPoints)
        {
            AiMsgInfo("[luma.partioCacherSampler] sorting points");
            readPoints->sort();
            AiMsgInfo("[luma.partioCacherSampler] KDtree Created");
            AiMsgInfo("[luma.partioCacherSampler] loaded %d points", (int)readPoints->numParticles());
        }
        //AiNodeSetLocalData(node, AiMalloc(sizeof(readPoints)));
        AiNodeSetLocalData(node,readPoints);
    }
}


/// TODO:  move most init stuff here 
node_update
{
}

node_finish
{
    AiMsgInfo("[luma.partioCacherSampler] finish...");


    if (finalized)
        return;

    AiMsgInfo("finish %s", AiNodeGetName(node));

    finalized = true;

    if (mode == MODE_WRITE) // write
    {
        std::vector<AtPoint> allP;
        std::vector<AtRGB> allRGB;
        uint t=0;
        uint threadCounter = 0;
        for (t=0; t < AI_MAX_THREADS; ++t)
        {
            if (threadData[t].P->size())
            {
                threadCounter ++;
            }
            allP.insert(allP.end(), threadData[t].P->begin(), threadData[t].P->end());
            delete threadData[t].P;
            allRGB.insert(allRGB.end(), threadData[t].RGB->begin(), threadData[t].RGB->end());
            delete threadData[t].RGB;
        }
        AiMsgInfo("[luma.partioCacherSampler] gathered %d points from %i threads.", (int)allP.size(),threadCounter);

        PARTIO::ParticlesDataMutable* writePoints=PARTIO::createInterleave();

        writePoints->addParticles(allP.size());

        PARTIO::ParticleAttribute position=writePoints->addAttribute("position",PARTIO::VECTOR,3);
        PARTIO::ParticleAttribute id=writePoints->addAttribute("id",PARTIO::INT,1);
        PARTIO::ParticleAttribute rgb = writePoints->addAttribute("rgbPP", PARTIO::VECTOR,3);

        PARTIO::ParticlesDataMutable::iterator it= writePoints->begin();
        PARTIO::ParticleAccessor positionAccess(position);
        it.addAccessor(positionAccess);
        PARTIO::ParticleAccessor idAccess(id);
        it.addAccessor(idAccess);
        PARTIO::ParticleAccessor rgbAccess(rgb);
        it.addAccessor(rgbAccess);

        int idCounter=0;
        for (it = writePoints->begin(); it!=writePoints->end(); ++it)
        {
            PARTIO::Data<float,3>& P=positionAccess.data<PARTIO::Data<float,3> >(it);
            PARTIO::Data<int,1>& id=idAccess.data<PARTIO::Data<int,1> >(it);
            PARTIO::Data<float,3>& color= rgbAccess.data<PARTIO::Data<float,3> >(it);

            P[0]=allP[idCounter].x;
            P[1]=allP[idCounter].y;
            P[2]=allP[idCounter].z;

            id[0]=idCounter;

            color[0] = allRGB[idCounter].r;
            color[1] = allRGB[idCounter].g;
            color[2] = allRGB[idCounter].b;

            idCounter++;
        }

        std::set<NodeAOVData>::iterator itAov;
        for (itAov=allAOVs.begin(); itAov != allAOVs.end(); ++itAov)
        {

            /// reset the iterator
            it=writePoints->begin();

            switch (itAov->type)
            {
            case AI_TYPE_RGB:
            case AI_TYPE_VECTOR:
            case AI_TYPE_POINT:
            {

                std::vector<AtVector> allVec;
                for (unsigned int t=0; t < AI_MAX_THREADS; ++t)
                {
                    std::vector<AtVector>* pData = threadData[t].AOVs->find(itAov->name)->second.dataA;
                    allVec.insert(allVec.end(), pData->begin(), pData->end());
                }

                if (allVec.size())
                {
                    PARTIO::ParticleAttribute  vectorAttribute = writePoints->addAttribute(itAov->name.c_str(), PARTIO::VECTOR, 3);
                    PARTIO::ParticleAccessor vectorAccess(vectorAttribute);
                    it.addAccessor(vectorAccess);
                    int a = 0;
                    for (it=writePoints->begin(); it!=writePoints->end(); ++it)
                    {
                        PARTIO::Data<float,3>& vecData=vectorAccess.data<PARTIO::Data<float,3> >(it);
                        vecData[0] = (float)allVec[a].x;
                        vecData[1] = (float)allVec[a].y;
                        vecData[2] = (float)allVec[a].z;
                        a++;
                    }
                }
                else
                {
                    AiMsgError("[luma.partioCacherSampler] no rgb data for  %s", itAov->name.c_str());
                }

                break;
            }

            case AI_TYPE_RGBA:
            {

                std::vector<AtRGBA> allPoint;
                for (unsigned int t=0; t < AI_MAX_THREADS; ++t)
                {
                    std::vector<AtRGBA>* pData = threadData[t].AOVs->find(itAov->name)->second.dataB;
                    allPoint.insert(allPoint.end(), pData->begin(), pData->end());
                }

                if (allPoint.size())
                {
                    PARTIO::ParticleAttribute  pointAttribute = writePoints->addAttribute(itAov->name.c_str(), PARTIO::FLOAT, 4);
                    PARTIO::ParticleAccessor pointAccess(pointAttribute);
                    it.addAccessor(pointAccess);
                    int a = 0;
                    for (it=writePoints->begin(); it!=writePoints->end(); ++it)
                    {
                        PARTIO::Data<float,4>& pointData=pointAccess.data<PARTIO::Data<float,4> >(it);
                        pointData[0] = (float)allPoint[a].r;
                        pointData[1] = (float)allPoint[a].g;
                        pointData[2] = (float)allPoint[a].b;
                        pointData[3] = (float)allPoint[a].a;
                        a++;
                    }
                }
                else
                {
                    AiMsgError("[luma.partioCacherSampler] no rgba data for  %s", itAov->name.c_str());
                }
                break;
            }

            case AI_TYPE_FLOAT:
            {

                std::vector<float> allFlt;
                for (unsigned int t=0; t < AI_MAX_THREADS; ++t)
                {
                    std::vector<float>* pData = threadData[t].AOVs->find(itAov->name)->second.dataC;
                    allFlt.insert(allFlt.end(), pData->begin(), pData->end());
                }

                if (allFlt.size())
                {
                    PARTIO::ParticleAttribute  floatAttribute = writePoints->addAttribute(itAov->name.c_str(), PARTIO::FLOAT, 1);
                    PARTIO::ParticleAccessor floatAccess(floatAttribute);
                    it.addAccessor(floatAccess);
                    int a = 0;
                    for (it=writePoints->begin(); it!=writePoints->end(); ++it)
                    {
                        PARTIO::Data<float,1>& fltData=floatAccess.data<PARTIO::Data<float,1> >(it);
                        fltData[0] = (float)allFlt[a];
                        a++;
                    }
                }
                else
                {
                    AiMsgError("[luma.partioCacherSampler] no float data for  %s", itAov->name.c_str());
                }

                break;
            }
            }/// end TYPE switch
        } /// end forAOV loop

        // delete AOVDataMap
        for (uint t=0; t < AI_MAX_THREADS; ++t)
        {
            delete threadData[t].AOVs;
        }

        AiMsgInfo("[luma.partioCacherSampler] writing %d points to \"%s\"", (int)allP.size(), file.c_str());

        // WRITE OUT HERE
        write(file.c_str(), *writePoints);
        writePoints->release();
        AiMsgInfo("[luma.partioCacherSampler] released  memory");

    }

    else if (mode == MODE_READ) /// read
    {
        uint missed_primary = 0;
        uint missed_secondary = 0;
        uint total_primary = 0;
        uint total_secondary = 0;
        for (int i=0; i < AI_MAX_THREADS; ++i)
        {
            missed_primary += threadData[i].missed_primary_samples;
            missed_secondary += threadData[i].missed_secondary_samples;
            total_primary += threadData[i].total_primary_samples;
            total_secondary += threadData[i].total_secondary_samples;
        }
        AiMsgInfo("[luma.partioCacherSampler] primary samples: %d total, %d missed (%.02f%%)", total_primary, missed_primary,
                  (total_primary == 0) ? 0.0f : (missed_primary/(float)total_primary*100.0f));

        AiMsgInfo("[luma.partioCacherSampler] secondary samples: %d total, %d missed (%.02f%%)", total_secondary, missed_secondary,
                  (total_secondary == 0) ? 0.0f : (missed_secondary/(float)total_secondary*100.0f));

        PARTIO::ParticlesDataMutable* readPoints = (PARTIO::ParticlesDataMutable*)AiNodeGetLocalData(node);
        readPoints->release();
        AiMsgInfo("[luma.partioCacherSampler] released  memory");
        
        /// doesn't work,  double free corrupt crash 
        //if(readPoints != NULL)
         //   AiFree(AiNodeGetLocalData(node));
    }

}

shader_evaluate
{

    AtRGB rgb = AiShaderEvalParamRGB(p_input);
    AtVector disp;
    AiV3Create(disp, rgb.r, rgb.g, rgb.b);
/// PASSTHRU MODE
    // not sure if  displacement context uses other rays besides  camera, so commented it out for now
    if (mode == MODE_PASSTHROUGH ||
    (mode == MODE_WRITE && sg->Rt != AI_RAY_CAMERA)// ||
    //(mode == MODE_READ && sg->Rt != AI_RAY_CAMERA))
    )
    {
        sg->out.RGB = AiShaderEvalParamRGB(p_input);
        sg->out.VEC = disp;
        return;
    }

    ThreadData *tdata = &threadData[sg->tid];

/// WRITE MODE
    if (mode == MODE_WRITE)
    {
        // beauty: this must happen first to trigger downstream evaluation of AOVs

        tdata->RGB->push_back(rgb);

        // point
        tdata->P->push_back(sg->P);

        // disp
        tdata->dispVec->push_back(disp);

/// AOV STUFFS 
/*
        std::set<NodeAOVData>::iterator itAOV;


        if (allAOVs.size())

        {
            for (itAOV=allAOVs.begin(); itAOV != allAOVs.end(); ++itAOV)
            {
                const char* aovName = itAOV->name.c_str();
                AOVDataMap::iterator it = tdata->AOVs->find(aovName);
                if (it != tdata->AOVs->end())
                {
                    switch (it->second.type)
                    {
                    case AI_TYPE_RGB:
                    case AI_TYPE_VECTOR:
                    case AI_TYPE_POINT:
                    {
                        AtVector v;
                        if (AiAOVGet(sg, aovName, v))
                        {
                            it->second.dataA->push_back(v);
                        }
                    }
                    break;
                    case AI_TYPE_RGBA:
                    {
                        AtRGBA rgba;
                        if (AiAOVGet(sg, aovName, rgba))
                        {
                            it->second.dataB->push_back(rgba);
                        }
                    }
                    break;
                    case AI_TYPE_FLOAT:
                    {
                        float f;
                        if (AiAOVGet(sg, aovName, f))
                        {
                            it->second.dataC->push_back(f);
                        }
                    }
                    break;
                    default:
                        AiMsgError("[luma.partioCacherSampler] AOV \"%s\" has unsupported data type", aovName);
                        return;
                    }
                }
                else
                {
                    AiMsgWarning("AOV: '%s': could not find", aovName);
                }
            }
        }
*/


        //if (sg->sc == AI_CONTEXT_SURFACE)
            sg->out.RGB = rgb;

        //if (sg->sc == AI_CONTEXT_DISPLACEMENT)
            sg->out.VEC = disp;

    } /// end WRITE MODE

///  READ MODE
    else if (mode == MODE_READ)
    {
        //  we only want primary rays, not  secondary  right? 
        if (sg->Rt == AI_RAY_CAMERA && (sg->sc == AI_CONTEXT_SURFACE || sg->sc == AI_CONTEXT_DISPLACEMENT))
            tdata->total_primary_samples += 1;
        else
            tdata->total_secondary_samples += 1;

        float* inputPoint = reinterpret_cast<float*>(&sg->P);

        std::vector<PARTIO::ParticleIndex> indexes;
        std::vector<float> pointDistanceSquared;

        float maxSearchDistance = searchDist;
        int firstSearchPoints = searchPoints;
        float firstSearchDistance = maxSearchDistance;

        if(blendMode == BLEND_NONE || blendMode == BLEND_ERROR)
        {
            firstSearchPoints = 1;
            firstSearchDistance = maxSearchDistance*.01;
            /// bit of a magic number but  should suffice to allow for  searching for  points that are nearly at the search position
        }

        // this also does the find  points for BLEND_ALL
        /// not sure if this is the best use of LocalData? 
        PARTIO::ParticlesDataMutable* readPoints = (PARTIO::ParticlesDataMutable*)AiNodeGetLocalData(node);
        readPoints->findNPoints(inputPoint, firstSearchPoints, firstSearchDistance, indexes, pointDistanceSquared);


        // if no points within the first search distance /count
        if (!indexes.size())
        {
            if (blendMode == BLEND_ERROR )
            {
                readPoints->findNPoints(inputPoint, searchPoints, maxSearchDistance, indexes, pointDistanceSquared);
            }

            // if no points within the final search distance /count
            if (!indexes.size())
            {
                sg->out.RGB = AiShaderEvalParamRGB(p_input);
                sg->out.VEC = disp;
                if (diag)
                {
                    AtColor errorColor = AiShaderEvalParamRGB(p_error_color);

                    //AiMsgInfo("failed to get closest point");
                    sg->out.RGB.r = errorColor.r;
                    sg->out.RGB.g = errorColor.g;
                    sg->out.RGB.b = errorColor.b;
                    AtVector errorDir =  AiShaderEvalParamVec(p_error_disp);
                    sg->out.VEC = errorDir;
                }


                AiAOVSetFlt(sg, AiShaderEvalParamStr(p_aov_diag), 1.0f);
                if (sg->Rt == AI_RAY_CAMERA && (sg->sc == AI_CONTEXT_SURFACE || sg->sc == AI_CONTEXT_DISPLACEMENT))
                {
                    tdata->missed_primary_samples += 1;
                }
                else
                {
                    tdata->missed_secondary_samples += 1;
                }
                return;
            }
        }

        /// need to sort the arrays.. partio does not return an ordered list of nearest neighbors.... yet

        std::vector<std::pair<int,float> > distanceSorted;
        distanceSorted.clear();
        for (uint i = 0; i < indexes.size(); i++)
        {
            std::pair<int,float> entry;
            entry.first = indexes[i];
            entry.second = pointDistanceSquared[i];
            distanceSorted.push_back(entry);

        }
        std::sort(distanceSorted.begin(), distanceSorted.end(),sort_pred());

        // we check here because there are some cases that are returning  these as NANs
        float minDist, maxDist;
        if (isnan(distanceSorted.begin()->second))
        {
            minDist = 0;
            AiMsgWarning("[luma.partioCacherSampler] found NAN in minDist, clamping...");
        }
        else
        {
            minDist = distanceSorted.begin()->second;
        }
        if (isnan(distanceSorted.end()->second))
        {
            maxDist = maxSearchDistance;
            AiMsgWarning("[luma.partioCacherSampler] found NAN in maxDist, clamping...");
        }
        else
        {
            maxDist = distanceSorted.end()->second;
        }

        /// temporary storage for  sample iteration
        AtRGB finalRGB = AI_RGB_BLACK;
        AtVector finalDisp = AI_V3_ZERO;
        std::vector<std::pair<int,float> >::iterator it;
/*
        std::map<const char*, AtRGB> aovRGB;
        std::map<const char*, AtRGBA> aovRGBA;
        std::map<const char*, float> aovFLOAT;

        std::set<NodeAOVData>::iterator itAOV;

        if (allAOVs.size())
        {
            for (itAOV=allAOVs.begin(); itAOV != allAOVs.end(); ++itAOV)
            {
                const char* aovName = itAOV->name.c_str();

                PARTIO::ParticleAttribute aovAttr;
                if (points->attributeInfo(aovName,aovAttr))
                {
                    if (aovAttr.type == PARTIO::VECTOR)
                    {
                        aovRGB.insert(pair<const char*, AtRGB> (aovName,AI_RGB_BLACK));
                    }
                    else if  (aovAttr.type == PARTIO::FLOAT )
                    {
                        if (aovAttr.count > 1)
                        {
                            aovRGBA.insert(pair<const char*, AtRGBA> (aovName,AI_RGBA_BLACK));
                        }
                        else
                        {
                            aovFLOAT.insert(pair<const char*, float> (aovName,0.0));
                        }
                    }
                    else
                    {
                        AiMsgWarning("[luma.partioCacherSampler] AOV: '%s': unknown data type", aovName);
                    }
                }
                else
                {
                    AiMsgWarning("[luma.partioCacherSampler] AOV: '%s': could not find", aovName);
                }
            }
        }
*/
        std::map<const char*,AtRGB>::iterator rgbit;
        std::map<const char*,AtRGBA>::iterator rgbAit;
        std::map<const char*,float>::iterator fltit;
        
        // once this loop is longer than 1,  i start getting the  bucket "blocks" in the render
        for (it = distanceSorted.begin(); it != distanceSorted.end(); ++it)
        {

            bool first = false;
            if (it == distanceSorted.begin())
            {
                first = true;
            }
            if (isnan(it->second))
            {
                AiMsgError("Found NAN inbetween min/max values!");
                continue;
            }
            // we may not need this anymore since the NANS seemed to be coming from above....
            float lerpVal = Clamp((1-(lerp(it->second, minDist,maxDist))),0.0f,1.0f);

/*
            if (aovRGB.size())
            {
                for (rgbit=aovRGB.begin(); rgbit != aovRGB.end(); ++rgbit)
                {
                    PARTIO::ParticleAttribute aovAttr;

                    if (points->attributeInfo(rgbit->first,aovAttr))
                    {
                        const float* vectorVal = points->data<float>(aovAttr, it->first);
                        AtRGB vv = {vectorVal[0],vectorVal[1],vectorVal[2]};
                        rgbit->second = AiColorLerp(lerpVal,vv, rgbit->second);
                        if(!first)
                        {
                            rgbit->second = AiColorLerp(lerpVal,vv, rgbit->second);

                        }
                        else
                        {
                            rgbit->second = vv;
                        }
                    }
                }
            }
            if (aovRGBA.size())
            {
                for (rgbAit=aovRGBA.begin(); rgbAit != aovRGBA.end(); ++rgbAit)
                {
                    PARTIO::ParticleAttribute aovAttr;

                    if (points->attributeInfo(rgbAit->first,aovAttr))
                    {
                        const float* vectorVal = points->data<float>(aovAttr, it->first);
                        AtRGBA vva = {vectorVal[0],vectorVal[1],vectorVal[2],vectorVal[3]};
                        if(!first)
                        {
                            rgbAit->second = AiColorLerp(lerpVal,vva, rgbAit->second);
                        }
                        else
                        {
                            rgbAit->second = vva;
                        }
                    }
                }
            }
            if (aovFLOAT.size())
            {
                for (fltit=aovFLOAT.begin(); fltit != aovFLOAT.end(); ++fltit)
                {
                    PARTIO::ParticleAttribute aovAttr;

                    if (points->attributeInfo(fltit->first,aovAttr))
                    {
                        const float* floatVal = points->data<float>(aovAttr, it->first);
                        if(!first)
                        {
                            fltit->second  = lerp(lerpVal,*floatVal, fltit->second);
                        }
                        else
                        {
                            fltit->second = *floatVal;
                        }
                    }
                }
            }
*/
            PARTIO::ParticleAttribute rgbAttr;
            // hardcoded for now, but will eventually want to put in a pulldown menu like vdbSampler
            if (!readPoints->attributeInfo("Cd", rgbAttr))
            {
                AiMsgError("EMPTY!");
                sg->out.RGB = AI_RGB_BLACK;
                sg->out.VEC = AI_V3_ZERO;
                return;
            }

            const float *rgbVal = readPoints->data<float>(rgbAttr, it->first);
            AtRGB vv = {rgbVal[0],rgbVal[1],rgbVal[2]};
            AtVector dv = {rgbVal[0],rgbVal[1],rgbVal[2]};
            if(!first)
            {
                vv = AiColorLerp(lerpVal,vv, finalRGB);
                dv = AiV3Lerp(lerpVal, dv, finalDisp);
            }
            finalRGB = vv;
            finalDisp = dv;

        } // searchPoints loop


        /// NOT SURE if it matters or makes a  difference to arnold if i use a color  out.RGB  to feed  a vector input like 
        /// vector  displacment in the shading networks?  so  I started adding both output types?  
        
        sg->out.RGB = finalRGB;
        sg->out.VEC = finalDisp;

/*
        if (aovRGB.size())
        {
            for (rgbit=aovRGB.begin(); rgbit != aovRGB.end(); ++rgbit)
            {
                if(std::string(rgbit->first) == "N")
                {
                    AtPoint foo;
                    foo.x = rgbit->second.r;
                    foo.y = rgbit->second.g;
                    foo.z = rgbit->second.b;
                    sg->N = foo;
                }
                else
                {
                    AiAOVSetRGB(sg, rgbit->first, rgbit->second);
                }
            }
        }
        if (aovRGBA.size())
        {
            for (rgbAit=aovRGBA.begin(); rgbAit != aovRGBA.end(); ++rgbAit)
            {
                AiAOVSetRGBA(sg, rgbAit->first, rgbAit->second);
            }
        }
        if (aovFLOAT.size())
        {
            for (fltit=aovFLOAT.begin(); fltit != aovFLOAT.end(); ++fltit)
            {
                AiAOVSetFlt(sg, fltit->first, fltit->second);
            }
        }
*/
    } /// end read mode
    else
    {
        AiMsgError("invalid mode!");
    }
}
