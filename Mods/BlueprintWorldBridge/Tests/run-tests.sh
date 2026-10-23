#!/usr/bin/env bash
set -euo pipefail

test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source_dir="$test_dir/../Source/BlueprintWorldBridge/Public"
output="$(mktemp "${TMPDIR:-/tmp}/blueprint-world-names.XXXXXX")"
trap 'rm -f "$output"' EXIT

"${CXX:-c++}" -std=c++20 -Wall -Wextra -Werror -pedantic \
  -I "$source_dir" "$test_dir/BlueprintWorldNamesTest.cpp" -o "$output"
"$output"
