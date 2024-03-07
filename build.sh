#!/usr/bin/bash

# if [[ $1 == "leveldb" ]] ; then 
#     make BIND_LEVELDB=1 EXTRA_CXXFLAGS=-I../leveldb/include EXTRA_LDFLAGS="-L../leveldb/build -lsnappy"
# elif [[ $1 == "hahsdb" ]] ; then
#     make BIND_HASHDB=1 EXTRA_LDFLAGS="-L../myself/hashdb/build"
# else
#     # compile hashdb by default
#     make BIND_HASHDB=1 DEBUG_BUILD=1 EXTRA_LDFLAGS="-L../myself/hashdb/build -lgflags -L/home/anthony/master/myself/hashdb/thirdparty/all_lib/ -l:libbf.a"
# fi

cd $(dirname $0)

LevelDB_CXXFLAGS='-I../leveldb/include'
LevelDB_LDFLAGS='-L../leveldb/build -lsnappy'

HashDB_CXXFLAGS=''
HashDB_LDFLAGS='-L../myself/hashdb/build -lgflags -Wl,-rpath=/home/anthony/master/myself/hashdb/build'

RocksDB_CXXFLAGS='-I../rocksdb/include'
RocksDB_LDFLAGS='-L../rocksdb -lzstd -lz'

WiredTiger_CXXFLAGS='-I../wiredtiger/build/include/ -I../wiredtiger/src/include/'
WiredTiger_LDFLAGS='-L/home/anthony/master/wiredtiger/build -Wl,-rpath=/home/anthony/master/wiredtiger/build'

Sqlite_CXXFLAGS='-I../sqlite/build'
Sqlite_LDFLAGS='-L/home/anthony/master/sqlite/build/.libs'

PebblesDB_CXXFLAGS='-I/home/anthony/master/pebblesdb/src/include'
PebblesDB_LDFLAGS='-L/home/anthony/master/pebblesdb/src/.libs -Wl,-rpath=/home/anthony/master/pebblesdb/src/.libs'


if [[ "$1" == "p" ]] ; then
    make \
    BIND_PEBBLESDB=1 \
    EXTRA_CXXFLAGS="$PebblesDB_CXXFLAGS" \
    EXTRA_LDFLAGS="$PebblesDB_LDFLAGS"
else
    make \
    BIND_LEVELDB=1 \
    BIND_HASHDB=1 \
    BIND_ROCKSDB=1 \
    BIND_WIREDTIGER=1 \
    BIND_SQLITE=1 \
    EXTRA_CXXFLAGS="$LevelDB_CXXFLAGS $HashDB_CXXFLAGS $RocksDB_CXXFLAGS $WiredTiger_CXXFLAGS $Sqlite_CXXFLAGS" \
    EXTRA_LDFLAGS="$LevelDB_LDFLAGS $HashDB_LDFLAGS $RocksDB_LDFLAGS $WiredTiger_LDFLAGS $Sqlite_LDFLAGS"
fi
