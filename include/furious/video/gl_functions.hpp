#pragma once

#include <GLFW/glfw3.h>
#include <cstddef>

// GL types not in GL 1.1 headers
#ifndef GL_GLEXT_TYPES_DEFINED
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
#endif

// GL constants not in GL 1.1 headers
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_RG8
#define GL_RG8 0x822B
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

namespace furious {

// Function pointer types
using PFNGLCREATESHADERPROC = GLuint(APIENTRY*)(GLenum type);
using PFNGLDELETESHADERPROC = void(APIENTRY*)(GLuint shader);
using PFNGLSHADERSOURCEPROC = void(APIENTRY*)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
using PFNGLCOMPILESHADERPROC = void(APIENTRY*)(GLuint shader);
using PFNGLGETSHADERIVPROC = void(APIENTRY*)(GLuint shader, GLenum pname, GLint* params);
using PFNGLGETSHADERINFOLOGPROC = void(APIENTRY*)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using PFNGLCREATEPROGRAMPROC = GLuint(APIENTRY*)(void);
using PFNGLDELETEPROGRAMPROC = void(APIENTRY*)(GLuint program);
using PFNGLATTACHSHADERPROC = void(APIENTRY*)(GLuint program, GLuint shader);
using PFNGLLINKPROGRAMPROC = void(APIENTRY*)(GLuint program);
using PFNGLGETPROGRAMIVPROC = void(APIENTRY*)(GLuint program, GLenum pname, GLint* params);
using PFNGLGETPROGRAMINFOLOGPROC = void(APIENTRY*)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using PFNGLUSEPROGRAMPROC = void(APIENTRY*)(GLuint program);
using PFNGLGETUNIFORMLOCATIONPROC = GLint(APIENTRY*)(GLuint program, const GLchar* name);
using PFNGLUNIFORM1IPROC = void(APIENTRY*)(GLint location, GLint v0);
using PFNGLACTIVETEXTUREPROC = void(APIENTRY*)(GLenum texture);
using PFNGLGENFRAMEBUFFERSPROC = void(APIENTRY*)(GLsizei n, GLuint* framebuffers);
using PFNGLDELETEFRAMEBUFFERSPROC = void(APIENTRY*)(GLsizei n, const GLuint* framebuffers);
using PFNGLBINDFRAMEBUFFERPROC = void(APIENTRY*)(GLenum target, GLuint framebuffer);
using PFNGLFRAMEBUFFERTEXTURE2DPROC = void(APIENTRY*)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC = GLenum(APIENTRY*)(GLenum target);
using PFNGLGENVERTEXARRAYSPROC = void(APIENTRY*)(GLsizei n, GLuint* arrays);
using PFNGLDELETEVERTEXARRAYSPROC = void(APIENTRY*)(GLsizei n, const GLuint* arrays);
using PFNGLBINDVERTEXARRAYPROC = void(APIENTRY*)(GLuint array);
using PFNGLGENBUFFERSPROC = void(APIENTRY*)(GLsizei n, GLuint* buffers);
using PFNGLDELETEBUFFERSPROC = void(APIENTRY*)(GLsizei n, const GLuint* buffers);
using PFNGLBINDBUFFERPROC = void(APIENTRY*)(GLenum target, GLuint buffer);
using PFNGLBUFFERDATAPROC = void(APIENTRY*)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void(APIENTRY*)(GLuint index);
using PFNGLVERTEXATTRIBPOINTERPROC = void(APIENTRY*)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

struct GLFunctions {
    PFNGLCREATESHADERPROC CreateShader = nullptr;
    PFNGLDELETESHADERPROC DeleteShader = nullptr;
    PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC CompileShader = nullptr;
    PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
    PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
    PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
    PFNGLATTACHSHADERPROC AttachShader = nullptr;
    PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC UseProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
    PFNGLUNIFORM1IPROC Uniform1i = nullptr;
    PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
    PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
    PFNGLGENBUFFERSPROC GenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC DeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC BindBuffer = nullptr;
    PFNGLBUFFERDATAPROC BufferData = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;

    bool load() {
        CreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(glfwGetProcAddress("glCreateShader"));
        DeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(glfwGetProcAddress("glDeleteShader"));
        ShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(glfwGetProcAddress("glShaderSource"));
        CompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(glfwGetProcAddress("glCompileShader"));
        GetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(glfwGetProcAddress("glGetShaderiv"));
        GetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(glfwGetProcAddress("glGetShaderInfoLog"));
        CreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(glfwGetProcAddress("glCreateProgram"));
        DeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(glfwGetProcAddress("glDeleteProgram"));
        AttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(glfwGetProcAddress("glAttachShader"));
        LinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(glfwGetProcAddress("glLinkProgram"));
        GetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(glfwGetProcAddress("glGetProgramiv"));
        GetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(glfwGetProcAddress("glGetProgramInfoLog"));
        UseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(glfwGetProcAddress("glUseProgram"));
        GetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(glfwGetProcAddress("glGetUniformLocation"));
        Uniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(glfwGetProcAddress("glUniform1i"));
        ActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(glfwGetProcAddress("glActiveTexture"));
        GenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(glfwGetProcAddress("glGenFramebuffers"));
        DeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(glfwGetProcAddress("glDeleteFramebuffers"));
        BindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(glfwGetProcAddress("glBindFramebuffer"));
        FramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(glfwGetProcAddress("glFramebufferTexture2D"));
        CheckFramebufferStatus = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(glfwGetProcAddress("glCheckFramebufferStatus"));
        GenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(glfwGetProcAddress("glGenVertexArrays"));
        DeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(glfwGetProcAddress("glDeleteVertexArrays"));
        BindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(glfwGetProcAddress("glBindVertexArray"));
        GenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(glfwGetProcAddress("glGenBuffers"));
        DeleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(glfwGetProcAddress("glDeleteBuffers"));
        BindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(glfwGetProcAddress("glBindBuffer"));
        BufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(glfwGetProcAddress("glBufferData"));
        EnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(glfwGetProcAddress("glEnableVertexAttribArray"));
        VertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(glfwGetProcAddress("glVertexAttribPointer"));

        return CreateShader && CreateProgram && GenFramebuffers && ActiveTexture;
    }
};

} // namespace furious
