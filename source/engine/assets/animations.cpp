#include "engine/assets/animations.hpp"
#include "engine/common/fs/tdf.hpp"
#include "engine/common/tracelog.hpp"

namespace CE::Assets::Animations {
    AnimationManager::AnimationManager(VFS::VFS& vfs, Renderer::IRenderer& renderer, int instance_id)
    : mVFS(vfs), mRenderer(renderer) {
        mInstanceID = instance_id;
    }

    void AnimationManager::Load(std::string name, std::string path) {
        if (!mVFS.FileExists(path.c_str())) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing animation info file: {}",
                mInstanceID, path);
            return;
        }

        TDFFile info;
        info.load(mVFS, path);

        if (!info.has("SourceImagePath")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing source image path key in: {}",
                mInstanceID, path);
            return;
        }

        std::string source_image_path =
            TDFFile::readString(info.entries["SourceImagePath"]);

        if (!mVFS.FileExists(source_image_path.c_str())) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing source image file: {}",
                mInstanceID, source_image_path);
            return;
        }

        if (!info.has("FrameCount")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing frame count key in: {}",
                mInstanceID, path);
            return;
        }

        uint32_t frame_count =
            TDFFile::readUInt(info.entries["FrameCount"]);

        if (!info.has("Frames")) {
            CE::Log(LogLevel::Error,
                "[Animation Manager {}] Missing Frames array in: {}",
                mInstanceID, path);
            return;
        }

        std::vector<TDFFile> frames =
            TDFFile::readObjectArray(info.entries["Frames"]);

        AnimationInfo anim;
        anim.mSourceFileName = source_image_path;
        anim.FramesInfo.reserve(frame_count);

        for (const TDFFile& f : frames) {
            FrameInfo frame{};

            if (f.has("Width"))    frame.Width    = TDFFile::readUInt(f.entries.at("Width"));
            if (f.has("Height"))   frame.Height   = TDFFile::readUInt(f.entries.at("Height"));
            if (f.has("X"))        frame.X        = TDFFile::readUInt(f.entries.at("X"));
            if (f.has("Y"))        frame.Y        = TDFFile::readUInt(f.entries.at("Y"));
            if (f.has("Duration")) frame.Duration  = TDFFile::readUInt(f.entries.at("Duration"));

            anim.FramesInfo.push_back(frame);
        }

        mAnimations[name] = std::move(anim);
    }

    uint32_t AnimationManager::CreateInstance(std::string name, bool loop) {
        auto it = mAnimations.find(name);

        if(it != mAnimations.end()) {
            AnimationInstance instance = {};
            instance.Loop = loop;
            instance.IsPlaying = false;
            instance.CurrentFrame = 0;
            instance.X = 0;
            instance.Y = 0;
            instance.Rotation = 0.0f;
            instance.AnimInfo = &it->second;
            mAnimationInstances[mNextHandleID++] = std::move(instance);
            return mNextHandleID;
        }
        CE::Log(LogLevel::Error, "[Animation Manager {}] Tried using an unloaded or missing animation: {}", 
        mInstanceID, name);
        return 0;
    }
}