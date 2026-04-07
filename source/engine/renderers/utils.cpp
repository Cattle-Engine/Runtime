#include "engine/renderer.hpp"

namespace CE::Renderer::Utils {
    void MakeIdentity(float* m) {
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    void MakeTranslate(float* m, float x, float y) {
        MakeIdentity(m);
        m[3] = x;
        m[7] = y;
    }
}