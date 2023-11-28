#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_OPENGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>

#include <iostream>

#ifdef _DEBUG
    #define ASSERT(x) if(!(x)) __debugbreak();
    #define GLCall(x) GLClearError();\
            x;\
            ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#else
    #define ASSERT(x)
    #define GLCall(x) x
#endif

#define PROFILE 1


inline void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

inline bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenglGL Error] (" << error << "): " << function << " " << file << ": " << line << std::endl;
        return false;
    }
    return true;
}
