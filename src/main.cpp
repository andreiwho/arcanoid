#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#endif

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <AL/al.h>
#include <AL/alc.h>

#include "audio.h"


#include <iostream>
#include <stdexcept>
#include <memory>
#include <string_view>

#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using Vec2 = glm::vec2;
using Mat4 = glm::mat4;

template<typename T> using Scoped = std::unique_ptr<T>;


#include <vector>
#include <fstream>

using namespace std::string_literals;

// -------------------------------------------------------------------------------------------
class IRefCounted
{
private:
    size_t refCount{ 1 };

public:
    virtual ~IRefCounted() = default;

    void addRef()
    {
        refCount++;
    }
    
    void release()
    {
        refCount--;
        if (refCount == 0)
        {
            delete this;
        }
    }

    size_t getRefCount() const
    {
        return refCount;
    }
};

template<typename T>
concept RefCounted = std::is_base_of_v<IRefCounted, T>;

template<RefCounted T>
class RefCnt
{
private:
    T* object{};

    RefCnt(T* object) 
        :   object(object)
    {}
public:
    template<typename ... Args>
    static RefCnt<T> make(Args&& ... args)
    {
        return RefCnt<T>(new T(std::forward<Args>(args)...));
    }

    ~RefCnt()
    {
        if (object)
        {
            object->release();
        }
    }

    RefCnt() = default;

    RefCnt(const RefCnt<T>& other)
        : object(other.object)
    {
        object->addRef();
    }

    RefCnt& operator=(const RefCnt<T>& other)
    {

        if (object)
        {
            object->release();
        }
        object = other.object;
        object->addRef();
        return *this;
    }

    RefCnt(RefCnt<T>&& other) noexcept
        :   object(other.object)
    {
        other.object = nullptr;
    }

    RefCnt& operator=(RefCnt<T>&& other) noexcept
    {
        if (object)
        {
            object->release();
        }
        object = other.object;
        other.object = nullptr;
        return *this;
    }

    T* operator->()
    {
        return object;
    }

    T& operator*()
    {
        return *object;
    }

    T* get()
    {
        return object;
    }
};

template<typename T> using Ref = RefCnt<T>;

// -------------------------------------------------------------------------------------------
class AudioEntry : public IRefCounted
{
private:

public:
    AudioEntry(std::string_view file)
    {
        ALubyte numChannels;
        ALint sampleRate;
        ALubyte bitsPerSample;
        ALsizei size;
        wavLoadFile(file, &numChannels, &sampleRate, &bitsPerSample, &size);
    }
};

// -------------------------------------------------------------------------------------------
class Shader : public IRefCounted
{
private:
    GLuint id{ 0 };

    GLint projectionMatrix{ 0 };
    GLint modelMatrix{ 0 };

public:
    Shader(std::string_view vertFile, std::string_view fragFile)
    {
        id = glCreateProgram();
        GLuint vs = compileShader(GL_VERTEX_SHADER, vertFile);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragFile);

        glAttachShader(id, vs);
        glAttachShader(id, fs);

        glLinkProgram(id);
        glValidateProgram(id);

        glDetachShader(id, fs);
        glDetachShader(id, vs);

        glDeleteShader(fs);
        glDeleteShader(vs);

        projectionMatrix = glGetUniformLocation(id, "projectionMatrix");
        modelMatrix = glGetUniformLocation(id, "modelMatrix");
    }

    ~Shader()
    {
        if (id)
        {
            glDeleteProgram(id);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
    {
        if (id)
        {
            glDeleteProgram(id);
        }

        id = other.id;
        other.id = 0;
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (id)
        {
            glDeleteProgram(id);
        }

        id = other.id;
        other.id = 0;
        return *this;
    }

    GLuint compileShader(GLenum type, std::string_view file)
    {
        std::ifstream f(file.data(), std::ios::ate | std::ios::badbit);
        size_t size = f.tellg();
        f.seekg(0);
        std::vector<char> src(size);
        f.read(src.data(), size);
        f.close();

        GLuint shaderId = glCreateShader(type);
        const char* srcCstr = src.data();
        glShaderSource(shaderId, 1, &srcCstr, nullptr);

        int result = 0;
        glCompileShader(shaderId);
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            int shaderLogLength = 0;
            glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &shaderLogLength);
            std::string log;
            log.resize(shaderLogLength);
            glGetShaderInfoLog(shaderId, shaderLogLength, &shaderLogLength, log.data());

            throw std::runtime_error("Failed to compile shader: "s + log);
        }

        return shaderId;
    }

    GLuint getId() const
    {
        return id;
    }

    void setProjectionMatrix(const Mat4& mat)
    {
        setUniform(projectionMatrix, mat);
    }

    void setModelMatrix(const Mat4& mat)
    {
        setUniform(modelMatrix, mat);
    }

