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

make \
    BIND_LEVELDB=1 \
    BIND_HASHDB=1 \
    EXTRA_CXXFLAGS=-I../leveldb/include \
    EXTRA_LDFLAGS="-L../leveldb/build -lsnappy -L../myself/hashdb/build -lgflags -L/home/anthony/master/myself/hashdb/thirdparty/all_lib/ -l:libbf.a"

