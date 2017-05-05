#include <ai.h>


AI_SHADER_NODE_EXPORT_METHODS(partioPointMtd);

namespace {
    enum partioPointParams {
        p_overrideRgbPP,
        p_particleColor,
        p_overrideOpacityPP,
        p_particleOpacity,
        p_doShadowOpacity,
        p_useLighting,
        p_Kd,
        p_Ki,
        p_Ks,
        p_shiny,
        p_KOcc,
        p_occSamples,
        p_occSpread,
        p_occFalloff,
        p_occMinDist,
        p_occMaxDist,
        p_invertOcc,
        p_emission,
        p_emFromIncand,
        p_aov_emission
    };


    struct ShaderData {
        AtSampler* sampler;
        AtString aov_emission;

        ShaderData() : sampler(0)
        {

        }

        ~ShaderData()
        {
            if (sampler) {
                AiSamplerDestroy(sampler);
            }
        }

        void update(AtNode* node)
        {
            if (sampler) {
                AiSamplerDestroy(sampler);
            }

            sampler = AiSampler(static_cast<uint32_t>(AiNodeGetInt(node, "occSamples")), 2, 2);
            aov_emission = AtString(AiNodeGetStr(node, "aov_emission"));
        }

        void* operator new(size_t size)
        {
            return AiMalloc(static_cast<unsigned int>(size));
        }

        void operator delete(void* d)
        {
            AiFree(d);
        }
    };

    void setup_opacity(AtShaderGlobals* sg, AtNode* node)
    {
        const static AtString opacityPP("opacityPP");
        AtRGB out_opacity;
        if (AiShaderEvalParamBool(p_overrideOpacityPP)) {
            out_opacity = AiShaderEvalParamRGB(p_particleOpacity);
        } else if (AiUDataGetFlt(opacityPP, out_opacity.r)) {
            out_opacity.b = out_opacity.g = out_opacity.r;
        }
#warning TODO!
        AiColorClipToZero(out_opacity);
    }
}

node_parameters
{
    AiParameterBool("overrideRgbPP", false);
    AiParameterRGB("particleColor", 1.0f, 1.0f, 1.0f);
    AiParameterBool("overrideOpacityPP", false);
    AiParameterRGB("particleOpacity", 1.0f, 1.0f, 1.0f);
    AiParameterBool("doShadowOpacity", false);
    AiParameterBool("useLighting", true);
    AiParameterFlt("Kd", 0.8f);
    AiParameterFlt("Ki", 1.0f);
    AiParameterFlt("Ks", 1.0f);
    AiParameterFlt("shiny", 10.0f);
    AiParameterFlt("KOcc", 1.0f);
    AiParameterInt("occSamples", 5);
    AiParameterFlt("occSpread", 90.0f);
    AiParameterFlt("occFalloff", 0.0f);
    AiParameterFlt("occMinDist", 0.0f);
    AiParameterFlt("occMaxDist", 2.0f);
    AiParameterBool("invertOcc", false);
    AiParameterRGB("emission", 0.0f, 0.0f, 0.0f);
    AiParameterBool("emColorFromIncand", false);
    AiParameterStr("aov_emission", "emission");

    AiMetaDataSetStr(nentry, nullptr, "maya.classification", "shader/surface");
    AiMetaDataSetStr(nentry, nullptr, "maya.name", "partioPoint");
    //AiMetaDataSetInt(mds, NULL, "maya.id", NULL);
}

node_initialize
{
    ShaderData* data = new ShaderData();
    AiNodeSetLocalData(node, data);
}

///////////////////////////////////////
/// UPDATE
node_update
{
    ShaderData* data = reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
    data->update(node);
}

//////////////////////////////////////
/// FINISH
node_finish
{
    delete reinterpret_cast<ShaderData*>(AiNodeGetLocalData(node));
}

shader_evaluate
{
    if (sg->Rt & AI_RAY_SHADOW) {
        sg->out_opacity = AI_RGB_WHITE;
        if (AiShaderEvalParamBool(p_doShadowOpacity)) {
            setup_opacity(sg, node);
        }
        return;
    }

    const ShaderData* data = (const ShaderData*)AiNodeGetLocalData(node);

    sg->out_opacity = AI_RGB_WHITE;
    sg->out.RGB() = AI_RGB_BLACK;
    sg->out.RGBA().a = 1.0f;

    setup_opacity(sg, node);

    if (AiShaderGlobalsApplyOpacity(sg, sg->out_opacity)) {
        return;
    }

    if (AiShaderEvalParamBool(p_overrideRgbPP)) {
        sg->out.RGB = AiShaderEvalParamRGB(p_particleColor);
    } else
        AiUDataGetRGB("rgbPP", &sg->out.RGB);

    if (AiShaderEvalParamBool(p_useLighting)) {
        AtRGB lighting = AI_RGB_BLACK;
        const float Kd = AiShaderEvalParamFlt(p_Kd);
        const float Ki = AiShaderEvalParamFlt(p_Ki);
        if (Kd > 0.0f) {
            lighting += AiDirectDiffuse(sg->Nf, sg) * Kd;
        }
        if (Ki > 0.0f) {
            lighting += AiIndirectDiffuse(sg->Nf, sg) * Ki;
        }

        sg->out.RGB() *= lighting;

        const float Ks = AiShaderEvalParamFlt(p_Ks);
        if (Ks > 0.0f) {
            const float shiny_val = AiShaderEvalParamFlt(p_shiny);
            const float shiny = AiClamp((shiny_val * shiny_val), 2.0f, 100000.0f);
            sg->out.RGB += AiStretchedPhongIntegrate(&sg->Nf, sg, shiny) * Ks;
        }
    }

    const float KOcc = AiShaderEvalParamFlt(p_KOcc);
    if (KOcc > 0.0f) {
        const float minDist = AiShaderEvalParamFlt(p_occMinDist);
        const float maxDist = AiShaderEvalParamFlt(p_occMaxDist);
        const float spread = AiShaderEvalParamFlt(p_occSpread);
        const float falloff = AiShaderEvalParamFlt(p_occFalloff);

        const float radians = AiClamp(spread, 0.0f, 90.0f) * AI_DTOR;
        AtRGB occ = AiOcclusion(sg->Nf, sg->Ngf, sg, minDist, maxDist, radians, falloff, data->sampler, 0) * KOcc;

        // normal values of occlusion are flipped  so allow us to use either one..
        if (!AiShaderEvalParamBool(p_invertOcc)) {
            occ.r = 1.0f - occ.r;
            occ.g = 1.0f - occ.g;
            occ.b = 1.0f - occ.b;
        }

        AiColorClipToZero(occ);
        sg->out.RGB() *= occ;
    }

    const static AtString incandescencePP("incandescencePP");

    AtRGB emission_color = AI_RGB_WHITE;
    if (AiShaderEvalParamBool(p_emFromIncand)) {
        if (!AiUDataGetRGB(incandescencePP, emission_color))
            AiUDataGetVec(incandescencePP, reinterpret_cast<AtVector&>(emission_color));
    }
    emission_color *= AiShaderEvalParamRGB(p_emission);
    AiColorClipToZero(emission_color);
    sg->out.RGB() += emission_color;
    AiAOVSetRGB(sg, data->aov_emission, emission_color);
}
