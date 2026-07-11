#include <xxhash.h>
#include <sstream>
#include <iomanip>

#include "engine/common/utils/hasher.hpp"

namespace CE::Utils {
    // pimpl because I don't want to leak xxhash everywhere
    class StreamingHasher::Impl {
        public:
            XXH3_state_t* mState;
    };
    
    // Streaming hasher functions
    StreamingHasher::StreamingHasher() {
        mImpl = new Impl;
        mImpl->mState = XXH3_createState();
    }
    
    StreamingHasher::~StreamingHasher() {
        XXH3_freeState(mImpl->mState);
        delete mImpl;
    }
    
    void StreamingHasher::AddData(const void* data, size_t size) {
        XXH3_64bits_update(mImpl->mState, data, size);
    }
    
    void StreamingHasher::AddString(const std::string& s) {
        uint32_t len = s.size();
        AddData(&len, sizeof(len));
        AddData(s.data(), s.size());
    }
    
    void StreamingHasher::AddValue(auto&& value) {
        AddData(&value, sizeof(value));
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
}   
