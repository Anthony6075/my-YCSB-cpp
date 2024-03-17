#!/usr/bin/bash

cd $(dirname $0)

LOCAL_OR_REMOTE=$1
DB_TYPE=$2

if [[ $LOCAL_OR_REMOTE == 'local' ]] ; then
    thirdparty_libs=thirdparty/local_libs
else
    thirdparty_libs=thirdparty/remote_libs
fi

cd $thirdparty_libs
cat librocksdb/xa* > librocksdb.so.8.9.0
ln -sf librocksdb.so.8.9.0 librocksdb.so.8.9
ln -sf librocksdb.so.8.9.0 librocksdb.so.8
ln -sf librocksdb.so.8.9.0 librocksdb.so
cd -

general_CXXFLAGS='-Ithirdparty/include'
general_LDFLAGS="-L$thirdparty_libs -Wl,-rpath=\$\$\{ORIGIN\}/$thirdparty_libs"

LevelDB_LDFLAGS='-lsnappy'

HashDB_LDFLAGS='-lgflags'

RocksDB_LDFLAGS='-lzstd -lz -lsnappy'


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
