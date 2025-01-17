#!/bin/bash

# Get last 20 commits and filter for scripts/ folder
for hash in $(git log --pretty=format:"%H" -n 20); do
 echo "Commit: $hash"
 git show --name-only $hash | grep "scripts/"
 echo "---"
done
