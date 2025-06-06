# cppLarryWilliamsTrade
linux 编译
1. 安装依赖
apt update
apt install libcurl4-openssl-dev nlohmann-json3-dev cmake build-essential
3. cmake 构建
cd cppLarryWilliamsTrade/project && mkdir build && cd build 
cmake .. 
make -j${nproc}
