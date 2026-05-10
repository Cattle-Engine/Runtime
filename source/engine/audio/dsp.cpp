#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

#include <algorithm>
#include <cmath>

namespace CE::Core::Audio {
    namespace {
        constexpr float kTwoPi = 6.2831853071795864769f;

        static void ResetFilterState(AudioFilter& filter, int sample_rate, int channels) {
            filter.SampleRate = sample_rate;
            filter.Channels = channels;
            filter.PrevIn.assign(static_cast<size_t>(channels), 0.0f);
            filter.PrevOut.assign(static_cast<size_t>(channels), 0.0f);
        }

        static void UpdateAlpha(AudioFilter& filter, int sample_rate) {
            const float sr = static_cast<float>(sample_rate);
            const float nyquist = sr * 0.5f;
            const float cutoff = std::clamp(filter.CutoffHz, 10.0f, std::max(10.0f, nyquist - 10.0f));

            const float rc = 1.0f / (kTwoPi * cutoff);
            const float dt = 1.0f / sr;

            if (filter.Type == AudioFilter::Type::LowPass) {
                filter.Alpha = dt / (rc + dt);
            } else {
                filter.Alpha = rc / (rc + dt);
            }
        }

        static void ApplyOnePole(AudioFilter& filter, const SDL_AudioSpec* spec, float* pcm, int samples) {
            if (!filter.Enabled || !spec || !pcm || samples <= 0) {
                return;
            }

            const int channels = std::max(1, spec->channels);
            const int sample_rate = std::max(1, spec->freq);

            if (filter.SampleRate != sample_rate || filter.Channels != channels ||
                static_cast<int>(filter.PrevOut.size()) != channels ||
                static_cast<int>(filter.PrevIn.size()) != channels) {
                ResetFilterState(filter, sample_rate, channels);
            }

            UpdateAlpha(filter, sample_rate);

            const int frames = samples / channels;
            if (frames <= 0) {
                return;
            }

            float* cursor = pcm;
            if (filter.Type == AudioFilter::Type::LowPass) {
                for (int i = 0; i < frames; ++i) {
                    for (int ch = 0; ch < channels; ++ch) {
                        const float x = *cursor;
                        const float y = filter.PrevOut[static_cast<size_t>(ch)] + filter.Alpha * (x - filter.PrevOut[static_cast<size_t>(ch)]);
                        filter.PrevOut[static_cast<size_t>(ch)] = y;
                        *cursor++ = y;
                    }
                }
            } else { // HighPass
                for (int i = 0; i < frames; ++i) {
                    for (int ch = 0; ch < channels; ++ch) {
                        const float x = *cursor;
                        const float y = filter.Alpha * (filter.PrevOut[static_cast<size_t>(ch)] + x - filter.PrevIn[static_cast<size_t>(ch)]);
                        filter.PrevIn[static_cast<size_t>(ch)] = x;
                        filter.PrevOut[static_cast<size_t>(ch)] = y;
                        *cursor++ = y;
                    }
                }
            }
        }

    }

    void SDLCALL AudioSystem::TrackCookedDSP(void* userdata, MIX_Track* track, const SDL_AudioSpec* spec, float* pcm, int samples) {
        auto* self = static_cast<AudioSystem*>(userdata);
        if (!self || !track) {
            return;
        }

        std::lock_guard<std::mutex> lock(self->mDSPMutex);
        auto it = self->mTrackFilters.find(track);
        if (it == self->mTrackFilters.end()) {
            return;
        }

        ApplyOnePole(it->second, spec, pcm, samples);
    }

    void AudioSystem::ApplyDSP(PlayingSound& sound) {
        if (!sound.Track) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            mTrackFilters[sound.Track] = sound.Filter;
        }

        if (!MIX_SetTrackCookedCallback(sound.Track, AudioSystem::TrackCookedDSP, this)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to set DSP callback: {}", mInstanceID, SDL_GetError());
        }
    }
}
