#! /bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: ./generate_input_files.sh file_name no_MB"

else
    base64 /dev/urandom | head -c $(($2 * 1000000)) > $1
fi