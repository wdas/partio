/*
 * The idea here is to have an initial implementation, then incrementally start adding new functions.
 * The initial one should use the same functions as in partioVisualizer (though cleaned up).
 * Later on we can start using VBOs, and avoid re-uploading the data every frame. This approach
 * is supported on all the configuration (if you don't have VBOs, you can't run VP2), and should
 * be the fallback option for older GPUs and OSX.
 * After the fallback option, we can start playing around with some proper instancing for the billboard display,
 * OpenGL has a number of instancing pipelines, most likely the easiest approach
 * (draw instanced, and transforming the vertices using the gl_instanceId and a mapped buffer of position and colors)
 * will be enough to be efficient enough, and it will still require a fairly old OpenGL (around 3.1). GLEW can be used
 * to check the GL version and find the function pointers.
 * MultiDrawIndirect is most likely faster, but it might not worth the extra effort to implement.
 *
 * - Pal
 */

#include "partioVisualizerDrawOverride.h"

#include <maya/MUserData.h>
#include <maya/MFnDependencyNode.h>

namespace {
    struct DrawData : public MUserData {
        partioVizReaderCache* p_reader_cache;
        MObject m_object;
        DrawData() : MUserData(false)
        {

        }

        void update_data(const MObject& object, partioVizReaderCache* pv_cache)
        {
            clear();
            m_object = object;
            p_reader_cache = pv_cache;
        }

        void clear()
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

    partioVisualizerDrawOverride::partioVisualizerDrawOverride(const MObject& obj) : MPxDrawOverride(obj, DrawCallback), m_object(obj)
    {
        MStatus status;
        MFnDependencyNode dnode(m_object, &status);
        if (status)
        {
            p_visualizer = dynamic_cast<partioVisualizer*>(dnode.userNode());
            if (p_visualizer)
                m_bbox = p_visualizer->updateParticleCache()->bbox;
        }
    }

    partioVisualizerDrawOverride::~partioVisualizerDrawOverride()
    {

    }

    void partioVisualizerDrawOverride::DrawCallback(const MDrawContext& context, const MUserData* data)
    {

    }

    MBoundingBox partioVisualizerDrawOverride::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const
    {
        return m_bbox;
    }

    MUserData* partioVisualizerDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath,
                                                            const MFrameContext& frameContext, MUserData* oldData)
    {
        DrawData* draw_data = oldData == 0 ? new DrawData() : reinterpret_cast<DrawData*>(oldData);
        if (p_visualizer != 0)
        {
            partioVizReaderCache* p_cache = p_visualizer->updateParticleCache();
            m_bbox = p_cache->bbox;
            draw_data->update_data(m_object, p_cache);
        }
        return draw_data;
    }
}