private:
    void setUniform(GLint location, const Mat4& mat)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
    }
};

// -------------------------------------------------------------------------------------------
struct Vertex
{
    Vertex(Vec2 position)
        : position(position)
    {}

    Vec2 position;
};

void APIENTRY glCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    std::cout << "OpenGL debug message: " << message << std::endl;
}

// -------------------------------------------------------------------------------------------
class Buffer : public IRefCounted
{
private:
    GLuint id{ 0 };
    GLenum kind{ 0 };
    GLenum usage{ 0 };

public:
    template<typename T>
    Buffer(GLenum kind, GLenum usage, const std::vector<T>& data)
        : kind(kind), usage(usage)
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
        : kind(other.kind),
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
class VertexArray : public IRefCounted
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

    void bindIndexBuffer(const Buffer* pBuffer)
    {
        glVertexArrayElementBuffer(id, pBuffer->getId());
    }

    GLuint getId() const
    {
        return id;
    }
};


// -------------------------------------------------------------------------------------------
class Quad : public IRefCounted
{
    Ref<VertexArray> vao;
    Ref<Buffer> vbo;
    Ref<Buffer> ibo;
    Ref<Shader> shader;

    Vec2 position{ 0.0f, -0.5f };
    Vec2 size{ 1.0f, 0.2f };

public:
    Quad(Vec2 position, Vec2 size, std::string_view vs, std::string_view fs)
        : position(position), size(size)
    {
        std::vector<Vertex> vertices =
        {
            Vertex({position.x - size.x / 2, position.y + size.y / 2}),
            Vertex({position.x - size.x / 2, position.y - size.y / 2}),
            Vertex({position.x + size.x / 2, position.y - size.y / 2}),
            Vertex({position.x + size.x / 2, position.y + size.y / 2}),
        };

        std::vector<GLuint> indices =
        {
            0,1,3,3,1,2
        };

        vbo = Ref<Buffer>::make(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices);
        ibo = Ref<Buffer>::make(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices);

        vao = Ref<VertexArray>::make();

        std::vector<VertexArray::LayoutElem> layout = {
            {0, 2, GL_FLOAT, false, sizeof(Vertex), 0}
        };

        vao->bindVertexBuffer(vbo.get());
        vao->bindIndexBuffer(ibo.get());
        vao->bindLayout(layout);

        shader = Ref<Shader>::make(vs, fs);
    }

    void draw(const Mat4& projection)
    {
        Mat4 modelMatrix = glm::identity<Mat4>();
        modelMatrix = glm::translate(modelMatrix, glm::vec3(position, 1.0f));

        glBindVertexArray(vao->getId());
        glUseProgram(shader->getId());
        shader->setProjectionMatrix(projection);
        shader->setModelMatrix(modelMatrix);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    Vec2 getSize() const
    {
        return size;
    }

    Vec2 getPosition() const
    {
        return position;
    }

    void moveX(float amount)
    {
        float clamp = 2.0f - (size.x / 2.0f);

        position.x += amount;
        position.x = glm::clamp(position.x, -clamp, clamp);
    }

    void moveY(float amount)
    {
        float clamp = 1.5f - (size.y / 2.0f);

        position.y += amount;
        position.y = glm::clamp(position.y, -clamp, clamp);
    }
};

class PlayerPlatform : public IRefCounted
{
private:
    Quad quad;

public:
    PlayerPlatform(Vec2 position, Vec2 size)
        : quad(position, size, "shaders/basic.vert", "shaders/basic.frag")
    {
    }

    void draw(const Mat4& projection)
    {
        quad.draw(projection);
    }

    void move(float direction, float speed)
    {
        quad.moveX(direction * speed);
    }

    Vec2 getPosition() const
    {
        return quad.getPosition();
    }

    Vec2 getSize() const
    {
        return quad.getSize();
    }
};

// -------------------------------------------------------------------------------------------
class BoxGrid : public IRefCounted
{
public:
    struct Box
    {
        Vertex topLeft;
        Vertex bottomLeft;
        Vertex bottomRight;
        Vertex topRight;

        bool hit(Vec2 position, Vec2 size) const
        {
            constexpr float selfBias = 0.1f;

            if (position.x - selfBias < topRight.position.x
                && position.x + selfBias > topLeft.position.x
                && position.y - selfBias < topLeft.position.y
                && position.y + selfBias > bottomRight.position.y)
            {
                return true;
            }
            return false;
        }
    };

private:
    Ref<VertexArray> vao;
    Ref<Buffer>  vbo;
    Ref<Buffer> ibo;

