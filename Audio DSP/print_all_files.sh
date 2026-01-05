#!/bin/bash

# Usage: ./list_files.sh [directory_path]
# If no path is provided, it defaults to the current directory.

TARGET_DIR=${1:-.}

find "$TARGET_DIR" -not -path '*/.*' | sed -e "s/[^-][^\/]*\// |/g" -e "s/|/ |--/g"