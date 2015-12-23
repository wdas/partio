#pragma once

#include <maya/MPxGeometryOverride.h>
#include <maya/MHWGeometry.h>
#include <maya/MShaderManager.h>

#include "partioVisualizer.h"

namespace MHWRender{
    class partioVisualizerGeometryOverride : public MHWRender::MPxGeometryOverride {
    public:
        static MHWRender::MPxGeometryOverride* creator(const MObject& object);

        partioVisualizerGeometryOverride(const MObject& object);

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

    private:
        MObject m_object;
        partioVizReaderCache* p_viz_reader_cache;
        MRenderer* p_renderer;
        const MShaderManager* p_shader_manager;
    };
}
