int gOnDrawId = -1;
int gOnDraw3DId = -1;
int gFrames = 0;
float gAngle = 0.0f;
bool gDidInit = false;
uint gBubbleAnim = 0;
uint gCubeHandle = 0;
CE::Graphics::MeshData@ gCubeMesh = null;
CE::Graphics::ThreeD::Camera3D gCamera3D;
bool gCameraReady = false;

CE::Graphics::Colour MakeColour(uint8 r, uint8 g, uint8 b, uint8 a) {
    CE::Graphics::Colour c(r, g, b, a);
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

CE::Graphics::ThreeD::Vec3 MakeVec3(float x, float y, float z) {
    CE::Graphics::ThreeD::Vec3 v(x, y, z);
    return v;
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

    if (gBubbleAnim != 0) {
        CE::Graphics::Text::DrawTextCol(
            "Animation test: assets/output.tdf (handle=" + gBubbleAnim + ")",
            baseX,
            infoY + 60,
            18.0f,
            MakeColour(0, 0, 0, 255)
        );
    } else {
        CE::Graphics::Text::DrawTextCol(
            "Animation test: failed to create instance (handle=0)",
            baseX,
            infoY + 60,
            18.0f,
            MakeColour(200, 0, 0, 255)
        );
    }
}

void InGameDraw3D(const string &in state, const string &in eventName) {
    if (gCubeHandle == 0) {
        return;
    }

    if (!gCameraReady) {
        gCamera3D.position = MakeVec3(0.0f, 1.5f, 4.0f);
        gCamera3D.target = MakeVec3(0.0f, 0.5f, 0.0f);
        gCamera3D.up = MakeVec3(0.0f, 1.0f, 0.0f);
        gCamera3D.fov = 1.0471976f;
        gCamera3D.nearClip = 0.05f;
        gCamera3D.farClip = 128.0f;
        gCamera3D.projection = CE::Graphics::ThreeD::CameraProjection::Perspective;
        CE::Graphics::ThreeD::SetCamera3D(gCamera3D);
        CE::Graphics::ThreeD::SetSunEnabled(true);
        CE::Graphics::ThreeD::SetSunDirection(MakeVec3(-0.4f, -1.0f, -0.3f));
        CE::Graphics::ThreeD::SetAmbientLight(MakeVec3(1.0f, 0.96f, 0.92f), 0.25f);
        gCameraReady = true;
    }

    CE::Graphics::ThreeD::Transform3D cubeTransform;
    cubeTransform.position = MakeVec3(0.0f, 0.0f, 0.0f);
    cubeTransform.rotation = MakeVec3(gAngle * 0.4f, gAngle, 0.0f);
    cubeTransform.scale = MakeVec3(1.0f, 1.0f, 1.0f);

    CE::Graphics::ThreeD::DrawMesh(gCubeHandle, cubeTransform, "demo_material", true);
}

void main() {
    CE::Graphics::Textures::LoadTexture("tato.webp", "garry_spud");
    CE::Graphics::Text::LoadFont("Roboto.ttf", "roboto", 32);

    CE::Graphics::Animations::LoadAnimation("output.tdf", "bubble");
    gBubbleAnim = CE::Graphics::Animations::CreateInstance("bubble");
    if (gBubbleAnim != 0) {
        CE::Graphics::Animations::SetTint(gBubbleAnim, MakeColour(255, 255, 255, 255));
        CE::Graphics::Animations::Play(gBubbleAnim, 1020, 520, true, true);
    }

    CE::Graphics::ThreeD::LoadMaterial("demo_material");
    CE::Graphics::ThreeD::SetMaterialTint("demo_material", MakeColour(255, 255, 255, 255));
    CE::Graphics::ThreeD::SetMaterialRoughness("demo_material", 0.35f);
    CE::Graphics::ThreeD::SetMaterialMetallic("demo_material", 0.05f);

    @gCubeMesh = CE::Graphics::CreateCube(1.0f, 1.0f, 1.0f, MakeColour(255, 120, 80, 255));
    if (gCubeMesh !is null) {
        gCubeHandle = CE::Graphics::ThreeD::CreateMeshHandle(gCubeMesh);
    }

    CE::Settings::SetSettingInt("test_int", "ScriptTest", 123);
    CE::Settings::SetSettingFloat("test_float", "ScriptTest", 1.25f);
    CE::Settings::SetSettingBool("test_bool", "ScriptTest", true);
    CE::Settings::SetSettingString("test_string", "ScriptTest", "hello from script");
    CE::Settings::ReloadSettings();

    gOnDrawId = CE::Events::On("InGame", "Draw2D", @InGameDraw);
    gOnDraw3DId = CE::Events::On("InGame", "Draw3D", @InGameDraw3D);

    CE::State::Set("InGame");

    CE::Audio::LoadSound(
        "test.mp3",
        "test",
        CE::Audio::Music
    );
    uint test = CE::Audio::CreateInstance("test");
    CE::Audio::AudioEffect reverb;
    reverb.enabled = true;
    reverb.type = CE::Audio::Reverb;
    reverb.wetMix = 0.45f;    
    reverb.roomSize = 0.85f;  
    reverb.damping = 0.25f;   
    CE::Audio::AddEffect(
        test,
        "test",
        reverb
    );
    CE::Audio::Play(test);
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
        CE::Exit();
    }
}
