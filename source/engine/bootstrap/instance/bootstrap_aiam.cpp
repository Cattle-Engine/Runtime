#include "engine/instance.hpp"

namespace CE {
    int Instance::Bootstrap_AssetImportersAndManagers() {
        g3DModelImporter = std::make_unique<Assets::Model3DImporter::ModelImporter>(*gVFS, *gGPUMeshManager, *gMaterialManager, *gTextureManager, *gRenderer);
        return 0;
    }
}