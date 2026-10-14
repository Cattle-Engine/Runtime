#pragma once

#include <string>
#include <filesystem>

#include "ce_common/as_compiler/ast.hpp"

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
     *
     * @return Returns the path as a std::string, if failed it shall return an empty std::string
     */
    std::string Import2Path(const AST::ASTImport& import, const std::vector<std::filesystem::path>& import_paths);

    /**
     * @brief Loads a script file from disk (using std::filesystem) and returns it as a string
     *
     * @param path The path to the file to read
     *
     * @return Returns a std::string
     */
    std::string GetScriptFromFS(const std::string& path);
} // namespace CE::Scripting::Impl::Common
