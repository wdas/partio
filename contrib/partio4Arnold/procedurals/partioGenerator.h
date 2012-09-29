/////////////////////////////////////////////////////////////////////////////////////
/// PARTIO GENERATOR  an arnold procedural cache reader that uses the partio Library
/// by John Cassella (redpawfx)  for  Luma Pictures  (c) 2012


#include "ai.h"
#include <Partio.h>
#include <PartioAttribute.h>
#include <PartioIterator.h>

#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>


// global variables for procedural
float arg_radius;
float arg_maxParticleRadius;
float arg_radiusMult;
const char* arg_file;
int arg_renderType;
bool arg_overrideRadiusPP;
const char* arg_rgbFrom;
const char* arg_opacFrom;
const char* arg_radFrom;
AtRGB arg_defaultColor;
AtFloat arg_defaultOpac;

float arg_motionBlurMult;
float global_motionByFrame;
float global_fps;
int global_motionBlurSteps;
Partio::ParticlesDataMutable* points;
Partio::ParticleAttribute positionAttr;
Partio::ParticleAttribute velocityAttr;
Partio::ParticleAttribute rgbAttr;
Partio::ParticleAttribute opacityAttr;
Partio::ParticleAttribute radiusAttr;
int  pointCount;
bool cacheExists;
bool canMotionBlur, hasRadiusPP, hasRgbPP,hasOpacPP;
