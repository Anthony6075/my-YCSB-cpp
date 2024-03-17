#!/usr/bin/bash

cd $(dirname $0)

mkdir -p a_learning

cd a_learning

DB=$1
WORKLOAD=$2

if [[ -z "$DB" ]] ; then
    echo "empty db"
    exit
fi

if [[ -z "$WORKLOAD" ]] ; then
    echo "empty workload"
    exit
fi

../ycsb -load -run \
        -db $DB \
        -P ../workloads/workload$WORKLOAD \
        -P ../$DB/$DB.properties \
        -s

#rm -rf ./ycsb-*
rm -rf /root/exp/data/ycsb-*
rm -rf ./hashdb
