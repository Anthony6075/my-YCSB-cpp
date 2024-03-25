#!/usr/bin/bash

PID=$1
INTERVAL=$2

if [[ -z "$PID" ]] ; then
    echo "invalid pid"
    exit
fi

if [[ -z "$INTERVAL" ]] ; then
    INTERVAL='1'
fi

IO_STATUS_FILE=/proc/$PID/io
READ_OUTPUT_FILE=read_output.txt
WRITE_OUTPUT_FILE=write_output.txt

rm -f "$READ_OUTPUT_FILE" "$WRITE_OUTPUT_FILE"

DATA=$(date '+%R:%S')
echo "$DATA" | tee -a "$READ_OUTPUT_FILE" "$WRITE_OUTPUT_FILE"
echo "$INTERVAL" | tee -a "$READ_OUTPUT_FILE" "$WRITE_OUTPUT_FILE"

while true ; do
    if [[ -f "$IO_STATUS_FILE" ]] ; then
        grep 'rchar' "$IO_STATUS_FILE" | tee -a "$READ_OUTPUT_FILE"
        grep 'wchar' "$IO_STATUS_FILE" | tee -a "$WRITE_OUTPUT_FILE"
    else
        break
    fi

    sleep "$INTERVAL"
done
