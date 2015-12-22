#pragma once

#include <maya/MPxGeometryOverride.h>
#include <maya/MHWGeometry.h>

class partioVisualizerOverride : public MHWRender::MPxGeometryOverride {
public:
    static MHWRender::MPxGeometryOverride* creator(const MObject& object);

    partioVisualizerOverride(const MObject& object);

    virtual void updateDG();
    virtual void updateRenderItems(
            const MDagPath& path,
            MHWRender::MRenderItemList& list);
    virtual void populateGeometry(
            const MHWRender::MGeometryRequirements& requirements,
            const MHWRender::MRenderItemList& renderItems,
            MHWRender::MGeometry& data);
    virtual void cleanUp();

    static MString registrantId;
};
