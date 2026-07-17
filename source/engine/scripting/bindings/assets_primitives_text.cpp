#include "engine/scripting/angelscript.hpp"
#include "engine/scripting/scripting_macros.hpp"

namespace {
    constexpr CE::Renderer::Colour kWhite{255, 255, 255, 255};
}

namespace CE::Scripting {
    bool Runtime::RegisterAssetPrimitiveBindings() {
        if (mScriptEngine == nullptr) {
            return false;
        }

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Primitives");
        CE_REGISTER_GLOBAL(
            "void DrawRectangle(float x, float y, float w, float h, const CE::Graphics::Colour &in colour, "
            "float rotation = 0.0f)",
            DrawRectangle);
        CE_REGISTER_GLOBAL(
            "void DrawCircle(float x, float y, float radius, int segments, const CE::Graphics::Colour &in colour)",
            DrawCircle);
        CE_REGISTER_GLOBAL("void DrawLine(float x1, float y1, float x2, float y2, float thickness, const "
                           "CE::Graphics::Colour &in colour)",
                           DrawLine);
        CE_REGISTER_GLOBAL("void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const "
                           "CE::Graphics::Colour &in colour, float rotation = 0.0f)",
                           DrawTriangle);
        CE_REGISTER_GLOBAL("void DrawRectangleLines(float x, float y, float w, float h, float thickness, const "
                           "CE::Graphics::Colour &in colour)",
                           DrawRectangleLines);
        CE_REGISTER_GLOBAL("void DrawCircleLines(float x, float y, float radius, int segments, float thickness, const "
                           "CE::Graphics::Colour &in colour)",
                           DrawCircleLines);

        mScriptEngine->SetDefaultNamespace("CE::Graphics::Text");
        CE_REGISTER_GLOBAL("bool LoadFont(const string &in path, const string &in name, int size)", LoadFont);
        CE_REGISTER_GLOBAL("void UnloadFont(const string &in name)", UnloadFont);
        CE_REGISTER_GLOBAL("void DrawText(const string &in text, int x, int y, float size)", DrawText);
        CE_REGISTER_GLOBAL(
            "void DrawTextCol(const string &in text, int x, int y, float size, const CE::Graphics::Colour &in colour)",
            DrawTextCol);
        CE_REGISTER_GLOBAL(
            "void DrawTextEx(const string &in text, const string &in name, int x, int y, float size, const "
            "CE::Graphics::Colour &in colour)",
            DrawTextEx);

        mScriptEngine->SetDefaultNamespace("");
        return true;
    }

    void Runtime::DrawRectangle(float x, float y, float w, float h, const Renderer::Colour &colour, float rotation) {
        mRenderer.DrawRect(x, y, w, h, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawCircle(float x, float y, float radius, int segments, const Renderer::Colour &colour) {
        mRenderer.DrawCircle(x, y, radius, segments, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawLine(float x1, float y1, float x2, float y2, float thickness, const Renderer::Colour &colour) {
        mRenderer.DrawLine(x1, y1, x2, y2, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
                               const Renderer::Colour &colour, float rotation) {
        mRenderer.DrawTriangle(x0, y0, x1, y1, x2, y2, colour.r, colour.g, colour.b, colour.a, rotation);
    }

    void Runtime::DrawRectangleLines(float x, float y, float w, float h, float thickness,
                                     const Renderer::Colour &colour) {
        mRenderer.DrawRectLines(x, y, w, h, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    void Runtime::DrawCircleLines(float x, float y, float radius, int segments, float thickness,
                                  const Renderer::Colour &colour) {
        mRenderer.DrawCircleLines(x, y, radius, segments, thickness, colour.r, colour.g, colour.b, colour.a);
    }

    bool Runtime::LoadFont(const std::string &path, const std::string &name, int size) {
        return mFontManager.Load(path, name, size);
    }

    void Runtime::UnloadFont(const std::string &name) {
        mFontManager.Unload(name);
    }

    void Runtime::DrawText(const std::string &text, int x, int y, float size) {
        mFontManager.Draw(text, x, y, size, {0, 0, 0, 255});
    }

    void Runtime::DrawTextEx(const std::string &text, const std::string &name, int x, int y, float size,
                             const Renderer::Colour &colour) {
        mFontManager.DrawEx(text, name, x, y, size, colour);
    }

    void Runtime::DrawTextCol(const std::string &text, int x, int y, float size, const Renderer::Colour &colour) {
        mFontManager.Draw(text, x, y, size, colour);
    }
} // namespace CE::Scripting
