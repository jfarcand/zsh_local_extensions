#!/usr/bin/env bash

# Function to display usage information
usage() {
    echo "Usage: $0 -t <title> -b <message_body> [-f]"
    echo "  -t: Issue title"
    echo "  -b: Issue body"
    echo "  -f: Force push without confirmation"
    exit 1
}

# Parse arguments for title, message body, and force flag
force_push=0
while getopts ":t:b:f" opt; do
    case ${opt} in
        t )
            title=$OPTARG
            ;;
        b )
            body=$OPTARG
            ;;
        f )
            force_push=1
            ;;
        \? )
            usage
            ;;
    esac
done

# Check if both title and body are provided
if [ -z "$title" ] || [ -z "$body" ]; then
    usage
fi

# Check if there are changes to be committed
if [ -z "$(git status --porcelain)" ]; then
    echo "No changes to commit. Aborting script."
    exit 0
fi

# Create the GitHub issue using the `gh` CLI
issue_url=$(gh issue create --title "$title" --body "$body")

echo "Issue created: $issue_url"

# Check if the issue_url is defined
if [ -z "$issue_url" ]; then
    echo "Error: Issue creation failed."
    exit 1
fi

# Extract the issue number from the URL
issue_number=$(echo $issue_url | grep -oE '[0-9]+$')

# Check if the issue_number is defined
if [ -z "$issue_number" ]; then
    echo "Error: Failed to extract issue number from URL."
    exit 1
fi

# Echo the issue URL
echo "Issue number: $issue_number"

# Ask for confirmation before continuing
if [ $force_push -eq 0 ]; then
    read -p "Continue with git operations? (y/n) " confirm
    if [[ $confirm != [yY] ]]; then
        echo "Aborting git operations."
        exit 0
    fi
fi

# Perform Git operations
git add *
git commit -m "Fixes #$issue_number"
git push

