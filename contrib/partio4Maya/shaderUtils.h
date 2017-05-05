#include <GL/glew.h>
#include <vector>
#include <memory>
#include <exception>
#include <sstream>

class GLProgram{
private:
    GLProgram(GLuint program, GLuint stage);
public:
    ~GLProgram();

    class CreateException{
    private:
        std::string m_error;
    public:
        CreateException(const char* error, const char* file, int line)
        {
            std::stringstream ss; ss << "[" << file << ":" << line << "] " << error;
            m_error = ss.str();
        }

        const char* what() const
        {
            return m_error.c_str();
        }
    };

    static std::shared_ptr<GLProgram> create_program(GLenum stage, GLsizei count, ...);

    void set_uniform(GLenum type, int location, int tuple, ...);
    void set_uniformv(GLenum type, int location, int tuple, int count, ...);
    void set_matrix(int location, int count, bool transpose, const float* matrices);
    int get_uniform_location(const char* name) const;

    GLuint get_stage() const;
    GLuint get_stage_bit() const;
    GLuint get_program() const;
private:
    GLuint m_program;
    GLuint m_stage;
};

class GLPipeline{
private:
    GLPipeline();
public:
    ~GLPipeline();

    class ValidateException{
    private:
        std::string m_error;
    public:
        ValidateException(const char* error, const char* file, int line)
        {
            std::stringstream ss; ss << "[" << file << ":" << line << "] " << error;
            m_error = ss.str();
        }

        const char* what() const
        {
            return m_error.c_str();
        }
    };

    static std::shared_ptr<GLPipeline> create_pipeline(int count, ...);

    GLuint get_filled_stages() const;
    std::shared_ptr<GLProgram> get_program(GLuint stage) const;

    class ScopedSet {
    public:
        ScopedSet(const GLPipeline& pipeline);
        ~ScopedSet();
    };
private:
    GLuint m_pipeline;
    std::vector<std::shared_ptr<GLProgram>> m_programs;

    void add_program(std::shared_ptr<GLProgram>& program);
    void validate();
};
