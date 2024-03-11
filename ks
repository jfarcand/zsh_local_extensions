#!/bin/zsh

# Usage function
usage() {
    echo "Usage: $0 <secret-name> [-n <namespace>]"
    echo "Retrieve a secret value from Kubernetes."
    echo "  <secret-name>       The name of the secret."
    echo "  -n <namespace>      The namespace where the secret is located (default: production)."
    exit 1
}

# Check for required dependencies
if ! command -v kubectl &> /dev/null || ! command -v jq &> /dev/null; then
    echo "Error: This script requires kubectl and jq to be installed."
    exit 1
fi

# Set default namespace
namespace="production"

# Parse command line arguments
while getopts ":n:" opt; do
    case ${opt} in
        n ) namespace=$OPTARG ;;
        \? ) usage ;;
    esac
done
shift $((OPTIND -1))

# Check if secret name was provided
if [ $# -eq 0 ]; then
    usage
fi
secret_name=$1

# List available secrets in the namespace
echo "Available secrets in namespace '$namespace':"
kubectl get secrets -n "$namespace"

# Retrieve and decode the secret value
echo "Retrieving secret '$secret_name' from namespace '$namespace':"
kubectl get secret "$secret_name" -n "$namespace" -o json | jq -r '.data | map_values(@base64d)'

