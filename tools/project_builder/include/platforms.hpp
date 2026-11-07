#pragma once

#include <string>

namespace Platforms {
    bool AddDirToPath(std::string path);
    bool RemoveDirFromPath(std::string path);
    bool IsDirInPath(std::string path);
    
    std::string GetExecutablePath();
}