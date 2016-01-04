#pragma once

#include <maya/MPxDrawOverride.h>

namespace MHWRender{
    class partioVisualizerDrawOverride : public MPxDrawOverride {
    public:
        static MPxDrawOverride* creator(const MObject& obj);

        static MString registrantId;
    private:
        partioVisualizerDrawOverride(const MObject& obj);
        ~partioVisualizerDrawOverride();

        static void DrawCallback(const MDrawContext& context, const MUserData* data);

        virtual MUserData* prepareForDraw(
                const MDagPath& objPath,
                const MDagPath& cameraPath,
                const MFrameContext& frameContext,
                MUserData* oldData);
    };
}
