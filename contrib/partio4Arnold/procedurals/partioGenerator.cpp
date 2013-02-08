/////////////////////////////////////////////////////////////////////////////////////
/// PARTIO GENERATOR  an arnold procedural cache reader that uses the partio Library
/// by John Cassella (redpawfx)  for  Luma Pictures  (c) 2012

#include "partioGenerator.h"

#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

using namespace std;
using namespace Partio;


bool partioCacheExists(const char* fileName)
{
    struct stat fileInfo;
    bool statReturn;
    int intStat;

    intStat = stat(fileName, &fileInfo);
    if (intStat == 0)
    {
        statReturn = true;
    }
    else
    {
        statReturn = false;
    }

    return(statReturn);

}

// read parameter values from node and load partio cache if it exists
static int MyInit(AtNode *mynode, void **user_ptr)
{
    //cout << "INIT" << endl;
    *user_ptr = mynode; // make a copy of the parent procedural

    pointCount 	= 0;
    cacheExists = false;
    canMotionBlur = false;
    hasRgbPP = false;
    hasOpacPP = false;
    hasRadiusPP = false;

    // initialize the arg values
    arg_file 				= "";
    arg_radius 				= 0.1f;
    arg_maxParticleRadius  = 1000000.0f;
    arg_radiusMult 			= 1;
    arg_renderType 			= 0; // 0 = points, 1 = sphere, 2 = plane
    arg_motionBlurMult 		= 1;
    arg_overrideRadiusPP 	= false;
    arg_rgbFrom				= "";
    arg_opacFrom			= "";
	arg_radFrom             = "";
    arg_defaultColor 		= AI_RGB_WHITE;
    arg_defaultOpac			= 1.0f;
    global_motionBlurSteps 	= 1;
    global_fps 				= 24;
    global_motionByFrame 	= 0.5f;


    // check for values on node
    if (AiNodeLookUpUserParameter(mynode, "arg_file") != NULL)
    {
        arg_file 				= AiNodeGetStr(mynode, "arg_file");
    }
    if (AiNodeLookUpUserParameter(mynode, "overrideRadiusPP") != NULL)
    {
        arg_overrideRadiusPP    = AiNodeGetBool(mynode, "overrideRadiusPP");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_radius") != NULL)
    {
        arg_radius 				= AiNodeGetFlt(mynode, "arg_radius");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_maxParticleRadius") != NULL)
    {
       arg_maxParticleRadius  = AiNodeGetFlt(mynode, "arg_maxParticleRadius");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_radiusMult") != NULL)
    {
        arg_radiusMult 			= AiNodeGetFlt(mynode, "arg_radiusMult");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_renderType") != NULL)
    {
        arg_renderType 			= AiNodeGetInt(mynode, "arg_renderType");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_motionBlurMult") != NULL)
    {
        arg_motionBlurMult 		= AiNodeGetFlt(mynode, "arg_motionBlurMult");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_rgbFrom") != NULL)
    {
        arg_rgbFrom				= AiNodeGetStr(mynode, "arg_rgbFrom");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_opacFrom") != NULL)
    {
        arg_opacFrom			= AiNodeGetStr(mynode, "arg_opacFrom");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_radFrom") != NULL)
    {
        arg_radFrom			= AiNodeGetStr(mynode, "arg_radFrom");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_defaultColor") != NULL)
    {
        arg_defaultColor		= AiNodeGetRGB(mynode, "arg_defaultColor");
    }
    if (AiNodeLookUpUserParameter(mynode, "arg_defaultOpac") != NULL)
    {
        arg_defaultOpac			= AiNodeGetFlt(mynode, "arg_defaultOpac");
    }
    if (AiNodeLookUpUserParameter(mynode, "global_motionBlurSteps") != NULL)
    {
        global_motionBlurSteps 	= AiNodeGetInt(mynode, "global_motionBlurSteps");
    }
    if (AiNodeLookUpUserParameter(mynode, "global_fps") != NULL)
    {
        global_fps				= AiNodeGetFlt(mynode, "global_fps");
    }
    if (AiNodeLookUpUserParameter(mynode, "global_motionByFrame") != NULL)
    {
        global_motionByFrame	= AiNodeGetFlt(mynode, "global_motionByFrame");
    }

    AiMsgInfo("[luma.partioGenerator] loading cache  %s", arg_file);

    if (partioCacheExists(arg_file))
    {
        cacheExists = true;
        points=read(arg_file);
        if (points)
        {
            pointCount =(int)points->numParticles();
            AiMsgInfo("[luma.partioGenerator] loaded %d points", pointCount);
        }
        else
		{
			AiMsgInfo("[luma.partioGenerator] skipping, no points");
			return  FALSE;
		}
    }
    return TRUE;
}

// close  partio cache
static int MyCleanup(void *user_ptr)
{
    if (points)
    {
        points->release();
    }
    return TRUE;
}

static int MyNumNodes(void *user_ptr)
{
    return  1;
}


// call function to copy values from cache read into AI-Arrays
static AtNode *MyGetNode(void *user_ptr, int i)
{
    //cout << "Get node" << endl;
    AtArray *pointarr;
    AtArray *radarr;
    AtArray *rgbArr;
    AtArray *opacityArr;
    AtNode *currentInstance = AiNode("points"); // initialize node
    pointarr 	= AiArrayAllocate(pointCount,global_motionBlurSteps,AI_TYPE_POINT);
    radarr 		= AiArrayAllocate(pointCount,global_motionBlurSteps,AI_TYPE_FLOAT);

    if (cacheExists)
    {
        // we at least have to have  position, I mean come on throw us a bone here....
        if (!points->attributeInfo("position",positionAttr) && !points->attributeInfo("Position",positionAttr))
        {
            AiMsgInfo("[luma.partioGenerator] Could not find position attr maybe you should do something about that");
        }
        else
        {
            if (points->attributeInfo("velocity",velocityAttr) || points->attributeInfo("Velocity",velocityAttr))
            {
                AiMsgInfo("[luma.partioGenerator] found velocity attr,  motion blur is a GO!!");
                canMotionBlur = true;
            }
            /// RGB
            if (arg_rgbFrom !="" && points->attributeInfo(arg_rgbFrom, rgbAttr))
            {
                AiMsgInfo("[luma.partioGenerator] found rgbPP attr...");
                hasRgbPP = true;
            }
            AiNodeDeclare(currentInstance, "rgbPP", "uniform RGB");
            rgbArr = AiArrayAllocate(pointCount,global_motionBlurSteps,AI_TYPE_RGB);

            /// OPACITY
            if (arg_opacFrom !="" && points->attributeInfo(arg_opacFrom, opacityAttr))
            {
                AiMsgInfo("[luma.partioGenerator] found opacityPP attr...");
                hasOpacPP = true;
            }
            AiNodeDeclare(currentInstance, "opacityPP", "uniform Float");
            opacityArr = AiArrayAllocate(pointCount,global_motionBlurSteps,AI_TYPE_FLOAT);

			/// RADIUS by default  if "none" is defined it will look for  radiusPP or  radius
			if (arg_radFrom != "" && points->attributeInfo(arg_radFrom,radiusAttr))
            {
                AiMsgInfo("[luma.partioGenerator] found radius attr...%s", arg_radFrom);
                hasRadiusPP = true;
            }
            else if ( points->attributeInfo("radiusPP",radiusAttr) || points->attributeInfo("radius",radiusAttr))
			{
				AiMsgInfo("[luma.partioGenerator] found radius attr...");
                hasRadiusPP = true;
			}

            for (int i = 0; i< pointCount; i++)
            {
				bool badParticle =  false;
                const float * partioPositions = points->data<float>(positionAttr,i);
                AtPoint point;
				if (AiIsFinite(partioPositions[0]) && AiIsFinite(partioPositions[1]) && AiIsFinite(partioPositions[2]))
				{
					point.x = partioPositions[0];
					point.y = partioPositions[1];
					point.z = partioPositions[2];
				}
				else
				{
					point.x = 0.0f;
					point.y = 0.0f;
					point.z = 0.0f;
					badParticle = true;
					AiMsgWarning("[luma.partioGenerator] found INF or NAN particle, hiding it...");
				}

				if (canMotionBlur)
				{
					const float * partioVelo = points->data<float>(velocityAttr,i);
					AtVector velocitySubstep;
					if (badParticle)
					{
						velocitySubstep.x = 0.0f;
						velocitySubstep.y = 0.0f;
						velocitySubstep.z = 0.0f;
					}
					else
					{
						velocitySubstep.x = partioVelo[0];
						velocitySubstep.y = partioVelo[1];
						velocitySubstep.z = partioVelo[2];
					}
					velocitySubstep /= global_fps;
					velocitySubstep *=  global_motionByFrame; // motion by frame
					velocitySubstep /= global_motionBlurSteps;
					velocitySubstep *= arg_motionBlurMult;

					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						AtPoint newPoint = point + velocitySubstep*s;
						AiArraySetPnt(pointarr,((s*pointCount)+i),newPoint);
					}
				}
				else
				{
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						AiArraySetPnt(pointarr,((s*pointCount)+i),point);
					}
				}

				/// RGBPP
				if (hasRgbPP)
				{
					const float * partioRGB = points->data<float>(rgbAttr,i);
					AtRGB color;
					if (rgbAttr.count > 1)
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
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						AiArraySetRGB(rgbArr,((s*pointCount)+i), color);
					}
				}
				/// rgbPP set to none
				else
				{
					// get color from default node color
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						AiArraySetRGB(rgbArr,((s*pointCount)+i), arg_defaultColor);
					}
				}
				/// opacityPP
				if (hasOpacPP)
				{
					const float * partioOpac = points->data<float>(opacityAttr,i);
					AtFloat opac;
					if (opacityAttr.count == 1)
					{
						opac = partioOpac[0];
					}
					else
					{
						opac = float((partioOpac[0]*0.2126)+(partioOpac[1]*0.7152)+(partioOpac[2]*.0722));
					}
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						// hack to make bad particles which were set to 0,0,0  transparent
						if (badParticle)
						{
							AiArraySetFlt(opacityArr ,((s*pointCount)+i), 0.0f);
						}
						else
						{
							AiArraySetFlt(opacityArr ,((s*pointCount)+i), opac);
						}
					}
				}
				/// opacityPP set to none
				else
				{
					// get opac from node
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						if (badParticle)
						{
							AiArraySetFlt(opacityArr ,((s*pointCount)+i), 0.0f);
						}
						else
						{
							AiArraySetFlt(opacityArr, ((s*pointCount)+i), arg_defaultOpac);
						}
					}
				}
				/// User Defined or default  RadiusAttr
				if (hasRadiusPP && !arg_overrideRadiusPP)
				{
					const float * partioRadius = points->data<float>(radiusAttr,i);
					AtFloat rad;
					if (radiusAttr.count == 1)
					{
						rad = partioRadius[0];
					}
					else
					{
						rad = abs(float((partioRadius[0]*0.2126)+(partioRadius[1]*0.7152)+(partioRadius[2]*.0722)));
					}

					rad *= arg_radiusMult;
					rad *= arg_radius;
					// clamp the radius to maxParticleRadius just in case we have rogue particles
					if (rad > arg_maxParticleRadius)
					{
					rad = arg_maxParticleRadius;
					}
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						AiArraySetFlt(radarr , ((s*pointCount)+i), rad);
					}
				}
				else
				{
					for (int s = 0; s < global_motionBlurSteps; s++)
					{
						if (badParticle)
						{
							AiArraySetFlt(radarr, ((s*pointCount)+i), 0.0f);
						}
						else
						{
							AiArraySetFlt(radarr, ((s*pointCount)+i), arg_radius*arg_radiusMult);
						}
					}
				}
            }

            AiNodeSetArray(currentInstance, "rgbPP", rgbArr);
            AiNodeSetArray(currentInstance, "opacityPP", opacityArr);
        }
    }

    /// these 3  will always be here
    AiNodeSetArray(currentInstance, "points", pointarr);
	AiNodeSetArray(currentInstance, "position",pointarr);
    AiNodeSetArray(currentInstance, "radius", radarr);
    AiNodeSetInt(currentInstance, "mode", arg_renderType);

	AiNodeSetBool(currentInstance, "opaque", false);

    return currentInstance;
}

// vtable passed in by proc_loader macro define
proc_loader
{
    vtable->Init     = MyInit;
    vtable->Cleanup  = MyCleanup;
    vtable->NumNodes = MyNumNodes;
    vtable->GetNode  = MyGetNode;
    strcpy(vtable->version, AI_VERSION);
    return TRUE;
}