    Ref<Shader> shader;
    std::vector<Box> vertices;

    Vec2 position;
    Vec2 boxSize;
    float margin;
    int countX;
    int countY;

    std::vector<size_t> skippedBoxes;

    size_t indexCount = 0;

public:
    BoxGrid(Vec2 position, Vec2 boxSize = Vec2(0.5f, 0.5f), float margin = 0.01f, int countX = 10, int countY = 3)
        :   position(position), boxSize(boxSize), margin(margin), countX(countX), countY(countY)
    {
        regenerate();
    }

    void regenerate()
    {
        vertices = generateVertices(position, boxSize, margin, countX, countY);
        auto indices = generateIndices(vertices.size());

        vbo = Ref<Buffer>::make(GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertices);
        ibo = Ref<Buffer>::make(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, indices);

        vao = Ref<VertexArray>::make();

        std::vector<VertexArray::LayoutElem> layout = {
            {0, 2, GL_FLOAT, false, sizeof(Vertex), 0}
        };

        vao->bindVertexBuffer(vbo.get());
        vao->bindIndexBuffer(ibo.get());
        vao->bindLayout(layout);

        shader = Ref<Shader>::make("shaders/box.vert", "shaders/box.frag");

        indexCount = indices.size();
    }

    std::vector<Box> generateVertices(Vec2 position, Vec2 boxSize, float margin, int countX, int countY)
    {
        float lastX = position.x;
        float lastY = position.y;

        std::vector<Box> vertices;
        vertices.reserve(static_cast<size_t>(countX) * countY);

        size_t boxIndex = 0;
        for (int y = 0; y < countY; y++)
        {
            for (int x = 0; x < countX; x++)
            {
                for (auto idx : skippedBoxes)
                {
                    if (boxIndex == idx)
                    {
                        vertices.emplace_back(Box{ Vec2{-100.0f, -100.0f}, Vec2{-100.0f, -100.0f}, Vec2{-100.0f, -100.0f}, Vec2{-100.0f, -100.0f} });
                        goto skip_box;
                    }
                }

                vertices.emplace_back(Box{
                   .topLeft = Vec2{ lastX - boxSize.x / 2, lastY + boxSize.y / 2 },
                   .bottomLeft = Vec2{ lastX - boxSize.x / 2, lastY - boxSize.y / 2 },
                   .bottomRight = Vec2{ lastX + boxSize.x / 2, lastY - boxSize.y / 2 },
                   .topRight = Vec2{ lastX + boxSize.x / 2, lastY + boxSize.y / 2 } });
                
                skip_box:
                lastX += margin + boxSize.x;
                boxIndex++;
            }
            lastY -= margin + boxSize.y;
            lastX = position.x;
        }

        return vertices;
    }

    std::vector<GLuint> generateIndices(size_t vertexCount)
    {
        std::vector<GLuint> indices;
        indices.reserve(vertexCount * 6);

        GLuint lastIdx = 0;
        for (size_t i = 0; i < vertexCount; i++)
        {
            indices.emplace_back(lastIdx);
            indices.emplace_back(lastIdx + 1);
            indices.emplace_back(lastIdx + 3);
            indices.emplace_back(lastIdx + 3);
            indices.emplace_back(lastIdx + 1);
            indices.emplace_back(lastIdx + 2);
            lastIdx += 4;
        }

        return indices;
    }

    void draw(const Mat4& projection)
    {
        glBindVertexArray(vao->getId());
        glUseProgram(shader->getId());
        shader->setProjectionMatrix(projection);
        shader->setModelMatrix(glm::identity<glm::mat4>());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    }

    const std::vector<Box>& getBoxes() const
    {
        return vertices;
    }

    void destroyBox(size_t idx)
    {
        skippedBoxes.push_back(idx);
        regenerate();
    }
};


class Ball : public IRefCounted
{
private:
    Quad quad;

    Ref<PlayerPlatform> player;
    Ref<BoxGrid> grid;

public:
    Ball(Vec2 position, Vec2 size, const Ref<PlayerPlatform>& player, const Ref<BoxGrid>& grid)
        : quad(position, size, "shaders/ball.vert", "shaders/ball.frag"), player(player), grid(grid)
    {}

    void draw(const Mat4& projection)
    {
        quad.draw(projection);
    }

    void move(Vec2 direction, float speed)
    {
        quad.moveX(direction.x * speed);
        quad.moveY(direction.y * speed);
    }

    Vec2 getPosition() const
    {
        return quad.getPosition();
    }

    Vec2 getSize() const
    {
        return quad.getSize();
    }

