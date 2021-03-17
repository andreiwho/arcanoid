#include <iostream>
#include <stdexcept>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Application
{
private:
    GLFWwindow* window{ nullptr };
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

            glfwPollEvents();

            glfwSwapBuffers(window);
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
        if (glewInit() != GLEW_OK)
        {
            throw std::runtime_error("Failed to init opengl");
        }
    }

    void createResources()
    {

    }

};

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