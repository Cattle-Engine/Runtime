#pragma once

#include <angelscript.h>
#include <string>

#include "engine/common/fs/vfs.hpp"
#include "engine/common/misc/gameinfo.hpp"
#include "engine/settings.hpp"
#include "engine/assets/fonts.hpp"
#include "engine/assets/textures.hpp"
#include "engine/input/keyboard.hpp"
#include "engine/input/mouse.hpp"
#include "engine/renderer.hpp"

namespace CE {
    class Instance;
}

namespace CE::Scripting {
    class Runtime {
        public:
            Runtime(
                VFS::VFS& vfs,
                CE::GameInfo& gameInfo,
                Settings::SettingsManager& settingsManager,
                Instance& instance,
                Renderer::IRenderer& renderer,
                Assets::Textures::TextureManager& textureManager,
                Assets::Fonts::FontManager& fontManager,
                Input::Keyboard& keyboard,
                Input::Mouse& mouse
            );
            ~Runtime();

            bool RunStartup();
            bool RunUpdate(); // TODO ADD AN ACTUAL CALLBACK SYSTEM
            bool Initialize();
            const std::string& GetLastError() const;

        private:
            static void MessageCallback(const asSMessageInfo* msg, void* param);

            bool RegisterAssetsBindings();
            bool RegisterInputBindings();
            bool RegisterInstanceBindings();
            bool Fail(const std::string& message);

            static void ConstructColour(Renderer::Colour* self);
            static void ConstructColourRGBA(
                uint8_t r,
                uint8_t g,
                uint8_t b,
                uint8_t a,
                Renderer::Colour* self
            );

            void LoadTexture(const std::string& path, const std::string& name);
            void UnloadTexture(const std::string& name);
            void DrawTexture(const std::string& name, int x, int y);
            void DrawTextureEx(const std::string& name, int x, int y, const Renderer::Colour& colour);
            void DrawTextureRot(const std::string& name, int x, int y, float rotation);
            void DrawTextureRotEx(const std::string& name, int x, int y, float rotation, const Renderer::Colour& colour);
            void DrawTexturePro(const std::string& name, int x, int y, int w,
                int h, float rotation, const Renderer::Colour& colour);
            void DrawRectangle(float x, float y, float w, float h, const Renderer::Colour& colour, float rotation);
            void DrawCircle(float x, float y, float radius, int segments, const Renderer::Colour& colour);
            void DrawLine(float x1, float y1, float x2, float y2, float thickness, const Renderer::Colour& colour);
            void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const Renderer::Colour& colour, float rotation);
            void DrawRectangleLines(float x, float y, float w, float h, float thickness, const Renderer::Colour& colour);
            void DrawCircleLines(float x, float y, float radius, int segments, float thickness, const Renderer::Colour& colour);

            bool LoadFont(const std::string& path, const std::string& name, int size);
            void UnloadFont(const std::string& name);
            void DrawText(const std::string& text, int x, int y, float size);
            void DrawTextEx(const std::string& text, const std::string& name, int x, int y, float size, const Renderer::Colour& colour);
            void DrawTextCol(const std::string& text, int x, int y, float size, const Renderer::Colour& colour);

            bool IsKeyDown(Input::KeyboardKeys key);
            bool IsKeyPressed(Input::KeyboardKeys key);
            bool IsKeyReleased(Input::KeyboardKeys key);

            bool IsMouseButtonDown(Input::MouseButtons button);
            bool IsMouseButtonPressed(Input::MouseButtons button);
            bool IsMouseButtonReleased(Input::MouseButtons button);
            int GetMouseX();
            int GetMouseY();
            int GetMouseDeltaX();
            int GetMouseDeltaY();
            int GetMouseWheelX();
            int GetMouseWheelY();

            void ExitInstance();
            float GetDeltaTime();
            float GetFrameTime();
            int GetFPS();
            int GetInstanceID();
            void ReloadSettings();
            int GetSettingInt(const std::string& key, const std::string& section, int fallback);
            float GetSettingFloat(const std::string& key, const std::string& section, float fallback);
            bool GetSettingBool(const std::string& key, const std::string& section, bool fallback);
            std::string GetSettingString(const std::string& key, const std::string& section, const std::string& fallback);
            void SetSettingInt(const std::string& key, const std::string& section, int value);
            void SetSettingFloat(const std::string& key, const std::string& section, float value);
            void SetSettingBool(const std::string& key, const std::string& section, bool value);
            void SetSettingString(const std::string& key, const std::string& section, const std::string& value);

            asIScriptEngine* mScriptEngine = nullptr;
            asIScriptContext* mContext = nullptr;
            asIScriptModule* mScriptModule = nullptr;

            asIScriptFunction* mUpdateFunc = nullptr;
            asIScriptContext*  mUpdateCtx  = nullptr;   
            std::string mLastError;

            VFS::VFS& mVFS;
            CE::GameInfo& mGameInfo;
            Settings::SettingsManager& mSettingsManager;
            Instance& mInstance;
            Renderer::IRenderer& mRenderer;
            Assets::Textures::TextureManager& mTextureManager;
            Assets::Fonts::FontManager& mFontManager;
            Input::Keyboard& mKeyboard;
            Input::Mouse& mMouse;
    };
}

namespace CE::Scripting::Utils {
    std::string LoadScript(VFS::VFS& vfs, const char* path);
}
