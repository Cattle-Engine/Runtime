#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_AssetImportersAndManagers() {
        g3DModelImporter = std::make_unique<Assets::Model3DImporter::ModelImporter>(
            *mVFS, *mGPUMeshManager, *mMaterialManager, *mTextureManager, *mRenderer);
        return 0;
    }
} // namespace CE