#include "engine/audio/audio.hpp"
#include "engine/common/tracelog.hpp"

#include <algorithm>
#include <cmath>

namespace CE::Core::Audio {
    namespace {
        constexpr float kTwoPi = 6.2831853071795864769f;
        constexpr float kMaxDelayMs = 2000.0f;

        static float ClampMix(float value) {
            return std::clamp(value, 0.0f, 1.0f);
        }

        static void ResetFilterState(AudioFilter& filter, int sample_rate, int channels) {
            filter.SampleRate = sample_rate;
            filter.Channels = channels;
            filter.WriteFrame = 0;
            filter.LFOPhase = 0.0f;
            filter.PrevIn.assign(static_cast<size_t>(channels), 0.0f);
            filter.PrevOut.assign(static_cast<size_t>(channels), 0.0f);
            filter.DelayBuffer.clear();
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

        static size_t EnsureDelayBuffer(AudioFilter& filter, int sample_rate, int channels, float max_delay_ms) {
            const float clamped_ms = std::clamp(max_delay_ms, 1.0f, kMaxDelayMs);
            const int max_delay_frames = std::max(2, static_cast<int>(std::ceil((clamped_ms / 1000.0f) * static_cast<float>(sample_rate))) + 2);
            const size_t required_frames = static_cast<size_t>(max_delay_frames);
            const size_t required_samples = required_frames * static_cast<size_t>(channels);

            if (filter.SampleRate != sample_rate || filter.Channels != channels ||
                filter.DelayBuffer.size() != required_samples ||
                static_cast<int>(filter.PrevOut.size()) != channels ||
                static_cast<int>(filter.PrevIn.size()) != channels) {
                ResetFilterState(filter, sample_rate, channels);
                filter.SampleRate = sample_rate;
                filter.Channels = channels;
                filter.PrevIn.assign(static_cast<size_t>(channels), 0.0f);
                filter.PrevOut.assign(static_cast<size_t>(channels), 0.0f);
                filter.DelayBuffer.assign(required_samples, 0.0f);
            }

            return required_frames;
        }

        static float ReadDelayedSample(const AudioFilter& filter, size_t buffer_frames, int channels, int channel, float delay_frames) {
            if (buffer_frames == 0 || channels <= 0 || filter.DelayBuffer.empty()) {
                return 0.0f;
            }

            const float clamped_delay = std::clamp(delay_frames, 0.0f, static_cast<float>(buffer_frames - 1));
            const float read_pos = static_cast<float>(filter.WriteFrame) - clamped_delay;
            float wrapped_pos = std::fmod(read_pos, static_cast<float>(buffer_frames));
            if (wrapped_pos < 0.0f) {
                wrapped_pos += static_cast<float>(buffer_frames);
            }

            const size_t frame_a = static_cast<size_t>(wrapped_pos) % buffer_frames;
            const size_t frame_b = (frame_a + 1) % buffer_frames;
            const float fraction = wrapped_pos - std::floor(wrapped_pos);

            const size_t idx_a = frame_a * static_cast<size_t>(channels) + static_cast<size_t>(channel);
            const size_t idx_b = frame_b * static_cast<size_t>(channels) + static_cast<size_t>(channel);
            const float sample_a = filter.DelayBuffer[idx_a];
            const float sample_b = filter.DelayBuffer[idx_b];
            return sample_a + ((sample_b - sample_a) * fraction);
        }

        static void WriteDelaySample(AudioFilter& filter, size_t buffer_frames, int channels, int channel, float sample) {
            if (buffer_frames == 0 || channels <= 0 || filter.DelayBuffer.empty()) {
                return;
            }

            const size_t idx = (filter.WriteFrame % buffer_frames) * static_cast<size_t>(channels) + static_cast<size_t>(channel);
            filter.DelayBuffer[idx] = sample;
        }

        static void AdvanceDelayFrame(AudioFilter& filter, size_t buffer_frames) {
            if (buffer_frames == 0) {
                return;
            }
            filter.WriteFrame = (filter.WriteFrame + 1) % buffer_frames;
        }

        static void ApplyDelay(AudioFilter& filter, const SDL_AudioSpec* spec, float* pcm, int samples) {
            if (!filter.Enabled || !spec || !pcm || samples <= 0) {
                return;
            }

            const int channels = std::max(1, spec->channels);
            const int sample_rate = std::max(1, spec->freq);
            const int frames = samples / channels;
            if (frames <= 0) {
                return;
            }

            const float wet = ClampMix(filter.WetMix);
            const float feedback = ClampMix(filter.Feedback);
            const float delay_ms = std::clamp(filter.DelayMs, 1.0f, kMaxDelayMs);
            const size_t buffer_frames = EnsureDelayBuffer(filter, sample_rate, channels, delay_ms);
            const float delay_frames = std::max(1.0f, (delay_ms / 1000.0f) * static_cast<float>(sample_rate));

            float* cursor = pcm;
            for (int frame = 0; frame < frames; ++frame) {
                for (int ch = 0; ch < channels; ++ch) {
                    const float dry = *cursor;
                    const float delayed = ReadDelayedSample(filter, buffer_frames, channels, ch, delay_frames);
                    const float output = dry + ((delayed - dry) * wet);
                    WriteDelaySample(filter, buffer_frames, channels, ch, dry + (delayed * feedback));
                    *cursor++ = output;
                }
                AdvanceDelayFrame(filter, buffer_frames);
            }
        }

        static void ApplyChorus(AudioFilter& filter, const SDL_AudioSpec* spec, float* pcm, int samples) {
            if (!filter.Enabled || !spec || !pcm || samples <= 0) {
                return;
            }

            const int channels = std::max(1, spec->channels);
            const int sample_rate = std::max(1, spec->freq);
            const int frames = samples / channels;
            if (frames <= 0) {
                return;
            }

            const float wet = ClampMix(filter.WetMix);
            const float base_delay_ms = std::clamp(filter.DelayMs, 5.0f, 60.0f);
            const float depth_ms = std::clamp(filter.DepthMs, 0.1f, 20.0f);
            const float rate_hz = std::clamp(filter.RateHz, 0.01f, 8.0f);
            const size_t buffer_frames = EnsureDelayBuffer(filter, sample_rate, channels, base_delay_ms + depth_ms + 4.0f);

            float* cursor = pcm;
            for (int frame = 0; frame < frames; ++frame) {
                const float modulation = (std::sin(filter.LFOPhase) * 0.5f) + 0.5f;
                const float delay_ms = base_delay_ms + (depth_ms * modulation);
                const float delay_frames = std::max(1.0f, (delay_ms / 1000.0f) * static_cast<float>(sample_rate));

                for (int ch = 0; ch < channels; ++ch) {
                    const float dry = *cursor;
                    const float delayed = ReadDelayedSample(filter, buffer_frames, channels, ch, delay_frames);
                    WriteDelaySample(filter, buffer_frames, channels, ch, dry);
                    *cursor++ = dry + ((delayed - dry) * wet);
                }

                filter.LFOPhase += kTwoPi * rate_hz / static_cast<float>(sample_rate);
                if (filter.LFOPhase > kTwoPi) {
                    filter.LFOPhase = std::fmod(filter.LFOPhase, kTwoPi);
                }
                AdvanceDelayFrame(filter, buffer_frames);
            }
        }

        static void ApplyReverb(AudioFilter& filter, const SDL_AudioSpec* spec, float* pcm, int samples) {
            if (!filter.Enabled || !spec || !pcm || samples <= 0) {
                return;
            }

            const int channels = std::max(1, spec->channels);
            const int sample_rate = std::max(1, spec->freq);
            const int frames = samples / channels;
            if (frames <= 0) {
                return;
            }

            const float wet = ClampMix(filter.WetMix);
            const float room_size = ClampMix(filter.RoomSize);
            const float damping = ClampMix(filter.Damping);
            const float base_ms = 25.0f + (room_size * 70.0f);
            const size_t buffer_frames = EnsureDelayBuffer(filter, sample_rate, channels, base_ms * 3.5f);
            const float tap_a = std::max(1.0f, (base_ms / 1000.0f) * static_cast<float>(sample_rate));
            const float tap_b = std::max(1.0f, ((base_ms * 1.7f) / 1000.0f) * static_cast<float>(sample_rate));
            const float tap_c = std::max(1.0f, ((base_ms * 2.3f) / 1000.0f) * static_cast<float>(sample_rate));

            float* cursor = pcm;
            for (int frame = 0; frame < frames; ++frame) {
                for (int ch = 0; ch < channels; ++ch) {
                    const float dry = *cursor;
                    const float delayed_a = ReadDelayedSample(filter, buffer_frames, channels, ch, tap_a);
                    const float delayed_b = ReadDelayedSample(filter, buffer_frames, channels, ch, tap_b);
                    const float delayed_c = ReadDelayedSample(filter, buffer_frames, channels, ch, tap_c);
                    const float reverb = (delayed_a * 0.5f) + (delayed_b * 0.3f) + (delayed_c * 0.2f);
                    const float damped = (filter.PrevOut[static_cast<size_t>(ch)] * damping) + (reverb * (1.0f - damping));
                    filter.PrevOut[static_cast<size_t>(ch)] = damped;
                    WriteDelaySample(filter, buffer_frames, channels, ch, dry + (damped * room_size));
                    *cursor++ = dry + ((damped - dry) * wet);
                }
                AdvanceDelayFrame(filter, buffer_frames);
            }
        }

        static void ApplyEffect(AudioFilter& filter, const SDL_AudioSpec* spec, float* pcm, int samples) {
            switch (filter.Type) {
                case AudioFilter::Type::LowPass:
                case AudioFilter::Type::HighPass:
                    ApplyOnePole(filter, spec, pcm, samples);
                    break;
                case AudioFilter::Type::Reverb:
                    ApplyReverb(filter, spec, pcm, samples);
                    break;
                case AudioFilter::Type::Delay:
                    ApplyDelay(filter, spec, pcm, samples);
                    break;
                case AudioFilter::Type::Chorus:
                    ApplyChorus(filter, spec, pcm, samples);
                    break;
            }
        }
    }

