# cppLarryWilliamsTrade
linux 编译

1. 安装依赖
```bash
sudo apt update
sudo apt install libcurl4-openssl-dev cmake build-essential libssl-dev git 
```
-------------------------------------------------------
2. cmake 构建
```bash
cd cppLarryWilliamsTrade/project && mkdir build && cd build 
cmake .. 
make -j${nproc}
```
