# cppLarryWilliamsTrade
linux 编译

1. 安装依赖
```bash
sudo apt update
sudo apt install libcurl4-openssl-dev nlohmann-json3-dev cmake build-essential libssl-dev git 
```
-------------------------------------------------------

2. 手动安装lws
```bash
git clone https://github.com/warmcat/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake ..
make -j{nproc}
sudo make install
```

---------------
3. cmake 构建
```bash
cd cppLarryWilliamsTrade/project && mkdir build && cd build 
cmake .. 
make -j${nproc}
```
