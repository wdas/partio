/////////////////////////////////////////////////////////////////////////////////////
/// PARTIO GENERATOR  an arnold procedural cache reader that uses the partio Library
/// by John Cassella (redpawfx)  for  Luma Pictures  (c) 2013

#include "partioGenerator.h"

#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

using namespace std;
using namespace Partio;

//
/*
inline bool lsHandleCudaErrorFunc(const char* fl, int ln)
{
 cudaError_t cet = cudaGetLastError();
 if(cet != cudaSuccess)
 {
  std::cout << "[" << fl << " : " << ln << "] : " << cudaGetErrorString(cet) << "\n";
  return true;
 }
 else return false;
}

#define lsHandleCudaError() lsHandleCudaErrorFunc(__FILE__, __LINE__)
*/



bool partioCacheExists ( const char* fileName )
{
    struct stat fileInfo;
    bool statReturn;
    int intStat;

    intStat = stat ( fileName, &fileInfo );
    if ( intStat == 0 )
    {
        statReturn = true;
    }
    else
    {
        statReturn = false;
    }

    return ( statReturn );

}

// read parameter values from node and load partio cache if it exists
static int MyInit ( AtNode *mynode, void **user_ptr )
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
    arg_stepSize			= 0.0f;
    global_motionBlurSteps 	= 1;
    global_fps 				= 24;
    global_motionByFrame 	= 0.5f;


    // check for values on node
    if ( AiNodeLookUpUserParameter ( mynode, "arg_file" ) != NULL )
    {
        arg_file 				= AiNodeGetStr ( mynode, "arg_file" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "overrideRadiusPP" ) != NULL )
    {
        arg_overrideRadiusPP    = AiNodeGetBool ( mynode, "overrideRadiusPP" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_radius" ) != NULL )
    {
        arg_radius 				= AiNodeGetFlt ( mynode, "arg_radius" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_maxParticleRadius" ) != NULL )
    {
        arg_maxParticleRadius  = AiNodeGetFlt ( mynode, "arg_maxParticleRadius" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_radiusMult" ) != NULL )
    {
        arg_radiusMult 			= AiNodeGetFlt ( mynode, "arg_radiusMult" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_renderType" ) != NULL )
    {
        arg_renderType 			= AiNodeGetInt ( mynode, "arg_renderType" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_motionBlurMult" ) != NULL )
    {
        arg_motionBlurMult 		= AiNodeGetFlt ( mynode, "arg_motionBlurMult" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_rgbFrom" ) != NULL )
    {
        arg_rgbFrom				= AiNodeGetStr ( mynode, "arg_rgbFrom" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_opacFrom" ) != NULL )
    {
        arg_opacFrom			= AiNodeGetStr ( mynode, "arg_opacFrom" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_radFrom" ) != NULL )
    {
        arg_radFrom			= AiNodeGetStr ( mynode, "arg_radFrom" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_defaultColor" ) != NULL )
    {
        arg_defaultColor		= AiNodeGetRGB ( mynode, "arg_defaultColor" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_defaultOpac" ) != NULL )
    {
        arg_defaultOpac			= AiNodeGetFlt ( mynode, "arg_defaultOpac" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "global_motionBlurSteps" ) != NULL )
    {
        global_motionBlurSteps 	= AiNodeGetInt ( mynode, "global_motionBlurSteps" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "global_fps" ) != NULL )
    {
        global_fps				= AiNodeGetFlt ( mynode, "global_fps" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "global_motionByFrame" ) != NULL )
    {
        global_motionByFrame	= AiNodeGetFlt ( mynode, "global_motionByFrame" );
    }
    if ( AiNodeLookUpUserParameter ( mynode, "arg_stepSize" ) != NULL )
    {
        arg_stepSize			= AiNodeGetFlt ( mynode, "arg_stepSize" );
    }

    AiMsgInfo ( "[partioGenerator] loading cache  %s", arg_file );

    if ( partioCacheExists ( arg_file ) )
    {
        cacheExists = true;
        points=read ( arg_file );
        if ( points )
        {
            pointCount = ( int ) points->numParticles();
            AiMsgInfo ( "[partioGenerator] loaded %d points", pointCount );
        }
        else
        {
            AiMsgInfo ( "[partioGenerator] skipping, no points" );
            return  FALSE;
        }
    }
    return TRUE;
}

// close  partio cache
static int MyCleanup ( void *user_ptr )
{
    if ( points )
    {
		AiMsgInfo ( "[partioGenerator] releasing points!" );
        points->release();
    }
    return TRUE;
}

static int MyNumNodes ( void *user_ptr )
{
    return  1;
}


// call function to copy values from cache read into AI-Arrays
static AtNode *MyGetNode ( void *user_ptr, int i )
{
    //cout << "Get node" << endl;
    AtArray *pointarr;
    AtArray *radarr;
    AtArray *rgbArr;
    AtArray *opacityArr;
    AtNode *currentInstance = AiNode ( "points" ); // initialize node
    pointarr 	= AiArrayAllocate ( pointCount,global_motionBlurSteps,AI_TYPE_POINT );
    //radarr 		= AiArrayAllocate(pointCount,global_motionBlurSteps,AI_TYPE_FLOAT);
    //radarr     = AiArrayAllocate ( pointCount,1,AI_TYPE_FLOAT );

    if ( cacheExists )
    {
        // we at least have to have  position, I mean come on throw us a bone here....
        if ( !points->attributeInfo ( "position",positionAttr ) && !points->attributeInfo ( "Position",positionAttr ) )
        {
            AiMsgInfo ( "[partioGenerator] Could not find position attr maybe you should do something about that" );
        }
        else
        {
            if ( points->attributeInfo ( "velocity",velocityAttr ) || points->attributeInfo ( "Velocity",velocityAttr ) )
            {
                AiMsgInfo ( "[partioGenerator] found velocity attr,  motion blur is a GO!!" );
                canMotionBlur = true;
            }
            /// RGB
            AiNodeDeclare ( currentInstance, "rgbPP", "uniform RGB" );
            if ( arg_rgbFrom !="" && points->attributeInfo ( arg_rgbFrom, rgbAttr ) )
            {
                AiMsgInfo ( "[partioGenerator] found rgbPP attr..." );
                hasRgbPP = true;
				rgbArr = AiArrayAllocate ( pointCount,1,AI_TYPE_RGB );
            }

            /// OPACITY
            AiNodeDeclare ( currentInstance, "opacityPP", "uniform Float" );
            if ( arg_opacFrom !="" && points->attributeInfo ( arg_opacFrom, opacityAttr ) )
            {
                AiMsgInfo ( "[partioGenerator] found opacityPP attr..." );
                hasOpacPP = true;
				opacityArr = AiArrayAllocate ( pointCount,1,AI_TYPE_FLOAT );
            }

            /// RADIUS by default  if "none" is defined it will look for  radiusPP or  radius
            if ( (arg_radFrom != "" && points->attributeInfo ( arg_radFrom,radiusAttr )) && !arg_overrideRadiusPP )
            {
                AiMsgInfo ( "[partioGenerator] found radius attr...%s", arg_radFrom );
                hasRadiusPP = true;
				radarr     = AiArrayAllocate ( pointCount,1,AI_TYPE_FLOAT );
            }
            else if ( (points->attributeInfo ( "radiusPP",radiusAttr ) || points->attributeInfo ( "radius",radiusAttr )) &&  !arg_overrideRadiusPP)
            {
                AiMsgInfo ( "[partioGenerator] found radius attr..." );
                hasRadiusPP = true;
				radarr     = AiArrayAllocate ( pointCount,1,AI_TYPE_FLOAT );
            }

            for ( int i = 0; i< pointCount; i++ )
            {
                bool badParticle =  false;
                const float * partioPositions = points->data<float> ( positionAttr,i );
                AtPoint point;
                if ( AiIsFinite ( partioPositions[0] ) && AiIsFinite ( partioPositions[1] ) && AiIsFinite ( partioPositions[2] ) )
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
                    AiMsgWarning ( "[partioGenerator] found INF or NAN particle, hiding it..." );
                }

                if ( canMotionBlur )
                {
                    const float * partioVelo = points->data<float> ( velocityAttr,i );
                    AtVector velocitySubstep;
                    AtVector velocity;
                    if ( badParticle )
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

                    // we need to use this to offset the position below to export the position on "Center" motion blur step
                    velocity = velocitySubstep;
                    if ( global_motionBlurSteps > 1 )
                    {
                        velocitySubstep /= global_motionBlurSteps-1;
                    }

                    velocitySubstep *= arg_motionBlurMult;

                    for ( int s = 0; s < global_motionBlurSteps; s++ )
                    {
                        AtPoint newPoint = ( point- ( velocity*.5 ) ) + velocitySubstep*s;
                        AiArraySetPnt ( pointarr, ( ( s*pointCount ) +i ), newPoint );
                    }
                }
                else
                {
                    for ( int s = 0; s < global_motionBlurSteps; s++ )
                    {
                        AiArraySetPnt ( pointarr, ( ( s*pointCount ) +i ), point );
                    }
                }

                /// RGBPP
                if ( hasRgbPP )
                {
                    const float * partioRGB = points->data<float> ( rgbAttr, i );
                    AtRGB color;
                    if ( rgbAttr.count > 1 )
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

                    // currently no support for motion blur of arbitrary attrs in points
                    //for (int s = 0; s < global_motionBlurSteps; s++)
                    //{
                    //	AiArraySetRGB(rgbArr,((s*pointCount)+i), color);
                    //}
                    AiArraySetRGB ( rgbArr,i , color );
                }

                /// opacityPP
                if ( hasOpacPP )
                {
                    const float * partioOpac = points->data<float> ( opacityAttr,i );
                    AtFloat opac;
                    if ( opacityAttr.count == 1 )
                    {
                        opac = partioOpac[0];
                    }
                    else
                    {
                        opac = float ( ( partioOpac[0]*0.2126 ) + ( partioOpac[1]*0.7152 ) + ( partioOpac[2]*.0722 ) );
                    }

                    //for (int s = 0; s < global_motionBlurSteps; s++)
                    //{
                    // hack to make bad particles which were set to 0,0,0  transparent
                    if ( badParticle )
                    {
                        //AiArraySetFlt(opacityArr ,((s*pointCount)+i), 0.0f);
                        AiArraySetFlt ( opacityArr ,i , 0.0f );
                    }
                    else
                    {
                        //AiArraySetFlt(opacityArr ,((s*pointCount)+i), opac);
                        AiArraySetFlt ( opacityArr ,i ,opac );
                    }
                    //}
                }

                /// User Defined or default  RadiusAttr
                if ( hasRadiusPP && !arg_overrideRadiusPP )
                {
                    const float * partioRadius = points->data<float> ( radiusAttr,i );
                    AtFloat rad;
                    if ( radiusAttr.count == 1 )
                    {
                        rad = partioRadius[0];
                    }
                    else
                    {
                        rad = abs ( float ( ( partioRadius[0]*0.2126 ) + ( partioRadius[1]*0.7152 ) + ( partioRadius[2]*.0722 ) ) );
                    }

                    rad *= arg_radiusMult;
                    rad *= arg_radius;
                    // clamp the radius to maxParticleRadius just in case we have rogue particles
                    if ( rad > arg_maxParticleRadius )
                    {
                        rad = arg_maxParticleRadius;
                    }

                    // if we decide to support motion blur radius scale then re-enable here
                    //for (int s = 0; s < global_motionBlurSteps; s++)
                    //{
                    //	AiArraySetFlt(radarr , ((s*pointCount)+i), rad);
                    //}
                    AiArraySetFlt ( radarr , i, rad );
                }

            } // for loop per particle

        } // position found

    } // cache exists


    if (!hasRgbPP)
	{
		cout << " setting  rgbPP " << endl;
		AiNodeSetRGB ( currentInstance, "rgbPP", arg_defaultColor.r, arg_defaultColor.g, arg_defaultColor.b );
	}
	else
	{
		cout << " settign rgbPP array" << endl;
		AiNodeSetArray ( currentInstance, "rgbPP", rgbArr );
	}

	if (!hasOpacPP)
	{
		cout << " setting  opacityPP " << endl;
		//AiNodeSetFlt ( currentInstance, "opacityPP", arg_defaultOpac );
	}
	else
	{
		cout << " setting  opacityPP array " << endl;
		//AiNodeSetArray ( currentInstance, "opacityPP", opacityArr );
	}

	if (!hasRadiusPP || arg_overrideRadiusPP)
	{
		cout << " setting  radius " << endl;
		//AiNodeSetFlt ( currentInstance, "radius", arg_radius*arg_radiusMult );
	}
	else
	{
		cout << " setting  radius array " << endl;
		//AiNodeSetArray ( currentInstance, "radius", radarr );
	}


    /// these  will always be here
	AiNodeSetArray ( currentInstance, "points", pointarr );
    //AiNodeSetArray ( currentInstance, "position",pointarr );  // we want to enable this when they fix some stuff on particles

	AiNodeSetInt ( currentInstance, "mode", arg_renderType );

    AiNodeSetBool ( currentInstance, "opaque", false );

    if ( arg_stepSize > 0.0f )
    {
        AiNodeSetFlt ( currentInstance, "step_size", arg_stepSize );
    }

    return currentInstance;
}

// vtable passed in by proc_loader macro define
proc_loader
{
    vtable->Init     = MyInit;
    vtable->Cleanup  = MyCleanup;
    vtable->NumNodes = MyNumNodes;
    vtable->GetNode  = MyGetNode;
    strcpy ( vtable->version, AI_VERSION );
    return TRUE;
}

