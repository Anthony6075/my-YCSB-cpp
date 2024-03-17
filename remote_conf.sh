#!/usr/bin/bash

mkdir -p /root/exp/data

git config --global http.version HTTP/1.1
git config --global user.name anthony_in_server
git config --global user.email Anthony6075@163.com

apt update
apt install -y cmake
apt install -y libgflags-dev
apt install -y libzstd-dev
apt install -y libsnappy-dev
apt install -y swig
