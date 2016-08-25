#include "shaderUtils.h"

#include <iostream>
#include <cstring>
#include <vector>

#include <stdarg.h>

#define THROW_PROGRAM_CREATE(mess) throw GLProgram::CreateException(mess, __FILE__, __LINE__)
#define THROW_PIPELINE_VALIDATE(mess) throw GLPipeline::ValidateException(mess, __FILE__, __LINE__)

namespace {
    const GLuint INVALID_GL_OBJECT = static_cast<GLuint>(-1);

}

GLProgram::GLProgram(GLuint program, GLuint stage) : m_program(program), m_stage(stage)
{

}
GLProgram::~GLProgram()
{
}

std::shared_ptr<GLProgram> GLProgram::create_program(GLenum stage, GLsizei count, ...)
{
    class ScopedShader {
    private:
        GLuint shader;
    public:
        ScopedShader(GLenum stage) : shader(glCreateShader(stage))
        { }

        ~ScopedShader() { glDeleteShader(shader); }

        inline operator GLuint() { return shader; }
    };

    GLuint program = INVALID_GL_OBJECT;
    std::vector<const char*> sources;
    sources.reserve(count);
    va_list va;
    va_start(va, count);
    for (GLsizei i = 0; i < count; ++i)
        sources.push_back(va_arg(va, const char*));
    va_end(va);
    // we don't use the builtin function to be able to print proper errors
    // per creation step
    ScopedShader shader(stage);
    if (shader)
    {
        glShaderSource(shader, count, sources.data(), 0);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_TRUE)
        {
            program = glCreateProgram();
            if (program)
            {
                glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
                glAttachShader(program, shader);
                glLinkProgram(program);
                glDetachShader(program, shader);
            }
            else
                THROW_PROGRAM_CREATE("Error calling glCreatProgram!");
        }
        else
        {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            if (log_length > 0)
            {
                std::vector<GLchar> log(log_length, '\0');
                glGetShaderInfoLog(shader, log_length, 0, log.data());
                THROW_PROGRAM_CREATE(log.data());
            }
            else
                THROW_PROGRAM_CREATE("Unkown error while compiling the shader!");
        }
    }
    else
        THROW_PROGRAM_CREATE("Error calling glCreateShader!");

    return std::shared_ptr<GLProgram>(new GLProgram(program, stage));
}

void GLProgram::set_uniform(GLenum type, int location, int tuple, ...)
{
    va_list va;
    va_start(va, tuple);

    if (type == GL_FLOAT)
    {
        float* args = reinterpret_cast<float*>(alloca(sizeof(float) * tuple));
        for (int i = 0; i < tuple; ++i)
            args[i] = static_cast<float>(va_arg(va, double));
        if (tuple == 1)
            glProgramUniform1f(m_program, location, args[0]);
        else if (tuple == 2)
            glProgramUniform2f(m_program, location, args[0], args[1]);
        else if (tuple == 3)
            glProgramUniform3f(m_program, location, args[0], args[1], args[2]);
        else if (tuple == 4)
            glProgramUniform4f(m_program, location, args[0], args[1], args[2], args[3]);
    }
    else if (type == GL_INT)
    {
        int* args = reinterpret_cast<int*>(alloca(sizeof(int) * tuple));
        for (int i = 0; i < tuple; ++i)
            args[i] = va_arg(va, int);
        if (tuple == 1)
            glProgramUniform1i(m_program, location, args[0]);
        else if (tuple == 2)
            glProgramUniform2i(m_program, location, args[0], args[1]);
        else if (tuple == 3)
            glProgramUniform3i(m_program, location, args[0], args[1], args[2]);
        else if (tuple == 4)
            glProgramUniform4i(m_program, location, args[0], args[1], args[2], args[3]);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        unsigned int* args = reinterpret_cast<unsigned int*>(alloca(sizeof(unsigned int) * tuple));
        for (int i = 0; i < tuple; ++i)
            args[i] = va_arg(va, unsigned int);
        if (tuple == 1)
            glProgramUniform1ui(m_program, location, args[0]);
        else if (tuple == 2)
            glProgramUniform2ui(m_program, location, args[0], args[1]);
        else if (tuple == 3)
            glProgramUniform3ui(m_program, location, args[0], args[1], args[2]);
        else if (tuple == 4)
            glProgramUniform4ui(m_program, location, args[0], args[1], args[2], args[3]);
    }

    va_end(va);
}

