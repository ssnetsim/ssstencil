#!/bin/bash

set -e

# fetches the clang-format settings file
wget -q https://raw.githubusercontent.com/nicmcd/pkgbuild/master/clang-format -O .clang-format

# formats the code using clang-format
find . -type f \( -iname "*.h" -o -iname "*.cc" -o -iname "*.tcc" \) \
    | xargs clang-format -i --style=file --verbose
