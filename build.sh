#!/usr/bin/bash

cd $(dirname $0)

LOCAL_OR_REMOTE=$1
DB_TYPE=$2

if [[ $LOCAL_OR_REMOTE == 'local' ]] ; then
    general_LDFLAGS='-Lthirdparty/local_libs -Wl,-rpath=$$\{ORIGIN\}/thirdparty/local_libs'
else
    general_LDFLAGS='-Lthirdparty/remote_libs -Wl,-rpath=$$\{ORIGIN\}/thirdparty/remote_libs'
fi

general_CXXFLAGS='-Ithirdparty/include'

LevelDB_LDFLAGS='-lsnappy'

HashDB_LDFLAGS='-lgflags'

RocksDB_LDFLAGS='-lzstd -lz'


if [[ "$DB_TYPE" == "pebblesdb" ]] ; then
    make \
    BIND_PEBBLESDB=1 \
    EXTRA_CXXFLAGS="$general_CXXFLAGS" \
    EXTRA_LDFLAGS="$general_LDFLAGS"
else
    make \
    BIND_LEVELDB=1 \
    BIND_HASHDB=1 \
    BIND_ROCKSDB=1 \
    BIND_WIREDTIGER=1 \
    BIND_SQLITE=1 \
    EXTRA_CXXFLAGS="$general_CXXFLAGS" \
    EXTRA_LDFLAGS="$general_LDFLAGS $LevelDB_LDFLAGS $HashDB_LDFLAGS $RocksDB_LDFLAGS"
fi
