#pragma once

#include <vector>
#include <string>

namespace CE::Scripting::Impl {
    class Preprocessor {
        public:
            struct Define {
                std::string name;
                std::string data;
            };

            bool Process(const std::string& in, std::string& out);
            bool AddDefine(Define define);
            bool RemoveDefine(const std::string& name);
        private:
            std::vector<Define> mDefines;
    };
}