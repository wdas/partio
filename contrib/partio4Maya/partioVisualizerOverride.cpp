#include "partioVisualizerOverride.h"

MString partioVisualizerOverride::registrantId("partioVisualizerOverride");

MHWRender::MPxGeometryOverride* partioVisualizerOverride::creator(const MObject& object)
{
    return new partioVisualizerOverride(object);
}

partioVisualizerOverride::partioVisualizerOverride(const MObject& object) : MHWRender::MPxGeometryOverride(object)
{

}

void partioVisualizerOverride::updateDG()
{

}

void partioVisualizerOverride::updateRenderItems(const MDagPath& path,
                                                 MHWRender::MRenderItemList& list)
{

}

void partioVisualizerOverride::populateGeometry(const MHWRender::MGeometryRequirements& requirements,
                                                const MHWRender::MRenderItemList& renderItems,
                                                MHWRender::MGeometry& data)
{

}

void partioVisualizerOverride::cleanUp()
{

}
