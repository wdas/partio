#pragma once

#include <maya/MPxDrawOverride.h>

#include "partioVisualizer.h"

namespace MHWRender{
    class partioVisualizerDrawOverride : public MPxDrawOverride {
    public:
        static MPxDrawOverride* creator(const MObject& obj);

        static MString registrantId;
    private:
        partioVisualizerDrawOverride(const MObject& obj);
        ~partioVisualizerDrawOverride();

        static void DrawCallback(const MDrawContext& context, const MUserData* data);

        // this is pure virtual in Maya 2015
        virtual MBoundingBox boundingBox(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const;

        virtual MUserData* prepareForDraw(
                const MDagPath& objPath,
                const MDagPath& cameraPath,
                const MFrameContext& frameContext,
                MUserData* oldData);

        const MObject m_object;
        partioVisualizer* p_visualizer;
        MBoundingBox m_bbox;
    };
}
