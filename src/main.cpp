#include "glad/glad.h"
#include <GLFW/glfw3.h>


#include <iostream>
#include <stdexcept>
#include <memory>

#include <glm/glm.hpp>

template<typename T> using Scoped = std::unique_ptr<T>;
template<typename T> using Ref = std::shared_ptr<T>;

#include <vector>

// -------------------------------------------------------------------------------------------
struct Vertex
{
    glm::vec2 position;
};

void APIENTRY glCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    std::cout << "OpenGL debug message: " << message << std::endl;
}

// -------------------------------------------------------------------------------------------
class Buffer
{
private:
    GLuint id{ 0 };
    GLenum kind{ 0 };
    GLenum usage{ 0 };

public:
    template<typename T>
    Buffer(GLenum kind, GLenum usage, const std::vector<T>& data)
        :   kind(kind),  usage(usage)
    {
        glCreateBuffers(1, &id);
        if (id == 0)
        {
            throw std::runtime_error("Failed to create buffer");
        }

        glNamedBufferData(id, data.size() * sizeof T, data.data(), usage);
    }

    ~Buffer()
    {
        if (id)
        {
            glDeleteBuffers(1, &id);
        }
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
        :   kind(other.kind),
            usage(other.usage)
    {
        if (id)
        {
            glDeleteBuffers(1, &id);
        }
        id = other.id;
        other.id = 0;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (id)
        {
            glDeleteBuffers(1, &id);
        }
        id = other.id;
        other.id = 0;
        return *this;
    }

    GLuint getId() const
    {
        return id;
    }

    GLenum getType() const
    {
        return kind;
    }
};


// -------------------------------------------------------------------------------------------
class VertexArray
{
private:
    GLuint id{ 0 };

public:
    struct LayoutElem
    {
        GLuint index{ 0 };
        GLenum count{ 0 };
        GLenum format{ 0 };
        bool normalized{ false };
        size_t stride{ 0 };
        size_t offset{ 0 };
    };

    VertexArray()
    {
        glCreateVertexArrays(1, &id);
    }

    ~VertexArray()
    {
        if (id)
        {
            glDeleteVertexArrays(1, &id);
        }
    }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept
    {
        if (id)
        {
            glDeleteVertexArrays(1, &id);
        }
        id = other.id;
        other.id = 0;
    }

    VertexArray& operator=(VertexArray&& other) noexcept
    {
        if (id)
        {
            glDeleteVertexArrays(1, &id);
        }
        id = other.id;
        other.id = id;
        return *this;
    }

    void bindLayout(const std::vector<LayoutElem> elems)
    {
        for (const auto& elem : elems)
        {
            glEnableVertexArrayAttrib(id, elem.index);

            glVertexArrayAttribFormat(
                id,
                elem.index,
                elem.count,
                elem.format,
                elem.normalized,
                static_cast<GLuint>(elem.offset)
            );

            glVertexArrayAttribBinding(id, elem.index, 0);
        }
    }

    void bindVertexBuffer(const Buffer* pBuffer)
    {
        glVertexArrayVertexBuffer(id, 0, pBuffer->getId(), 0, sizeof Vertex);
    }

    GLuint getId() const
    {
        return id;
    }
};


// -------------------------------------------------------------------------------------------
class Application
{
private:
    GLFWwindow* window{ nullptr };

    Ref<VertexArray> vao;
    Ref<Buffer> vbo;

public:
    Application(int width, int height, const char* title)
    {
        initWindow(width, height, title);
        initContext();
        createResources();
    }

    int run()
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        while (!glfwWindowShouldClose(window))
        {
            auto now = glfwGetTime();
            glfwPollEvents();
            glClear(GL_COLOR_BUFFER_BIT);
            render();
            glfwSwapBuffers(window);
            update(glfwGetTime() - now);
        }

        glfwTerminate();
        return 0;
    }

private:
    void update(double deltaTime)
    {

    }

    void render()
    {
        glBindVertexArray(vao->getId());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    void initWindow(int width, int height, const char* title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window)
        {
            throw std::runtime_error("Failed to create a window");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
    }

    void initContext()
    {
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

#ifndef NDEBUG
        glDebugMessageCallback(glCallback, nullptr);
#endif
    }

    // Load assets
    void createResources()
    {

        vao = std::make_shared<VertexArray>();

        // Create quad
        vbo = std::make_shared<Buffer>(GL_ARRAY_BUFFER, GL_STATIC_DRAW, std::vector{
            Vertex {{0.0f, 0.5f}},
            Vertex {{-0.5f, -0.5f}},
            Vertex {{0.5f, -0.5f}},
        });

        std::vector<VertexArray::LayoutElem> layout = {
            {0, 2, GL_FLOAT, false, sizeof(Vertex), 0}
        };

        vao->bindVertexBuffer(vbo.get());
        vao->bindLayout(layout);
    }

};


// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------
int main()
{
    try
    {
        auto app = std::make_unique<Application>(1280, 720, "Arcanoid");
        return app->run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
}