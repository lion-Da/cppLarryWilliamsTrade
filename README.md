# cppLarryWilliamsTrade
linux 编译




1. 安装依赖
sudo apt update
sudo apt install libcurl4-openssl-dev nlohmann-json3-dev cmake build-essential libssl-dev git 
-------------------------------------------------------

2. 手动安装lws
git clone https://github.com/warmcat/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake ..
make -j{nproc}
sudo make install


---------------
3. cmake 构建
cd cppLarryWilliamsTrade/project && mkdir build && cd build 
cmake .. 
make -j${nproc}


# window构建
1. 切换win分支
2. 进入project下 新建build 目录 进入build目录 cmd 使用vs带的cmake 生成sln cmake .. -G "Visual Studio 17 2022"
3. 调整输入引用、库、链接库
