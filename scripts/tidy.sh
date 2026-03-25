# tidy.sh
#!/bin/bash
CXXFLAGS="-Wall -Wextra -std=c++20 -Iinclude -DENGINE_BUILT_ON_OS=\"$(uname -s)\""

for file in $(find source -type f \( -name '*.cpp' -o -name '*.c' \)); do
    clang-tidy "$file" -fix -- \
        $CXXFLAGS
done