void GLProgram::set_uniformv(GLenum type, int location, int tuple, int count, ...)
{
    va_list va;
    va_start(va, count);

    const int element_count = tuple * count;

    if (type == GL_FLOAT)
    {
        float* args = reinterpret_cast<float*>(alloca(sizeof(float) * element_count));
        for (int i = 0; i < element_count; ++i)
            args[i] = static_cast<float>(va_arg(va, double));
        if (tuple == 1)
            glProgramUniform1fv(m_program, location, count, args);
        else if (tuple == 2)
            glProgramUniform2fv(m_program, location, count, args);
        else if (tuple == 3)
            glProgramUniform3fv(m_program, location, count, args);
        else if (tuple == 4)
            glProgramUniform4fv(m_program, location, count, args);
    }
    else if (type == GL_INT)
    {
        int* args = reinterpret_cast<int*>(alloca(sizeof(int) * element_count));
        for (int i = 0; i < element_count; ++i)
            args[i] = va_arg(va, int);
        if (tuple == 1)
            glProgramUniform1iv(m_program, location, count, args);
        else if (tuple == 2)
            glProgramUniform2iv(m_program, location, count, args);
        else if (tuple == 3)
            glProgramUniform3iv(m_program, location, count, args);
        else if (tuple == 4)
            glProgramUniform4iv(m_program, location, count, args);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        unsigned int* args = reinterpret_cast<unsigned int*>(alloca(sizeof(unsigned int) * element_count));
        for (int i = 0; i < element_count; ++i)
            args[i] = va_arg(va, unsigned int);
        if (tuple == 1)
            glProgramUniform1uiv(m_program, location, count, args);
        else if (tuple == 2)
            glProgramUniform2uiv(m_program, location, count, args);
        else if (tuple == 3)
            glProgramUniform3uiv(m_program, location, count, args);
        else if (tuple == 4)
            glProgramUniform4uiv(m_program, location, count, args);
    }

    va_end(va);
}

void GLProgram::set_matrix(int location, int count, bool transpose, const float* matrices)
{
    glProgramUniformMatrix4fv(m_program, location, count, transpose, matrices);
}

int GLProgram::get_uniform_location(const char* name) const
{
    return glGetUniformLocation(m_program, name);
}

GLPipeline::GLPipeline()
{
}

GLPipeline::~GLPipeline()
{
}

std::shared_ptr<GLPipeline> GLPipeline::create_pipeline(int count, ...)
{
    GLPipeline* pipeline = new GLPipeline();

    va_list va;
    va_start(va, count);

    for (int i = 0; i < count; ++i)
        pipeline->add_program(*va_arg(va, std::shared_ptr<GLProgram>*));

    va_end(va);

    pipeline->validate();

    return std::shared_ptr<GLPipeline>(pipeline);
}

void GLPipeline::add_program(std::shared_ptr<GLProgram>& program)
{
    m_programs.push_back(program);
}

void GLPipeline::validate()
{
    glGenProgramPipelines(1, &m_pipeline);

    for (const auto& program : m_programs)
        glUseProgramStages(m_pipeline, program->get_stage_bit(), program->get_program());

    glValidateProgramPipeline(m_pipeline);
    GLint status = GL_FALSE;
    glGetProgramPipelineiv(m_pipeline, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint log_length = 0;
        glGetProgramPipelineiv(m_pipeline, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0)
        {
            std::vector<GLchar> log(log_length, '\0');
            glGetProgramPipelineInfoLog(m_pipeline, log_length, 0, log.data());
            THROW_PIPELINE_VALIDATE(log.data());
        }
        THROW_PIPELINE_VALIDATE("Unknown error when validating the pipeline!");
    }
}

GLuint GLPipeline::get_filled_stages() const
{
    GLuint stages = 0;
    for (const auto& program : m_programs)
        stages = stages | program->get_stage_bit();
    return stages;
}

std::shared_ptr<GLProgram> GLPipeline::get_program(GLuint stage) const
{
    for (const auto& program : m_programs)
    {
        if (program->get_stage() == stage)
            return program;
    }
    return m_programs.front();
}

GLPipeline::ScopedSet::ScopedSet(const GLPipeline& pipeline)
{
    glBindProgramPipeline(pipeline.m_pipeline);
}

GLPipeline::ScopedSet::~ScopedSet()
{
    glBindProgramPipeline(0);
}

GLuint GLProgram::get_stage() const
{
    return m_stage;
}

GLuint GLProgram::get_stage_bit() const
{
    switch (m_stage)
    {
        case GL_VERTEX_SHADER:
            return GL_VERTEX_SHADER_BIT;
        case GL_FRAGMENT_SHADER:
            return GL_FRAGMENT_SHADER_BIT;
        case GL_GEOMETRY_SHADER:
            return GL_GEOMETRY_SHADER_BIT;
        case GL_TESS_CONTROL_SHADER:
            return GL_TESS_CONTROL_SHADER_BIT;
        case GL_TESS_EVALUATION_SHADER:
            return GL_TESS_EVALUATION_SHADER_BIT;
        case GL_COMPUTE_SHADER:
            return GL_COMPUTE_SHADER_BIT;
        default:
            return 0;
    }
}

GLuint GLProgram::get_program() const
{
    return m_program;
}
