#!/bin/bash

# Enable logging
LOG_FILE="backup_postgres.log"
exec > >(tee -a $LOG_FILE) 2>&1

echo "$(date) - Script started"

# Load environment variables from .envrc
if [ -f .envrc ]; then
  export $(grep -v '^#' .envrc | xargs)
  echo "$(date) - .envrc file loaded"
else
  echo "$(date) - Error: .envrc file not found!"
  exit 1
fi

# Use environment variables directly
if [ -z "$POSTGRES_USER" ] || [ -z "$POSTGRES_PASSWORD" ] || [ -z "$POSTGRES_DB" ]; then
  echo "$(date) - Error: Missing required environment variables (POSTGRES_USER, POSTGRES_PASSWORD, POSTGRES_DB)!"
  exit 1
fi

# Find the running PostgreSQL container ID
CONTAINER_ID=$(docker ps | grep postgres | awk '{print $1}')
if [ -z "$CONTAINER_ID" ]; then
  echo "$(date) - Error: No running PostgreSQL container found!"
  exit 1
fi

echo "$(date) - Found PostgreSQL container ID: $CONTAINER_ID"

# Create a backup
BACKUP_FILE="fyri_backup_$(date +%Y%m%d_%H%M%S).sql"
echo "$(date) - Starting backup to $BACKUP_FILE"
PGPASSWORD=$POSTGRES_PASSWORD docker exec $CONTAINER_ID \
  pg_dump -U $POSTGRES_USER -d $POSTGRES_DB > $BACKUP_FILE

if [ $? -eq 0 ]; then
  echo "$(date) - Backup successful: $BACKUP_FILE"
else
  echo "$(date) - Error: Backup failed!"
  exit 1
fi

echo "$(date) - Script completed"