    void SDLCALL AudioSystem::TrackCookedDSP(void* userdata, MIX_Track* track, const SDL_AudioSpec* spec, float* pcm, int samples) {
        auto* self = static_cast<AudioSystem*>(userdata);
        if (!self || !track) {
            return;
        }

        std::lock_guard<std::mutex> lock(self->mDSPMutex);
        auto it = self->mTrackStates.find(track);
        if (it == self->mTrackStates.end()) {
            return;
        }

        for (AudioFilter& effect : it->second.Effects) {
            ApplyEffect(effect, spec, pcm, samples);
        }
    }

    void AudioSystem::ApplyDSP(PlayingSound& sound) {
        if (!sound.Track) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mDSPMutex);
            TrackState& state = mTrackStates[sound.Track];
            state.Type = sound.Clip ? sound.Clip->Type : AudioType::Error;
            state.Volume = sound.Volume;
            state.Effects = BuildEffectChain(sound);
        }

        if (!MIX_SetTrackCookedCallback(sound.Track, AudioSystem::TrackCookedDSP, this)) {
            CE::Log(LogLevel::Warn, "[Audio {}] Failed to set DSP callback: {}", mInstanceID, SDL_GetError());
        }
    }

    std::vector<AudioFilter> AudioSystem::BuildEffectChain(const PlayingSound& sound) const {
        std::vector<AudioFilter> effects;
        if (sound.Filter.Enabled) {
            effects.push_back(sound.Filter);
        }

        effects.reserve(effects.size() + sound.Effects.size());
        for (const AudioFilter& effect : sound.Effects) {
            if (effect.Enabled) {
                effects.push_back(effect);
            }
        }
        return effects;
    }
}
