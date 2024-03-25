#!/usr/bin/bash

PID=$1
INTERVAL=$2

if [[ -z "$PID" ]] ; then
    echo "invalid pid"
    exit
fi

if [[ -z "$INTERVAL" ]] ; then
    INTERVAL='1s'
fi

STATUS_FILE=/proc/$PID/status
OUTPUT_FILE=output.txt

rm -f "$OUTPUT_FILE"

while true ; do
    if [[ -f "$STATUS_FILE" ]] ; then
        grep 'VmSize' "$STATUS_FILE" | tee -a "$OUTPUT_FILE"
    else
        break
    fi

    sleep "$INTERVAL"
done
