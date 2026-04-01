#include <vector>

#include "engine/renderers/opengl.hpp"
#include "engine/common/tracelog.hpp"
#include "engine/core.hpp"

#include "third_party/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CE::Renderers::OpenGL::Utils {
    static void CreateWhiteTexture() {
        if (whiteTexture.handle)
            return;

        GLuint texId = 0;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        const uint32_t pixel = 0xFFFFFFFFu;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
        glBindTexture(GL_TEXTURE_2D, 0);

        allocatedTextures.push_back(texId);

        whiteTexture.handle = reinterpret_cast<void*>(static_cast<uintptr_t>(texId));
        whiteTexture.width = 1;
        whiteTexture.height = 1;
        whiteTexture.format = CE::Renderers::TextureFormat::RGBA8;
    }

    static GLuint CompileShader(GLenum type, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint logLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 1));
            glGetShaderInfoLog(shader, logLen, nullptr, log.data());
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Shader compile failed: {}", log.data());
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    static GLuint LinkProgram(GLuint vs, GLuint fs) {
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

        GLint ok = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint logLen = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(static_cast<size_t>(logLen > 1 ? logLen : 1));
            glGetProgramInfoLog(prog, logLen, nullptr, log.data());
            CE::Log(CE::LogLevel::Fatal, "[OpenGL Renderer] Program link failed: {}", log.data());
            glDeleteProgram(prog);
            return 0;
        }

        return prog;
    }

    GLuint GetGLTexture(const Texture& t) {
        return (GLuint)(uintptr_t)t.handle;
    }

    static bool ReadAllBytesFromVFS(const char* virtual_path, std::vector<uint8_t>& out_bytes) {
        out_bytes.clear();
        if (!virtual_path)
            return false;

        auto& vfs = CE::Global::GetVFS();
        uint64_t size64 = 0;
        if (!vfs.GetFileSize(virtual_path, size64))
            return false;

        if (size64 == 0) {
            out_bytes.clear();
            return true;
        }

        if (size64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            return false;

        auto* file = vfs.OpenFile(virtual_path);
        if (!file)
            return false;

        out_bytes.resize(static_cast<size_t>(size64));
        size_t offset = 0;
        while (offset < out_bytes.size()) {
            const size_t n = vfs.ReadFile(file, out_bytes.data() + offset, out_bytes.size() - offset);
            if (n == 0)
                break;
            offset += n;
        }
        vfs.CloseFile(file);

        if (offset != out_bytes.size())
            return false;

        return true;
    }
}