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

#include <maya/MUserData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDrawContext.h>
#include <maya/MHWGeometryUtilities.h>

namespace {
    const char* vertex_shader_code = "#version 110\n" \
            "uniform mat4 world_view_proj;\n" \
            "void main(void) { gl_Position = world_view_proj * gl_Vertex; gl_FrontColor = gl_Color; gl_BackColor = gl_Color; }\n";
    const char* pixel_shader_code = "#version 110\n" \
            "void main(void) { gl_FragColor = gl_Color; }\n";

    GLuint INVALID_GL_OBJECT = static_cast<GLuint>(-1);
    GLuint vertex_shader = INVALID_GL_OBJECT;
    GLuint pixel_shader = INVALID_GL_OBJECT;
    GLuint shader_program = INVALID_GL_OBJECT;
    GLint world_view_proj_location = INVALID_GL_OBJECT;

    template <GLint shader_type>
    bool create_shader(GLuint& shader, const char* shader_code)
    {
        shader = glCreateShader(shader_type);

        GLint code_size = static_cast<GLint>(strlen(shader_code));
        glShaderSource(shader, 1, &shader_code, &code_size);
        glCompileShader(shader);
        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::cout << "[partioVisualizer] Error building shader : " << std::endl;
            std::cout << shader_code << std::endl;
            GLint max_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);
            std::vector<GLchar> info_log(max_length);
            glGetShaderInfoLog(shader, max_length, &max_length, &info_log[0]);
            std::cout << &info_log[0] << std::endl;
            glDeleteShader(shader); shader = INVALID_GL_OBJECT;
            return false;
        }
        else
            return true;
    }

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

        static void drawBillboardCircleAtPoint(const float* position, float radius, int drawType, BillboardDrawData& data)
        {
            glPushMatrix();
            glTranslatef(position[0], position[1], position[2]);
            glMatrixMode(GL_MODELVIEW_MATRIX);
            float m[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, m);
            m[0] = 1.0f;
            m[1] = 0.0f;
            m[2] = 0.0f;
            m[4] = 0.0f;
            m[5] = 1.0f;
            m[6] = 0.0f;
            m[8] = 0.0f;
            m[9] = 0.0f;
            m[10] = 1.0f;
            glLoadMatrixf(m);

            // TODO: setup radius using the scale of the Matrix
            // also get rid of all this gl matrix tingamagic, and do it manually

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

        void draw(bool as_bounding_box = false) const
        {
            if (p_reader_cache == 0 || p_reader_cache->particles == 0 || p_reader_cache->positionAttr.attributeIndex == -1)
                return;

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

                    glDisable(GL_POINT_SMOOTH);
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glEnableClientState(GL_COLOR_ARRAY);

                    glPointSize(m_point_size);
                    if (p_reader_cache->particles->numParticles() > 0)
                    {
                        const float* partioPositions = p_reader_cache->particles->data<float>(p_reader_cache->positionAttr, 0);

                        glVertexPointer(3, GL_FLOAT, stride_position, partioPositions);
                        glColorPointer(4, GL_FLOAT, stride_color, p_reader_cache->rgba.data());
                        glDrawArrays(GL_POINTS, 0, (p_reader_cache->particles->numParticles() / (m_draw_skip + 1)));
                    }

                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_COLOR_ARRAY); // even though we are pushing and popping
                    // attribs disabling the color array is required or else it will freak out VP1
                    // interestingly it's not needed for VP2...
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
                        drawBillboardCircleAtPoint(partioPositions, p_reader_cache->radius[i], m_draw_style, billboard_data);
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
        const DrawData* draw_data = reinterpret_cast<const DrawData*>(data);

        if (draw_data == 0 || shader_program == INVALID_GL_OBJECT)
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
        float world_view_proj[4][4];
        context.getMatrix(MHWRender::MDrawContext::kWorldViewProjMtx).get(world_view_proj);

        GLint current_program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

        glUseProgram(shader_program);

        glUniformMatrix4fv(world_view_proj_location, 1, false, &world_view_proj[0][0]);

        draw_data->draw(draw_bounding_box);

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

        glUseProgram(current_program);

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
        return kOpenGL;
    }

    void partioVisualizerDrawOverride::init_shaders()
    {

        if (!create_shader<GL_VERTEX_SHADER>(vertex_shader, vertex_shader_code))
            return;
        if (!create_shader<GL_FRAGMENT_SHADER>(pixel_shader, pixel_shader_code))
        {
            glDeleteShader(vertex_shader); vertex_shader = INVALID_GL_OBJECT;
            return;
        }

        shader_program = glCreateProgram();

        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, pixel_shader);
        glLinkProgram(shader_program);

        GLint success = 0;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::cout << "[partioVisualizer] Error linking shader." << std::endl;

            GLint max_length = 0;
            glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);
            std::vector<GLchar> info_log(max_length);
            glGetProgramInfoLog(shader_program, max_length, &max_length, &info_log[0]);

            std::cout << &info_log[0] << std::endl;

            glDeleteShader(vertex_shader); vertex_shader = INVALID_GL_OBJECT;
            glDeleteShader(pixel_shader); pixel_shader = INVALID_GL_OBJECT;
            glDeleteProgram(shader_program); shader_program = INVALID_GL_OBJECT;
            return;
        }

        glDetachShader(shader_program, vertex_shader);
        glDetachShader(shader_program, pixel_shader);

        world_view_proj_location = glGetUniformLocation(shader_program, "world_view_proj");
    }

    void partioVisualizerDrawOverride::free_shaders()
    {
        if (vertex_shader != INVALID_GL_OBJECT)
            glDeleteShader(vertex_shader);
        if (pixel_shader != INVALID_GL_OBJECT)
            glDeleteShader(pixel_shader);
        if (shader_program != INVALID_GL_OBJECT)
            glDeleteProgram(shader_program);
    }
}
