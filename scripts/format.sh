#!/bin/bash

# Set the directory to search for files.
src_dir="src"

# Find all .cpp and .hpp files under the src directory.
find "$src_dir" -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 | while IFS= read -r -d $'\0' file; do
  # Check if clang-format is installed.
  if ! command -v clang-format &> /dev/null
  then
    echo "clang-format could not be found. Please install it."
    exit 1
  fi

  # Format the file using clang-format.
  echo "Formatting: $file"
  clang-format -i "$file"
done

echo "Formatting complete."
