#include "engine/scripting/angelscript.hpp"
#include "engine/assets/animations.hpp"

#include <new>
#include <scriptarray/scriptarray.h>

namespace {
    constexpr CE::Renderer::Colour kWhite {255, 255, 255, 255};
}

namespace CE::Scripting {
    void Runtime::ConstructColour(Renderer::Colour* self) {
        new (self) Renderer::Colour();
    }

    void Runtime::ConstructColourRGBA(
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a,
        Renderer::Colour* self
    ) {
        new (self) Renderer::Colour {r, g, b, a};
    }

    bool Runtime::RegisterAssetsBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        int result = 0;

        mScriptEngine->SetDefaultNamespace("CE::Graphics");

        result = mScriptEngine->RegisterObjectType(
            "Colour",
            sizeof(Renderer::Colour),
            asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Renderer::Colour>()
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Colour",
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(ConstructColour),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectBehaviour(
            "Colour",
            asBEHAVE_CONSTRUCT,
            "void f(uint8 r, uint8 g, uint8 b, uint8 a)",
            asFUNCTION(ConstructColourRGBA),
            asCALL_CDECL_OBJLAST
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Colour", "uint8 r", asOFFSET(Renderer::Colour, r));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Colour", "uint8 g", asOFFSET(Renderer::Colour, g));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Colour", "uint8 b", asOFFSET(Renderer::Colour, b));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterObjectProperty("Colour", "uint8 a", asOFFSET(Renderer::Colour, a));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnum("ShaderStage");
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnumValue("ShaderStage", "Vertex", static_cast<int>(Renderer::ShaderStage::Vertex));
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterEnumValue("ShaderStage", "Fragment", static_cast<int>(Renderer::ShaderStage::Fragment));
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Textures");

        result = mScriptEngine->RegisterGlobalFunction(
            "void LoadTexture(const string &in path, const string &in name)",
            asMETHOD(Runtime, LoadTexture),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadTexture(const string &in name)",
            asMETHOD(Runtime, UnloadTexture),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTexture(const string &in name, int x, int y, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            asMETHOD(Runtime, DrawTexture),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureEx(const string &in name, int x, int y, const CE::Graphics::Colour &in colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            asMETHOD(Runtime, DrawTextureEx),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureRot(const string &in name, int x, int y, float rotation, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            asMETHOD(Runtime, DrawTextureRot),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureRotEx(const string &in name, int x, int y, float rotation, const CE::Graphics::Colour &in colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            asMETHOD(Runtime, DrawTextureRotEx),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTexturePro(const string &in name, int x, int y, int w, int h, float rotation, const CE::Graphics::Colour &in colour, bool flipX = false, bool flipY = false, float tileX = 1.0f, float tileY = 1.0f)",
            asMETHOD(Runtime, DrawTexturePro),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Shaders");

        result = mScriptEngine->RegisterGlobalFunction(
            "bool CreateShaderProgram(const string &in name)",
            asMETHOD(Runtime, CreateShaderProgram),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool LoadShader(const string &in path, const string &in name, int fragmentSamplerCount = 4)",
            asMETHOD(Runtime, LoadShader),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool LoadShaderStage(const string &in program, const string &in path, CE::Graphics::ShaderStage stage, int samplerCount = 1)",
            asMETHOD(Runtime, LoadShaderStage),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool UseDefaultShaderStage(const string &in program, CE::Graphics::ShaderStage stage)",
            asMETHOD(Runtime, UseDefaultShaderStage),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool CompileShaderProgram(const string &in name)",
            asMETHOD(Runtime, CompileShaderProgram),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool BindShader(const string &in name)",
            asMETHOD(Runtime, BindShaderProgram),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnbindShader()",
            asMETHOD(Runtime, UnbindShaderProgram),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadShader(const string &in name)",
            asMETHOD(Runtime, UnloadShader),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderFloat(const string &in uniformName, float value)",
            asMETHOD(Runtime, SetShaderFloat),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderVec2(const string &in uniformName, float x, float y)",
            asMETHOD(Runtime, SetShaderVec2),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderVec3(const string &in uniformName, float x, float y, float z)",
            asMETHOD(Runtime, SetShaderVec3),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderVec4(const string &in uniformName, float x, float y, float z, float w)",
            asMETHOD(Runtime, SetShaderVec4),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderMat4(const string &in uniformName, const array<float>@ values)",
            asMETHOD(Runtime, SetShaderMat4),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetShaderInt(const string &in uniformName, int value)",
            asMETHOD(Runtime, SetShaderInt),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "bool SetShaderTexture(const string &in uniformName, const string &in textureName, int slot = 0)",
            asMETHOD(Runtime, SetShaderTexture),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Primitives");

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawRectangle(float x, float y, float w, float h, const CE::Graphics::Colour &in colour, float rotation = 0.0f)",
            asMETHOD(Runtime, DrawRectangle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawCircle(float x, float y, float radius, int segments, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawCircle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawLine(float x1, float y1, float x2, float y2, float thickness, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawLine),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const CE::Graphics::Colour &in colour, float rotation = 0.0f)",
            asMETHOD(Runtime, DrawTriangle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawRectangleLines(float x, float y, float w, float h, float thickness, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawRectangleLines),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawCircleLines(float x, float y, float radius, int segments, float thickness, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawCircleLines),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Text");

        result = mScriptEngine->RegisterGlobalFunction(
            "bool LoadFont(const string &in path, const string &in name, int size)",
            asMETHOD(Runtime, LoadFont),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadFont(const string &in name)",
            asMETHOD(Runtime, UnloadFont),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawText(const string &in text, int x, int y, float size)",
            asMETHOD(Runtime, DrawText),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextCol(const string &in text, int x, int y, float size, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawTextCol),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextEx(const string &in text, const string &in name, int x, int y, float size, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, DrawTextEx),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Animations");

        result = mScriptEngine->RegisterGlobalFunction(
            "void LoadAnimation(const string &in path, const string &in name)",
            asMETHOD(Runtime, LoadAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void UnloadAnimation(const string &in name)",
            asMETHOD(Runtime, UnloadAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "uint CreateInstance(const string &in name)",
            asMETHOD(Runtime, CreateAnimationInstance),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DeleteInstance(uint handle)",
            asMETHOD(Runtime, DeleteAnimationInstance),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void Play(uint handle, int x, int y, bool loop = false, bool autoRender = true)",
            asMETHOD(Runtime, PlayAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void PlayRot(uint handle, int x, int y, bool loop, float rotation, bool autoRender = true)",
            asMETHOD(Runtime, PlayAnimationRot),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetPosition(uint handle, int x, int y, float rotation = 0.0f)",
            asMETHOD(Runtime, SetAnimationPosition),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void Seek(uint handle, uint frame)",
            asMETHOD(Runtime, SeekAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetDrawMode(uint handle, bool autoRender)",
            asMETHOD(Runtime, SetAnimationDrawMode),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetLooping(uint handle, bool loop)",
            asMETHOD(Runtime, SetAnimationLooping),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void SetTint(uint handle, const CE::Graphics::Colour &in colour)",
            asMETHOD(Runtime, SetAnimationTint),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void Pause(uint handle)",
            asMETHOD(Runtime, PauseAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void Stop(uint handle)",
            asMETHOD(Runtime, StopAnimation),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawFrame(uint handle)",
            asMETHOD(Runtime, DrawAnimationFrame),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    void Runtime::LoadTexture(const std::string& path, const std::string& name) {
        mTextureManager.Load(path.c_str(), name.c_str());
    }

    void Runtime::UnloadTexture(const std::string& name) {
        mTextureManager.Unload(name.c_str());
    }

    void Runtime::DrawTexture(const std::string& name, int x, int y, bool flipX, bool flipY, float tileX, float tileY) {
        mTextureManager.Draw(name.c_str(), x, y, kWhite, flipX, flipY, tileX, tileY);
    }

    void Runtime::DrawTextureEx(const std::string& name, int x, int y, const Renderer::Colour& colour, bool flipX, bool flipY, float tileX, float tileY) {
        mTextureManager.Draw(name.c_str(), x, y, colour, flipX, flipY, tileX, tileY);
    }

    void Runtime::DrawTextureRot(const std::string& name, int x, int y, float rotation, bool flipX, bool flipY, float tileX, float tileY) {
        mTextureManager.DrawRot(name.c_str(), x, y, rotation, kWhite, flipX, flipY, tileX, tileY);
    }

    void Runtime::DrawTextureRotEx(const std::string& name, int x, int y, float rotation, const Renderer::Colour& colour, bool flipX, bool flipY, float tileX, float tileY) {
        mTextureManager.DrawRot(name.c_str(), x, y, rotation, colour, flipX, flipY, tileX, tileY);
    }

    void Runtime::DrawTexturePro(const std::string& name, int x, int y, int w, int h, float rotation, const Renderer::Colour& colour, bool flipX, bool flipY, float tileX, float tileY) {
        mTextureManager.DrawPro(name.c_str(), x, y, w, h, rotation, colour, flipX, flipY, tileX, tileY);
    }

    bool Runtime::CreateShaderProgram(const std::string& name) {
        return mShaderManager.CreateProgram(name.c_str());
    }

    bool Runtime::LoadShader(const std::string& path, const std::string& name, int fragmentSamplerCount) {
        return mShaderManager.Load(path.c_str(), name.c_str(), fragmentSamplerCount);
    }

    bool Runtime::LoadShaderStage(const std::string& program, const std::string& path, Renderer::ShaderStage stage, int samplerCount) {
        return mShaderManager.LoadStage(program.c_str(), path.c_str(), stage, samplerCount);
    }

    bool Runtime::UseDefaultShaderStage(const std::string& program, Renderer::ShaderStage stage) {
        return mShaderManager.UseDefaultStage(program.c_str(), stage);
    }

    bool Runtime::CompileShaderProgram(const std::string& name) {
        return mShaderManager.Compile(name.c_str());
    }

    bool Runtime::BindShaderProgram(const std::string& name) {
        return mShaderManager.Bind(name.c_str());
    }

    void Runtime::UnbindShaderProgram() {
        mShaderManager.Unbind();
    }

    void Runtime::UnloadShader(const std::string& name) {
        mShaderManager.Unload(name.c_str());
    }

    void Runtime::SetShaderFloat(const std::string& uniformName, float value) {
        mShaderManager.SetFloat(uniformName.c_str(), value);
    }

    void Runtime::SetShaderVec2(const std::string& uniformName, float x, float y) {
        mShaderManager.SetVec2(uniformName.c_str(), x, y);
    }

    void Runtime::SetShaderVec3(const std::string& uniformName, float x, float y, float z) {
        mShaderManager.SetVec3(uniformName.c_str(), x, y, z);
    }

    void Runtime::SetShaderVec4(const std::string& uniformName, float x, float y, float z, float w) {
        mShaderManager.SetVec4(uniformName.c_str(), x, y, z, w);
    }

    void Runtime::SetShaderMat4(const std::string& uniformName, const CScriptArray* values) {
        if (!values || values->GetSize() < 16) {
            return;
        }

        float matrix[16] {};
        for (asUINT i = 0; i < 16; ++i) {
            matrix[i] = *static_cast<const float*>(values->At(i));
        }
        mShaderManager.SetMat4(uniformName.c_str(), matrix);
    }

    void Runtime::SetShaderInt(const std::string& uniformName, int value) {
        mShaderManager.SetInt(uniformName.c_str(), value);
    }

    bool Runtime::SetShaderTexture(const std::string& uniformName, const std::string& textureName, int slot) {
        return mShaderManager.SetTexture(uniformName.c_str(), textureName.c_str(), slot);
    }

    void Runtime::DrawRectangle(float x, float y, float w, float h, const Renderer::Colour& colour, float rotation) {
        mRenderer.DrawRect(x, y, w, h, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawCircle(float x, float y, float radius, int segments, const Renderer::Colour& colour) {
        mRenderer.DrawCircle(x, y, radius, segments, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawLine(float x1, float y1, float x2, float y2, float thickness, const Renderer::Colour& colour) {
        mRenderer.DrawLine(x1, y1, x2, y2, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const Renderer::Colour& colour, float rotation) {
        mRenderer.DrawTriangle(x0, y0, x1, y1, x2, y2, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawRectangleLines(float x, float y, float w, float h, float thickness, const Renderer::Colour& colour) {
        mRenderer.DrawRectLines(x, y, w, h, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawCircleLines(float x, float y, float radius, int segments, float thickness, const Renderer::Colour& colour) {
        mRenderer.DrawCircleLines(x, y, radius, segments, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    bool Runtime::LoadFont(const std::string& path, const std::string& name, int size) {
        return mFontManager.Load(path, name, size);
    }

    void Runtime::UnloadFont(const std::string& name) {
        mFontManager.Unload(name);
    }

    void Runtime::DrawText(const std::string& text, int x, int y, float size) {
        mFontManager.Draw(text, x, y, size, kWhite);
    }

    void Runtime::DrawTextCol(const std::string& text, int x, int y, float size, const Renderer::Colour& colour) {
        mFontManager.Draw(text, x, y, size, colour);
    }

    void Runtime::DrawTextEx(const std::string& text, const std::string& name, int x, int y, float size, const Renderer::Colour& colour) {
        mFontManager.DrawEx(text, name, x, y, size, colour);
    }

    void Runtime::LoadAnimation(const std::string& path, const std::string& name) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Load(name, path);
    }

    void Runtime::UnloadAnimation(const std::string& name) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Unload(name);
    }

    uint32_t Runtime::CreateAnimationInstance(const std::string& name) {
        if (!mAnimationManager) {
            return 0;
        }
        return mAnimationManager->CreateInstance(name);
    }

    void Runtime::DeleteAnimationInstance(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->DeleteInstance(handle);
    }

    void Runtime::PlayAnimation(uint32_t handle, int x, int y, bool loop, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Play(handle, x, y, loop, autoRender);
    }

    void Runtime::PlayAnimationRot(uint32_t handle, int x, int y, bool loop, float rotation, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->PlayRot(handle, x, y, loop, rotation, autoRender);
    }

    void Runtime::SetAnimationPosition(uint32_t handle, int x, int y, float rotation) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetPosition(handle, x, y, rotation);
    }

    void Runtime::SeekAnimation(uint32_t handle, uint32_t frame) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Seek(handle, frame);
    }

    void Runtime::SetAnimationDrawMode(uint32_t handle, bool autoRender) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetDrawMode(handle, autoRender);
    }

    void Runtime::SetAnimationLooping(uint32_t handle, bool loop) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetLooping(handle, loop);
    }

    void Runtime::SetAnimationTint(uint32_t handle, const Renderer::Colour& colour) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->SetTint(handle, colour);
    }

    void Runtime::PauseAnimation(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Pause(handle);
    }

    void Runtime::StopAnimation(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->Stop(handle);
    }

    void Runtime::DrawAnimationFrame(uint32_t handle) {
        if (!mAnimationManager) {
            return;
        }
        mAnimationManager->DrawFrame(handle);
    }
}
