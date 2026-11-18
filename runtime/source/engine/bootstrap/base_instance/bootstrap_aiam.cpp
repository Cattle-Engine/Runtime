#include "ce_runtime/instance/base_instance.hpp"

namespace CE {
    int BaseInstance::Bootstrap_AssetImportersAndManagers() {
        m3DModelImporter = std::make_unique<Assets::Model3DImporter::ModelImporter>(
            *mVFS, *mGPUMeshManager, *mMaterialManager, *mTextureManager, *mRenderer);
        return 0;
    }
} // namespace CE