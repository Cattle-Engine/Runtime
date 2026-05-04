int gOnDrawId = -1;
int gFrames = 0;
float gAngle = 0.0f;
bool gDidInit = false;

CE::Graphics::Colour MakeColour(uint8 r, uint8 g, uint8 b, uint8 a) {
    CE::Graphics::Colour c(r, g, b, a);
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

void InGameDraw(const string &in state, const string &in eventName) {
    float baseX = 40;
    float baseY = 40;
    float spacingX = 180;
    float spacingY = 140;

    float txY = baseY;

    CE::Graphics::Textures::DrawTexture("garry_spud", baseX + spacingX * 0, txY);
    CE::Graphics::Textures::DrawTextureEx("garry_spud", baseX + spacingX * 1, txY, MakeColour(255,255,255,200));
    CE::Graphics::Textures::DrawTextureRot("garry_spud", baseX + spacingX * 2, txY, gAngle);
    CE::Graphics::Textures::DrawTextureRotEx("garry_spud", baseX + spacingX * 3, txY, -gAngle, MakeColour(255,120,120,255));
    CE::Graphics::Textures::DrawTexturePro("garry_spud", baseX + spacingX * 4, txY, 128, 128, gAngle, MakeColour(120,255,120,255));

    float pxY = baseY + spacingY;

    CE::Graphics::Primitives::DrawRectangle(baseX, pxY, 140, 80, MakeColour(40,180,255,255));
    CE::Graphics::Primitives::DrawRectangle(baseX + spacingX, pxY, 140, 80, MakeColour(255,180,40,255), gAngle);

    CE::Graphics::Primitives::DrawCircle(baseX + 70, pxY + 120, 40, 32, MakeColour(200,40,255,255));
    CE::Graphics::Primitives::DrawCircleLines(baseX + spacingX + 70, pxY + 120, 40, 32, 3.0f, MakeColour(40,255,120,255));

    CE::Graphics::Primitives::DrawLine(baseX, pxY + 200, baseX + spacingX * 2, pxY + 200, 4.0f, MakeColour(255,255,255,255));

    CE::Graphics::Primitives::DrawTriangle(baseX + 40, pxY + 240, baseX + 120, pxY + 240, baseX + 80, pxY + 300, MakeColour(255,80,80,255));
    CE::Graphics::Primitives::DrawTriangle(baseX + spacingX + 40, pxY + 240, baseX + spacingX + 120, pxY + 240, baseX + spacingX + 80, pxY + 300, MakeColour(80,255,80,255), -gAngle);

    CE::Graphics::Primitives::DrawRectangleLines(baseX, pxY + 320, spacingX * 2, 90, 3.0f, MakeColour(0,0,0,255));

    CE::Graphics::Text::DrawText("AngelScript binding test", baseX, 10, 28.0f);

    float infoY = pxY + 430;

    CE::Graphics::Text::DrawTextCol(
        "State: " + state + " Event: " + eventName + " OnId: " + gOnDrawId,
        baseX,
        infoY,
        22.0f,
        MakeColour(0,0,0,255)
    );

    CE::Graphics::Text::DrawTextEx(
        "Roboto font (DrawTextEx)",
        "roboto",
        baseX,
        infoY + 30,
        22.0f,
        MakeColour(20,20,20,255)
    );
}

void main() {
    CE::Graphics::Textures::LoadTexture("tato.webp", "garry_spud");
    CE::Graphics::Text::LoadFont("Roboto.ttf", "roboto", 32);

    CE::Settings::SetSettingInt("test_int", "ScriptTest", 123);
    CE::Settings::SetSettingFloat("test_float", "ScriptTest", 1.25f);
    CE::Settings::SetSettingBool("test_bool", "ScriptTest", true);
    CE::Settings::SetSettingString("test_string", "ScriptTest", "hello from script");
    CE::Settings::ReloadSettings();

    gOnDrawId = CE::Events::On("InGame", "Draw", @InGameDraw);

    CE::State::Set("InGame");
}

void update() {
    gFrames++;

    float dt = CE::GetDeltaTime();
    float ft = CE::GetFrameTime();
    int fps = CE::GetFPS();
    int instanceId = CE::GetInstanceID();

    bool aDown = CE::Input::IsKeyDown(CE::Input::KEY_A);
    bool aPressed = CE::Input::IsKeyPressed(CE::Input::KEY_A);
    bool aReleased = CE::Input::IsKeyReleased(CE::Input::KEY_A);

    bool mDown = CE::Input::IsMouseButtonDown(CE::Input::LEFT);
    bool mPressed = CE::Input::IsMouseButtonPressed(CE::Input::LEFT);
    bool mReleased = CE::Input::IsMouseButtonReleased(CE::Input::LEFT);
    int mx = CE::Input::GetMouseX();
    int my = CE::Input::GetMouseY();
    int mdx = CE::Input::GetMouseDeltaX();
    int mdy = CE::Input::GetMouseDeltaY();
    int mwx = CE::Input::GetMouseWheelX();
    int mwy = CE::Input::GetMouseWheelY();

    int testInt = CE::Settings::GetSettingInt("test_int", "ScriptTest", -1);
    float testFloat = CE::Settings::GetSettingFloat("test_float", "ScriptTest", -1.0f);
    bool testBool = CE::Settings::GetSettingBool("test_bool", "ScriptTest", false);
    string testString = CE::Settings::GetSettingString("test_string", "ScriptTest", "<missing>");

    string state = CE::State::Get();

    gAngle += dt;

    CE::Graphics::Text::DrawTextCol("dt=" + dt + " ft=" + ft + " fps=" + fps + " id=" + instanceId, 20, 240, 18.0f, MakeColour(0, 0, 0, 255));
    CE::Graphics::Text::DrawTextCol("A: down=" + aDown + " pressed=" + aPressed + " released=" + aReleased, 20, 260, 18.0f, MakeColour(0, 0, 0, 255));
    CE::Graphics::Text::DrawTextCol("Mouse: d=" + mDown + " p=" + mPressed + " r=" + mReleased + " x=" + mx + " y=" + my, 20, 280, 18.0f, MakeColour(0, 0, 0, 255));
    CE::Graphics::Text::DrawTextCol("Mouse delta: x=" + mdx + " y=" + mdy + " wheel=" + mwx + "," + mwy, 20, 300, 18.0f, MakeColour(0, 0, 0, 255));
    CE::Graphics::Text::DrawTextCol("Settings: i=" + testInt + " f=" + testFloat + " b=" + testBool + " s=" + testString, 20, 320, 18.0f, MakeColour(0, 0, 0, 255));
    CE::Graphics::Text::DrawTextCol("State.Get()=" + state, 20, 340, 18.0f, MakeColour(0, 0, 0, 255));

    if (!gDidInit && gFrames > 10) {
        gDidInit = true;
    }

    if (CE::Input::IsKeyPressed(CE::Input::KEY_ESCAPE)) {
        CE::Graphics::Text::UnloadFont("roboto");
        CE::Graphics::Textures::UnloadTexture("garry_spud");
        CE::Exit();
    }
}