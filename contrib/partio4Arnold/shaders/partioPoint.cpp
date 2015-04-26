#include <ai.h>


AI_SHADER_NODE_EXPORT_METHODS(partioPointMtd);


enum partioPointParams
{
    p_preMult,
    p_overrideRgbPP,
    p_particleColor,
    p_overrideOpacityPP,
    p_particleOpacity,
    p_blackHole,
    p_MatteOpacity,
    p_doShadowOpacity,
    p_useLighting,
    //p_Ka,
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
    p_emFromIncand,
    p_emission,
    p_aov_direct_diffuse,
    p_aov_indirect_diffuse,
    p_aov_emission,


};


node_parameters
{
	AiParameterBOOL("premult"				, false);
	AiParameterBOOL("overrideRgbPP"		, false);
	AiParameterRGB("particleColor"		, 1.0f, 1.0f, 1.0f);
	AiParameterBOOL("overrideOpacityPP"	, false);
	AiParameterRGB("particleOpacity"		, 1.0f, 1.0f, 1.0f);
	AiParameterBOOL("blackHole"			,false);
	AiParameterRGB("matteOpacity"		, 1.0f, 1.0f, 1.0f);
	AiParameterBOOL("doShadowOpacity"     ,false);
	AiParameterBOOL("useLighting"			,true);
//    AiParameterFLT	( "Ka"	              	, 0.1f );  // AiAmbient has been removed  from arnold 4.1.+
	AiParameterFLT("Kd"                	, 0.8f);
	AiParameterFLT("Ki"					, 1.0f);
	AiParameterFLT("Ks"					, 1.0f);
	AiParameterFLT("shiny"				, 10.0f);
	AiParameterFLT("KOcc"				, 1.0f);
	AiParameterINT("occSamples"          , 5);
	AiParameterFLT("occSpread"			, 90);
	AiParameterFLT("occFalloff"			, 0);
	AiParameterFLT("occMinDist"			, 0);
	AiParameterFLT("occMaxDist"			, 2);
	AiParameterBool("invertOcc"			,false);
	AiParameterBool("emColorFromIncan"	,false);
	AiParameterRGB("emission"      		, 0.0f,0.0f,0.0f);
	AiParameterSTR("aov_direct_diffuse"  ,"direct_diffuse");
	AiParameterSTR("aov_indirect_diffuse" ,"indirect_diffuse");
	AiParameterSTR("aov_emission"        ,"emission");


	AiMetaDataSetStr(mds, NULL, "maya.classification", "shader/surface");
	AiMetaDataSetStr(mds, NULL, "maya.name", "partioPoint");
	//AiMetaDataSetInt(mds, NULL, "maya.id", NULL);
}

enum AutoTransparency
{
    ALWAYS,
    SHADOW_ONLY,
    NEVER
};

typedef struct
{
	AtSampler *sampler;
}
ShaderData;

node_initialize
{
	ShaderData *data = (ShaderData*) AiMalloc(sizeof(ShaderData));
	AiNodeSetLocalData(node, data);
}

///////////////////////////////////////
/// UPDATE
node_update
{
	ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
	data->sampler = NULL;
	data->sampler = AiSampler(AiNodeGetInt(node, "occSamples"), 2);
}


//////////////////////////////////////
/// FINISH
node_finish
{
	ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
	AiSamplerDestroy(data->sampler);
	//Freeing ShaderData
	AiFree(data);
}

