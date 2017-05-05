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
 * After some thinking, it's debatable if it's a good idea to use VBOs for displaying the points.
 * Usually we work with rather large point sets, and they could use up a lots of memory for the
 * point display. Around 28 bytes per particle, and it's fairly common to
 * have tens or hundreds of millions of particles.
 *
 * - Pal
 */

#include <GL/glew.h>

#include "partioVisualizerDrawOverride.h"
#include "shaderUtils.h"

#include <maya/MUserData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDrawContext.h>
#include <maya/MHWGeometryUtilities.h>

namespace {
    bool g_is_initialized = false;
    bool g_is_valid = false;

    //std::shared_ptr<GLPipeline> g_basic_particle_pipeline;

    GLuint INVALID_GL_OBJECT = static_cast<GLuint>(-1);

    std::shared_ptr<GLProgram> g_lit_particle_vertex;
    std::shared_ptr<GLPipeline> g_lit_particle_pipeline;

    GLint g_world_view_location = INVALID_GL_OBJECT;
    GLint g_proj_location = INVALID_GL_OBJECT;

    // we always want to init glew here, or else it might fail in rare cases
    // when done in the plugin init
    bool init_glew()
    {
        if (g_is_initialized)
            return g_is_valid;
        g_is_initialized = true;

        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();

        if (renderer == 0 || !renderer->drawAPIIsOpenGL())
        {
            MGlobal::displayError("[partio] DirectX for VP2 is not supported!");
            return false;
        }

        GLenum err = glewInit();
        if (err != GLEW_OK)
        {
            MGlobal::displayError(MString("[partio] Error initializing glew : ") + MString((const char*)glewGetErrorString(err)));
            return false;
        }

        try{

            g_lit_particle_vertex = GLProgram::create_program(GL_VERTEX_SHADER, 1,R"glsl(
#version 110
uniform mat4 world_view;
uniform mat4 proj;
void main(void) {
    gl_Position =  proj * world_view * gl_Vertex;
    float cosTheta = 1.0;
    if (gl_Normal.xyz != vec3(0.0, 0.0, 0.0))
        cosTheta = abs( dot(normalize(gl_NormalMatrix * gl_Normal.xyz), vec3(0.0, 0.0, -1.0)));
    gl_FrontColor = gl_Color * cosTheta;
    gl_BackColor = gl_Color * cosTheta;
}
            )glsl");

            auto lit_particle_fragment = GLProgram::create_program(GL_FRAGMENT_SHADER, 1, R"glsl(
#version 110

void main(void)
{
    gl_FragColor = gl_Color;
}
)glsl");

            g_lit_particle_pipeline = GLPipeline::create_pipeline(2, &g_lit_particle_vertex, &lit_particle_fragment);

            g_world_view_location = g_lit_particle_vertex->get_uniform_location("world_view");
            g_proj_location = g_lit_particle_vertex->get_uniform_location("proj");
        }
        catch(GLProgram::CreateException& ex)
        {
            std::cerr << "[ai_fractal] Error creating shader" << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }
        catch(GLPipeline::ValidateException& ex)
        {
            std::cerr << "[ai_fractal] Error validating pipeline" << std::endl;
            std::cerr << ex.what() << std::endl;
            return false;
        }

        g_is_valid = true;
        return true;
    }

    const char* vertex_shader_code =
            "#version 110\n"\
            "uniform mat4 world_view;\n" \
            "uniform mat4 proj;\n" \
            "void main(void) {\n" \
                    "gl_Position =  proj * world_view * gl_Vertex;\n" \
                    "float cosTheta = 1.0;\n" \
                    "if (gl_Normal.xyz != vec3(0.0, 0.0, 0.0)) cosTheta = abs( dot(normalize(gl_NormalMatrix * gl_Normal.xyz), vec3(0.0, 0.0, -1.0)));\n" \
                    "gl_FrontColor = gl_Color * cosTheta;\n" \
                    "gl_BackColor = gl_Color * cosTheta;\n" \
            " }\n";
    const char* pixel_shader_code = "#version 110\n" \
            "void main(void) { gl_FragColor = gl_Color; }\n";

