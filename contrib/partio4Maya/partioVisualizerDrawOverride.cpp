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
    private:
        MObject m_object;
        partioVizReaderCache* p_reader_cache;
        int m_draw_skip;
        int m_draw_style;
        float m_point_size;

    public:
        DrawData() : MUserData(false), p_reader_cache(0)
        {

        }

        // FIXME: mostly likely we don't need to update the m_object and p_reader_cache
        // because these are constant values, so move these to the constructor
        void update_data(const MObject& object, partioVizReaderCache* pv_cache)
        {
            clear();
            m_object = object;
            p_reader_cache = pv_cache;
            m_draw_skip = MPlug(m_object, partioVisualizer::aDrawSkip).asInt();
            m_draw_style = MPlug(m_object, partioVisualizer::aDrawStyle).asShort();
            m_point_size = MPlug(m_object, partioVisualizer::aPointSize).asFloat();
        }

        void clear()
        {

        }

        void draw() const
        {
            if (p_reader_cache == 0 || p_reader_cache->particles == 0 || p_reader_cache->positionAttr.attributeIndex == -1)
                return;

            glPushAttrib(GL_ALL_ATTRIB_BITS);

            // check for viewport draw mode
            if (m_draw_style == PARTIO_DRAW_STYLE_BOUNDING_BOX)
            {

            }
            else if (m_draw_style == PARTIO_DRAW_STYLE_POINTS)
            {
                const int stride_position = 3 * static_cast<int>(sizeof(float)) * m_draw_skip;
                const int stride_color = 4 * static_cast<int>(sizeof(float)) * m_draw_skip;

                glDisable(GL_POINT_SMOOTH);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);

                glPointSize(m_point_size);
                if (p_reader_cache->particles->numParticles() > 0)
                {
                    // now setup the position/color/alpha output pointers
                    const float* partioPositions = p_reader_cache->particles->data<float>(p_reader_cache->positionAttr, 0);

                    glVertexPointer(3, GL_FLOAT, stride_position, partioPositions);
                    glColorPointer(4, GL_FLOAT, stride_color, p_reader_cache->rgba.data());
                    glDrawArrays(GL_POINTS, 0, (p_reader_cache->particles->numParticles() / (m_draw_skip + 1)));
                }
            }
            else if (m_draw_style == PARTIO_DRAW_STYLE_RADIUS || m_draw_style == PARTIO_DRAW_STYLE_DISK)
            {

            }

            glPopAttrib();
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
        const DrawData* draw_data = reinterpret_cast<const DrawData*>(data);

        if (draw_data == 0)
            return;

        draw_data->draw();
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
