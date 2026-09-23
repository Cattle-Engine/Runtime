#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Core::Audio {
    AudioClip* AudioSystem::LoadSound(const std::string& path, AudioType type) {
        AudioClip* clip = new AudioClip;

        clip->Path = path;
        if (!mVFS.FileExists(path.c_str())) {
            CE_LOG(LogLevel::Error, "[Audio {}] Audio file does not exist: {}", mInstanceID, path);
            delete clip;
            return nullptr;
        }

        auto file = mVFS.OpenFile(path);
        if (!file) {
            delete clip;
            return nullptr;
        }
        clip->SourceBytes.resize(file->Size());
        if (!clip->SourceBytes.empty() && !file->Read(clip->SourceBytes.data(), clip->SourceBytes.size())) {
            delete clip;
            return nullptr;
        }
        SDL_IOStream* stream = SDL_IOFromConstMem(clip->SourceBytes.data(), clip->SourceBytes.size());
        if (!stream) {
            delete clip;
            return nullptr;
        }
        clip->Audio = MIX_LoadAudio_IO(mMixer, stream, true, false);
        if (!clip->Audio) {
            CE_LOG(LogLevel::Error, "[Audio {}] Failed to load audio file: {}", mInstanceID, path);
            delete clip;
            return nullptr;
        }

        clip->IsError = false;
        clip->IsLoaded = true;
        clip->Type = type;
        return clip;
    }

    void AudioSystem::DestroySound(AudioClip* clip) {
        if (clip == nullptr) {
            CE_LOG(LogLevel::Error, "[Audio {}] Audio clip was null!", mInstanceID);
            return;
        }
        if (clip->Audio) {
            MIX_DestroyAudio(clip->Audio);
            clip->Audio = nullptr;
        }
        delete clip;
    }
} // namespace CE::Core::Audio
