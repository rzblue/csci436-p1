#!/usr/bin/env bash

OUTPUT="full_source_listing.txt"
FILES=(
  "include/BaseServer.hpp"
  "include/HTTPProxyServer.hpp"
  "include/Logger.hpp"
  "src/BaseServer.cpp"
  "src/HTTPProxyServer.cpp"
  "src/Logger.cpp"
  "src/main.cpp"
)

echo "Generating $OUTPUT..."

# Truncate existing file if present
> "$OUTPUT"

for f in "${FILES[@]}"; do
    if [[ -f "$f" ]]; then
        echo "===== FILE: $f =====" >> "$OUTPUT"
        cat "$f" >> "$OUTPUT"
        echo -e "\n" >> "$OUTPUT"
    else
        echo "WARNING: Missing file $f" >&2
    fi
done

echo "✅ Done"
