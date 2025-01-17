#!/bin/bash

kill_port() {
    local port=$1
    if [ -z "$port" ]; then
        echo "Usage: $0 <port>"
        return 1
    fi

    # Find the PID of the process using the specified port
    local pid=$(lsof -t -i :$port)

    # Check if the PID exists and kill the process
    if [ -n "$pid" ]; then
        echo "Killing process using port $port (PID: $pid)"
        kill -9 $pid
    else
        echo "No process found using port $port"
    fi
}

# Call the function with the provided argument
kill_port "$1"


