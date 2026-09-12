CE::Texture gtexture;
CE::Audio::AudioAsset audio_asset;
CE::Audio::PlayingAudio bite_me;

void test(int x) {
}

void test(string s) {
}

void test(float f, string s) {
}

void main() {
    test(10);
    test("hello");
    test(3.14, "null");

    gtexture = CE::Graphics::Textures::LoadTexture("welcome.gif");
    audio_asset = CE::Audio::LoadSound("Siliconvalleysyrup.Temp.mp3", CE::Audio::AudioType::Music);
    bite_me = CE::Audio::CreatePlayingAudio(audio_asset);
    CE::Audio::PlaySound(bite_me);
}

void update() {
    // CE::Graphics::Text::DrawText(foo::global_vars::gTestString, 0, 0, 25);
    CE::Graphics::Render2D::DrawTexture(gtexture, 1, 1);
}