    // TODO: make partioVisualizer use the code from here
    struct DrawData : public MUserData {
    private:
        MObject m_object;
        partioVizReaderCache* p_reader_cache;
        int m_draw_skip;
        int m_draw_style;
        float m_point_size;
        float m_default_alpha;
        float m_icon_size;

        struct BillboardDrawData {
            std::vector<float> vertices;
            const size_t num_segments;
            float last_radius;

            BillboardDrawData(size_t _num_segments) : num_segments(_num_segments), last_radius(-1.0f)
            {
                vertices.resize(num_segments * 2);
            }
        };

        static void drawBillboardCircleAtPoint(const float* position, float radius, int drawType, BillboardDrawData& data, const MMatrix& world_view_matrix)
        {
            static MMatrix world_view_source = MMatrix::identity;
            world_view_source[3][0] = position[0];
            world_view_source[3][1] = position[1];
            world_view_source[3][2] = position[2];
            static __thread float world_view[4][4];
            (world_view_source * world_view_matrix).get(world_view);
            world_view[0][0] = 1.0f;
            world_view[0][1] = 0.0f;
            world_view[0][2] = 0.0f;
            world_view[1][0] = 0.0f;
            world_view[1][1] = 1.0f;
            world_view[1][2] = 0.0f;
            world_view[2][0] = 0.0f;
            world_view[2][1] = 0.0f;
            world_view[2][2] = 1.0f;

            MPoint world_view_pos = world_view_matrix * MPoint(position[0], position[1], position[2], 1.0);

            g_lit_particle_vertex->set_matrix(g_world_view_location, 1, false, &world_view[0][0]);

            // TODO: setup radius using the scale of the Matrix
            if (radius != data.last_radius)
            {
                data.last_radius = radius;

                const float theta = 2.0f * 3.1415926f / float(data.num_segments);
                const float tangetial_factor = tanf(theta);//calculate the tangential factor
                const float radial_factor = cosf(theta);//calculate the radial factor

                float x = radius;//we start at angle = 0
                float y = 0;

                for (size_t i = 0, vid = 0; i < data.num_segments; ++i)
                {
                    data.vertices[vid++] = x;
                    data.vertices[vid++] = y;

                    const float tx = -y;
                    const float ty = x;

                    //add the tangential vector
                    x += tx * tangetial_factor;
                    y += ty * tangetial_factor;

                    //correct using the radial factor
                    x *= radial_factor;
                    y *= radial_factor;
                }
            }

            if (drawType == PARTIO_DRAW_STYLE_RADIUS)
                glDrawArrays(GL_LINE_LOOP, 0, data.num_segments);
            else
                glDrawArrays(GL_POLYGON, 0, data.num_segments);

            glPopMatrix();
        }

        void draw_bounding_box() const
        {
            const MPoint bboxMin = p_reader_cache->bbox.min();
            const MPoint bboxMax = p_reader_cache->bbox.max();

            const float xMin = static_cast<float>(bboxMin.x);
            const float yMin = static_cast<float>(bboxMin.y);
            const float zMin = static_cast<float>(bboxMin.z);
            const float xMax = static_cast<float>(bboxMax.x);
            const float yMax = static_cast<float>(bboxMax.y);
            const float zMax = static_cast<float>(bboxMax.z);

            glColor4fv(m_wireframe_color);
            /// draw the bounding box
            glBegin(GL_LINES);

            glVertex3f(xMin, yMin, zMax);
            glVertex3f(xMax, yMin, zMax);

            glVertex3f(xMin, yMin, zMin);
            glVertex3f(xMax, yMin, zMin);

            glVertex3f(xMin, yMin, zMax);
            glVertex3f(xMin, yMin, zMin);

            glVertex3f(xMax, yMin, zMax);
            glVertex3f(xMax, yMin, zMin);

            glVertex3f(xMin, yMax, zMin);
            glVertex3f(xMin, yMax, zMax);

            glVertex3f(xMax, yMax, zMax);
            glVertex3f(xMax, yMax, zMin);

            glVertex3f(xMin, yMax, zMax);
            glVertex3f(xMax, yMax, zMax);

            glVertex3f(xMin, yMax, zMin);
            glVertex3f(xMax, yMax, zMin);

            glVertex3f(xMin, yMax, zMin);
            glVertex3f(xMin, yMin, zMin);

            glVertex3f(xMax, yMax, zMin);
            glVertex3f(xMax, yMin, zMin);

            glVertex3f(xMin, yMax, zMax);
            glVertex3f(xMin, yMin, zMax);

            glVertex3f(xMax, yMax, zMax);
            glVertex3f(xMax, yMin, zMax);

            glEnd();
        }

