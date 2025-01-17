#!/bin/bash

# Script to find files/directories excluding hidden directories, node_modules, ios, and android

find . \
  \( -path '*/.*' -o -path '*asset*' -o -path '*/coverage*' -o -path '*/node_modules*' -o -path '*/ios*' -o -path '*/android*' \) -prune -o -print
