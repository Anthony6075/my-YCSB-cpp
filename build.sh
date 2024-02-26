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
HashDB_LDFLAGS='-L../myself/hashdb/build -lgflags -L/home/anthony/master/myself/hashdb/thirdparty/all_lib/ -l:libbf.a'

RocksDB_CXXFLAGS='-I../rocksdb/include'
RocksDB_LDFLAGS='-L../rocksdb -lzstd -lz'

make \
    BIND_LEVELDB=1 \
    BIND_HASHDB=1 \
    BIND_ROCKSDB=1 \
    EXTRA_CXXFLAGS="$LevelDB_CXXFLAGS $HashDB_CXXFLAGS $RocksDB_CXXFLAGS" \
    EXTRA_LDFLAGS="$LevelDB_LDFLAGS $HashDB_LDFLAGS $RocksDB_LDFLAGS"

