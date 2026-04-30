#include "engine/scripting/angelscript.hpp"

#include <new>

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
            "void DrawTexture(const string &in name, int x, int y)",
            asMETHOD(Runtime, DrawTexture),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureEx(const string &in name, int x, int y, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawTextureEx),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureRot(const string &in name, int x, int y, float rotation)",
            asMETHOD(Runtime, DrawTextureRot),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextureRotEx(const string &in name, int x, int y, float rotation, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawTextureRotEx),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTexturePro(const string &in name, int x, int y, int w, int h, float rotation, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawTexturePro),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Primitives");

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawRectangle(float x, float y, float w, float h, CE::Graphics::Colour colour, float rotation = 0.0f)",
            asMETHOD(Runtime, DrawRectangle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawCircle(float x, float y, float radius, int segments, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawCircle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawLine(float x1, float y1, float x2, float y2, float thickness, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawLine),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, CE::Graphics::Colour colour, float rotation = 0.0f)",
            asMETHOD(Runtime, DrawTriangle),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawRectangleLines(float x, float y, float w, float h, float thickness, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawRectangleLines),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawCircleLines(float x, float y, float radius, int segments, float thickness, CE::Graphics::Colour colour)",
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
            "void DrawTextCol(const string &in text, int x, int y, float size, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawTextCol),
            asCALL_THISCALL_ASGLOBAL,
            this
        );
        if (result < 0) {
            return false;
        }

        result = mScriptEngine->RegisterGlobalFunction(
            "void DrawTextEx(const string &in text, const string &in name, int x, int y, float size, CE::Graphics::Colour colour)",
            asMETHOD(Runtime, DrawTextEx),
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

    void Runtime::DrawTexture(const std::string& name, int x, int y) {
        mTextureManager.Draw(name.c_str(), x, y, kWhite);
    }

    void Runtime::DrawTextureEx(const std::string& name, int x, int y, Renderer::Colour colour) {
        mTextureManager.Draw(name.c_str(), x, y, colour);
    }

    void Runtime::DrawTextureRot(const std::string& name, int x, int y, float rotation) {
        mTextureManager.DrawRot(name.c_str(), x, y, rotation, kWhite);
    }

    void Runtime::DrawTextureRotEx(const std::string& name, int x, int y, float rotation, Renderer::Colour colour) {
        mTextureManager.DrawRot(name.c_str(), x, y, rotation, colour);
    }

    void Runtime::DrawTexturePro(const std::string& name, int x, int y, int w, int h, float rotation, Renderer::Colour colour) {
        mTextureManager.DrawPro(name.c_str(), x, y, w, h, rotation, colour);
    }

    void Runtime::DrawRectangle(float x, float y, float w, float h, Renderer::Colour colour, float rotation) {
        mRenderer.DrawRect(x, y, w, h, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawCircle(float x, float y, float radius, int segments, Renderer::Colour colour) {
        mRenderer.DrawCircle(x, y, radius, segments, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawLine(float x1, float y1, float x2, float y2, float thickness, Renderer::Colour colour) {
        mRenderer.DrawLine(x1, y1, x2, y2, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, Renderer::Colour colour, float rotation) {
        mRenderer.DrawTriangle(x0, y0, x1, y1, x2, y2, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawRectangleLines(float x, float y, float w, float h, float thickness, Renderer::Colour colour) {
        mRenderer.DrawRectLines(x, y, w, h, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawCircleLines(float x, float y, float radius, int segments, float thickness, Renderer::Colour colour) {
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

    void Runtime::DrawTextCol(const std::string& text, int x, int y, float size, Renderer::Colour colour) {
        mFontManager.Draw(text, x, y, size, colour);
    }

    void Runtime::DrawTextEx(const std::string& text, const std::string& name, int x, int y, float size, Renderer::Colour colour) {
        mFontManager.DrawEx(text, name, x, y, size, colour);
    }
}
