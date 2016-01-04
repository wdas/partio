#include "partioVisualizerDrawOverride.h"

#include <maya/MUserData.h>

namespace {
    struct DrawData : public MUserData {

        DrawData() : MUserData(false)
        {

        }
    };
}

namespace MHWRender {
    MString partioVisualizerDrawOverride::registrantId("partioVisualizerDrawOverride");

    MPxDrawOverride* partioVisualizerDrawOverride::creator(const MObject& obj)
    {
        return new partioVisualizerDrawOverride(obj);
    }

    partioVisualizerDrawOverride::partioVisualizerDrawOverride(const MObject& obj) : MPxDrawOverride(obj, DrawCallback)
    {
    }

    partioVisualizerDrawOverride::~partioVisualizerDrawOverride()
    {

    }

    void partioVisualizerDrawOverride::DrawCallback(const MDrawContext& context, const MUserData* data)
    {
        static int counter = 0;
        std::cout << "Draw callback called " << counter++ << std::endl;

    }

    MUserData* partioVisualizerDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath,
                                                            const MFrameContext& frameContext, MUserData* oldData)
    {
        DrawData* draw_data = oldData == 0 ? new DrawData() : reinterpret_cast<DrawData*>(oldData);
        return draw_data;
    }
}