    // Test code before implementing collision detection
    float xStep = 1.0f;
    float yStep = -1.0f;
    void bounce(float speed)
    {
        float yBouncePoint = 1.499f - (quad.getSize().y / 2.0f);
        float xBouncePoint = 1.999f - (quad.getSize().x / 2.0f);

        quad.moveX(xStep * speed);
        quad.moveY(yStep * speed);

        if (quad.getPosition().x > xBouncePoint)
        {
            xStep = -1.0f;
        }
        if (quad.getPosition().x < -xBouncePoint)
        {
            xStep = 1.0f;
        }

        // Check for player intersection
        constexpr float collisionBias = 0.6f;
        constexpr float selfBias = 0.1f;

        if (getPosition().x - selfBias < player->getPosition().x + player->getSize().x / 2
            && getPosition().x + selfBias > player->getPosition().x - player->getSize().x / 2
            && getPosition().y - selfBias< player->getPosition().y + player->getSize().y / 2 - collisionBias
            && getPosition().y + selfBias > player->getPosition().y - player->getSize().y / 2 - collisionBias)
        {
            yStep = -yStep;
            return;
        }

        const auto& boxes = grid->getBoxes();
        for (size_t i = 0; i < boxes.size(); i++)
        {
            if (boxes[i].hit(getPosition(), getSize()))
            {
                yStep = -yStep;
                grid->destroyBox(i);
                return;
            }
        }

        if (quad.getPosition().y > yBouncePoint)
        {
            yStep = -1.0f;
        }
        if (quad.getPosition().y < -yBouncePoint)
        {
            yStep = 1.0f;
        }
    }
};


// -------------------------------------------------------------------------------------------
class Application
{
private:
    GLFWwindow* window{ nullptr };
    ALCdevice* audioDevice{ nullptr };
    ALCcontext* audioContext{ nullptr };

    Ref<PlayerPlatform> player;
    Ref<Ball> ball;
    Ref<BoxGrid> grid;

    Mat4 orthoMatrix;
public:
    Application(int width, int height, std::string_view title)
    {
        initWindow(width, height, title);
        initContext();
        initAudio();
        createResources();
    }

    int run()
    {
        glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
        while (!glfwWindowShouldClose(window))
        {
            auto now = glfwGetTime();
            glfwPollEvents();
            glClear(GL_COLOR_BUFFER_BIT);
            render();
            glfwSwapBuffers(window);
            update(static_cast<float>(glfwGetTime() - now));
        }

        glfwTerminate();
        return 0;
    }

private:
    void update(float deltaTime)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window, 1);
        }

        if (glfwGetKey(window, GLFW_KEY_R))
        {
            createResources();
        }

        player->move(deltaTime * -glfwGetKey(window, GLFW_KEY_A), 1.5f);
        player->move(deltaTime * glfwGetKey(window, GLFW_KEY_D), 1.5f);
        ball->bounce(deltaTime * 1.5f);
    }

    void render()
    {
        player->draw(orthoMatrix);
        ball->draw(orthoMatrix);
        grid->draw(orthoMatrix);
    }

private:
    void initWindow(int width, int height, std::string_view title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
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

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void initAudio()
    {
        audioDevice = alcOpenDevice(nullptr);

        if (!audioDevice)
        {
            throw std::runtime_error("Failed to init OpenAL");
        }

        audioContext = alcCreateContext(audioDevice, nullptr);
        if (!audioContext)
        {
            throw std::runtime_error("Failed to init OpenAL context");
        }

        alcMakeContextCurrent(audioContext);
    }

    // Load assets
    void createResources()
    {
        player = Ref<PlayerPlatform>::make(Vec2{ 0.0f, -0.6f }, Vec2{ 0.4f, 0.05f });

        // Starting margin
        //  -2.0f + margin + xSize / 2, 1.5f - margin - ySize / 2

        constexpr int gridX = 10;
        constexpr int gridY = 9;

        constexpr float margin = 0.01f;
        constexpr float xSize = 4.0f / gridX - margin * 1.1f;
        constexpr float ySize = xSize / 3;

        grid = Ref<BoxGrid>::make(Vec2{ -2.0f + margin + xSize / 2, 1.5f - margin - ySize / 2 }, Vec2(xSize, ySize), margin, gridX, gridY);
        ball = Ref<Ball>::make(Vec2{ 0.0f, 0.0f }, Vec2{ 0.1f, 0.1f }, player, grid);

        orthoMatrix = glm::ortho(-2.0f, 2.0f, -1.5f, 1.5f);

        Ref<AudioEntry> entry = Ref<AudioEntry>::make("audio/click.wav");
    }

};


// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------
int main()
{
    try
    {
        auto app = std::make_unique<Application>(800, 600, "Arcanoid");
        return app->run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    return main();
}
#endif