shader_evaluate
{
	ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

	AtRGB 		Oi = AI_RGB_BLACK;
	AtRGB  		Ci = AI_RGB_BLACK;
	AtRGB		MatOpac = AI_RGB_BLACK;
	bool   blackHole = false;

	AtRGB 		diffuse, indirDiff, ambient, occ, spec;
	AtRGB		rgbPP;
	AtRGB		incand;
	AtVector    incandVec;
	float 		opacityPP;
	float		minDist, maxDist, spread, falloff;
	AtRGB 		weighted_sample;
	float		brdf;
	AtVector 	V;

	minDist		=AiShaderEvalParamFlt(p_occMinDist);
	maxDist 	=AiShaderEvalParamFlt(p_occMaxDist);
	spread 		=AiShaderEvalParamFlt(p_occSpread);
	falloff		=AiShaderEvalParamFlt(p_occFalloff);
	MatOpac 	=AiShaderEvalParamRGB(p_MatteOpacity);
	blackHole 	=AiShaderEvalParamBool(p_blackHole);

	if(!AiShaderEvalParamBool(p_doShadowOpacity))
	{
		// early out shadow rays
		if(sg->Rt & AI_RAY_SHADOW)
			return;
	}

	AtColor emission_color = AiShaderEvalParamRGB(p_emission);
	if(AiUDataGetVec("incandescencePP", &incandVec) && AiShaderEvalParamBool(p_emFromIncand))
	{
		AtColor temp;
		temp.r = incandVec.x;
		temp.g = incandVec.y;
		temp.b = incandVec.z;
		emission_color = temp * emission_color; // used in this case as a multiplier
	}
	else if(AiUDataGetRGB("incandescencePP", &incand) && AiShaderEvalParamBool(p_emFromIncand))
	{
		emission_color = incand * emission_color;
	}

	AiColorClipToZero(emission_color);

	if(AiColorToGrey(emission_color) != 0.0f)
	{
		AiAOVSetRGB(sg, AiShaderEvalParamStr(p_aov_emission), emission_color);
	}

	if(AiUDataGetRGB("rgbPP", &rgbPP) &&!AiShaderEvalParamBool(p_overrideRgbPP))
	{
	}
	else
	{
		rgbPP = AiShaderEvalParamRGB(p_particleColor);
	}
	Ci += rgbPP;

	if(AiUDataGetFlt("opacityPP", &opacityPP) && !AiShaderEvalParamBool(p_overrideOpacityPP))
	{
		Oi.r = opacityPP;
		Oi.g = opacityPP;
		Oi.b = opacityPP;
	}
	else
	{
		Oi = AiShaderEvalParamRGB(p_particleOpacity);
	}

	if(AiShaderEvalParamBool(p_useLighting))
	{
		AiColorReset(spec);
		//ambient = AiAmbient; // AiAmbient removed from arnold  4.1+

		diffuse = AiDirectDiffuse(&sg->Nf, sg);
		indirDiff = AiIndirectDiffuse(&sg->Nf, sg);

		AiAOVSetRGB(sg, AiShaderEvalParamStr(p_aov_direct_diffuse), diffuse);
		AiAOVSetRGB(sg, AiShaderEvalParamStr(p_aov_indirect_diffuse), indirDiff);

		if(AiShaderEvalParamFlt(p_Ks) > 0)
		{
			float shinyVal = AiShaderEvalParamFlt(p_shiny);
			float shiny = CLAMP((shinyVal*shinyVal), 2.0f , 100000.0f);
			AiV3Neg(V, sg->Rd);
			brdf = AiStretchedPhongBRDF(&sg->Ld, &V, &sg->Nf,shiny);
			AiColorScale(weighted_sample, sg->Li, sg->we * brdf);
			AiColorAdd(spec, spec, weighted_sample);
			spec *= AiShaderEvalParamFlt(p_Ks);
		}

		AtRGB lighting;

		//AiColorScale(ambient, ambient,AiShaderEvalParamFlt(p_Ka));
		AiColorScale(diffuse, diffuse,AiShaderEvalParamFlt(p_Kd));
		AiColorScale(indirDiff, indirDiff, AiShaderEvalParamFlt(p_Ki));

		//AiColorAdd(diffuse, diffuse, indirDiff);
		//AiColorAdd(lighting, diffuse,ambient);
		AiColorAdd(lighting, diffuse, indirDiff);

		AiColorMult(Ci,Ci,lighting);
		if(AiShaderEvalParamFlt(p_Ks))
		{
			AiColorAdd(Ci, Ci, spec);
		}
	}

	if(AiShaderEvalParamFlt(p_KOcc) > 0)
	{
		float  radians = CLAMP(spread, 0.0f,90.0f) * (float)AI_DTOR;
		AtVector NBent;
		occ = AiOcclusion(&sg->Nf,&sg->Ngf, sg, minDist,maxDist,radians,falloff,data->sampler,&NBent);
		occ *= AiShaderEvalParamFlt(p_KOcc);

		if(!AiShaderEvalParamBool(p_invertOcc))   // normal values of occlusion are flipped  so allow us to use either one..
		{
			occ.r = 1-occ.r;
			occ.g = 1-occ.g;
			occ.b = 1-occ.b;
		}
		AiColorClipToZero(occ);
		if(!AiColorCorrupted(occ))
		{
			AiColorMult(Ci, Ci, occ);
		}

	}

	AiColorAdd(Ci, Ci, emission_color);

	if(!blackHole)
	{
		AiColorClipToZero(Oi);
		Oi *= AiColorToGrey(MatOpac);
		sg->out_opacity = Oi;
	}
	else
	{
		Oi *= AI_RGB_BLACK;
		Ci *= AI_RGB_BLACK;
	}

	if(AiShaderEvalParamFlt(p_preMult))
	{
		Ci *= AiColorToGrey(Oi);
	}

///////////////////////////
/// FINAL OUTPUT!

	sg->out.RGBA.r = Ci.r;
	sg->out.RGBA.g = Ci.g;
	sg->out.RGBA.b = Ci.b;
	sg->out.RGBA.a = AiColorToGrey(Oi);

}

