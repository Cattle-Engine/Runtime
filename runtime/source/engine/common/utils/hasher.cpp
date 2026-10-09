#include "engine/common/utils/hasher.hpp"

#include <iomanip>
#include <memory>
#include <sstream>

#include <xxhash.h>

namespace CE::Utils {
    // pimpl because I don't want to leak xxhash everywhere
    class StreamingHasher::Impl {
      public:
        XXH3_state_t* mState;
    };

    // Streaming hasher functions
    StreamingHasher::StreamingHasher() {
        mImpl = std::make_unique<Impl>();
        mImpl->mState = XXH3_createState();
        XXH3_64bits_reset(mImpl->mState);
    }

    StreamingHasher::~StreamingHasher() {
        XXH3_freeState(mImpl->mState);
    }

    void StreamingHasher::AddData(const void* data, size_t size) {
        XXH3_64bits_update(mImpl->mState, data, size);
    }

    void StreamingHasher::AddString(const std::string& s) {
        uint32_t len = s.size();
        AddData(&len, sizeof(len));
        AddData(s.data(), s.size());
    }

    uint64_t StreamingHasher::Finalize() {
        return XXH3_64bits_digest(mImpl->mState);
    }

    // standalone functions
    uint64_t Hash64(const void* data, std::size_t size) {
        return XXH3_64bits(data, size);
    }

    uint64_t Hash64(const std::string& string) {
        return XXH3_64bits(string.c_str(), string.size());
    }

    std::string Hash2String(uint64_t hash) {
        std::stringstream ss;

        ss << std::hex << std::setfill('0') << std::setw(0) << hash;

        return ss.str();
    }
} // namespace CE::Utils
