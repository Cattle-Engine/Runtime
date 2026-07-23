#pragma once

#include <string>

#include "engine/scripting/private/ast.hpp"

#include <engine/common/fs/vfs.hpp>

// This header contains shared code used by other internal systems
namespace CE::Scripting::Impl::Common {
    /**
     * @brief Turns an ASTImport into a std::string path
     *
     * When you do ```import foo;``` this shall check:
     *
     * A: Is it a normal .ceas file
     * B: Check if its a directory with a module.ceas
     *
     * @param import The ASTImport to make into a path
     * @param vfs This may seem unnecessary, but it is used to check if files exist etc
     *
     * @return Returns the path as a std::string, if failed it shall return an empty std::string
     */
    std::string Import2Path(const AST::ASTImport& import, VFS::VFS& vfs);

    /**
     * @brief Loads a script file from the VFS and returns it as a string
     *
     * @param path The path to the file to read
     * @param vfs The VFS to use
     *
     * @return Returns a std::string
     */
    std::string GetScriptFromVFS(const std::string& path, VFS::VFS& vfs);
} // namespace CE::Scripting::Impl::Common