        void clear()
        {
            // this is empty so far, could be extended later on
        }
    public:
        // attributes below are configured differently for VP1 and VP2
        MBoundingBox m_cache_bbox;
        MBoundingBox m_logo_bbox;
        float m_wireframe_color[4];
        int m_draw_error;

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
            m_default_alpha = MPlug(m_object, partioVisualizer::aDefaultAlpha).asFloat();
            m_icon_size = MPlug(m_object, partioVisualizer::aSize).asFloat();

            if (p_reader_cache)
                m_cache_bbox = p_reader_cache->bbox;

            static const MBoundingBox logo_bbox = partio4Maya::partioLogoBoundingBox();
            m_logo_bbox.clear();
            m_logo_bbox.expand(logo_bbox.min() * m_icon_size);
            m_logo_bbox.expand(logo_bbox.max() * m_icon_size);
        }

        void draw(bool as_bounding_box, const MMatrix& world_view_matrix) const
        {
            if (p_reader_cache == 0 || p_reader_cache->particles == 0 || p_reader_cache->positionAttr.attributeIndex == -1)
                return;

            // default normal to 0,0,0
            glNormal3f(0.0, 0.0, 0.0);

            // check for viewport draw mode
            if (m_draw_style == PARTIO_DRAW_STYLE_BOUNDING_BOX || as_bounding_box)
                draw_bounding_box();
            else
            {
                if (p_reader_cache->opacityAttr.attributeIndex != -1 || m_default_alpha < 1.0f)
                {
                    glDepthMask(true);
                    glEnable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                    glEnable(GL_POINT_SMOOTH);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }

                if (m_draw_style == PARTIO_DRAW_STYLE_POINTS)
                {
                    const int stride_position = 3 * static_cast<int>(sizeof(float)) * m_draw_skip;
                    const int stride_color = 4 * static_cast<int>(sizeof(float)) * m_draw_skip;
                    const int stride_normal = 3 * static_cast<int>(sizeof(float)) * m_draw_skip;

                    glDisable(GL_POINT_SMOOTH);
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glEnableClientState(GL_COLOR_ARRAY);

                    bool useNormals = false;

                    glPointSize(m_point_size);
                    if (p_reader_cache->particles->numParticles() > 0)
                    {
                        if (!p_reader_cache->normal.empty())
                        {
                            useNormals = true;
                            glEnableClientState(GL_NORMAL_ARRAY);
                        }

                        const float* partioPositions = p_reader_cache->particles->data<float>(p_reader_cache->positionAttr, 0);

                        glVertexPointer(3, GL_FLOAT, stride_position, partioPositions);
                        glColorPointer(4, GL_FLOAT, stride_color, p_reader_cache->rgba.data());
                        if (useNormals)
                        {
                            glNormalPointer(GL_FLOAT, stride_normal, p_reader_cache->normal.data());
                        }
                        glDrawArrays(GL_POINTS, 0, (p_reader_cache->particles->numParticles() / (m_draw_skip + 1))-3);
                    }

                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_COLOR_ARRAY); // even though we are pushing and popping
                    // attribs disabling the color array is required or else it will freak out VP1
                    // interestingly it's not needed for VP2...

                    if (useNormals)
                    {
                        glDisableClientState(GL_NORMAL_ARRAY);
                    }
                }
                else if (m_draw_style == PARTIO_DRAW_STYLE_RADIUS || m_draw_style == PARTIO_DRAW_STYLE_DISK)
                {
                    // if this is accessed from multiple threads
                    // we already screwed because of OpenGL
                    static BillboardDrawData billboard_data(10);

                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(2, GL_FLOAT, 0, &billboard_data.vertices[0]);

                    for (int i = 0; i < p_reader_cache->particles->numParticles(); i += (m_draw_skip + 1))
                    {
                        glColor4fv(&p_reader_cache->rgba[i * 4]);
                        const float* partioPositions = p_reader_cache->particles->data<float>(p_reader_cache->positionAttr, i);
                        drawBillboardCircleAtPoint(partioPositions, p_reader_cache->radius[i], m_draw_style, billboard_data, world_view_matrix);
                    }

                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_COLOR_ARRAY);
                }
            }

            return;
        }

        void draw_icon() const
        {
            partio4Maya::drawPartioLogo(m_icon_size);
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
            p_visualizer = dynamic_cast<partioVisualizer*>(dnode.userNode());
    }

    partioVisualizerDrawOverride::~partioVisualizerDrawOverride()
    {

    }

    void partioVisualizerDrawOverride::DrawCallback(const MDrawContext& context, const MUserData* data)
    {
        if (!init_glew())
            return;

        const DrawData* draw_data = reinterpret_cast<const DrawData*>(data);

        if (draw_data == 0)
            return;

        const bool draw_bounding_box = context.getDisplayStyle() & MHWRender::MFrameContext::kBoundingBox;

        bool draw_cache = true;
        bool draw_logo = true;
        MStatus status;
        MBoundingBox frustrum_box = context.getFrustumBox(&status);
        if (status)
        {
            if (!frustrum_box.intersects(draw_data->m_cache_bbox))
                draw_cache = false;
            if (draw_bounding_box || !frustrum_box.intersects(draw_data->m_logo_bbox))
                draw_logo = false;
        }

        if (!(draw_logo || draw_cache))
            return;

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        float world_view[4][4];
        MMatrix world_view_matrix = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx);
        world_view_matrix.get(world_view);
        float proj[4][4];
        context.getMatrix(MHWRender::MDrawContext::kProjectionMtx).get(proj);

        {
            GLPipeline::ScopedSet scoped_pipeline(*g_lit_particle_pipeline);

            g_lit_particle_vertex->set_matrix(g_world_view_location, 1, false, &world_view[0][0]);
            g_lit_particle_vertex->set_matrix(g_proj_location, 1, false, &proj[0][0]);

            draw_data->draw(draw_bounding_box, world_view_matrix);

            if (draw_logo)
            {
                if (draw_data->m_draw_error == 0)
                    glColor3fv(draw_data->m_wireframe_color);
                if (draw_data->m_draw_error == 1)
                    glColor3f(.75f, 0.0f, 0.0f);
                else if (draw_data->m_draw_error == 2)
                    glColor3f(0.0f, 0.0f, 0.0f);

                draw_data->draw_icon();
            }
        }

        glPopClientAttrib();
        glPopAttrib();
    }

    MBoundingBox partioVisualizerDrawOverride::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const
    {
        return MBoundingBox();
    }

    bool partioVisualizerDrawOverride::isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const
    {
        // the cache and the bounding box can change at any time
        // plus calling the prepareForDraw also updates the data, so we always have to draw
        // then manually do the frustum culling in the draw function
        return false;
    }

    MUserData* partioVisualizerDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath,
                                                            const MFrameContext& frameContext, MUserData* oldData)
    {
        DrawData* draw_data = oldData == 0 ? new DrawData() : reinterpret_cast<DrawData*>(oldData);
        if (p_visualizer != 0)
        {
            // TODO: debate if the the bbox queries should be moved to the bounding box function
            partioVizReaderCache* p_cache = p_visualizer->updateParticleCache();
            draw_data->update_data(m_object, p_cache);
            MColor color = MHWRender::MGeometryUtilities::wireframeColor(objPath);
            draw_data->m_wireframe_color[0] = color.r;
            draw_data->m_wireframe_color[1] = color.g;
            draw_data->m_wireframe_color[2] = color.b;
            draw_data->m_wireframe_color[3] = color.a;
            draw_data->m_draw_error = p_visualizer->drawError;
        }
        return draw_data;
    }

    DrawAPI partioVisualizerDrawOverride::supportedDrawAPIs() const
    {
#if MAYA_API_VERSION >= 201600
        return kOpenGL | kOpenGLCoreProfile;
#else
        return kOpenGL;
#endif
    }
}
