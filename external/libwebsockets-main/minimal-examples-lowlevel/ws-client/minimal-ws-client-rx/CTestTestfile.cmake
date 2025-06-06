# CMake generated Testfile for 
# Source directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx
# Build directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ws-client-rx-warmcat "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Debug/lws-minimal-ws-client-rx.exe" "-t")
  set_tests_properties(ws-client-rx-warmcat PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ws-client-rx-warmcat "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Release/lws-minimal-ws-client-rx.exe" "-t")
  set_tests_properties(ws-client-rx-warmcat PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ws-client-rx-warmcat "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/MinSizeRel/lws-minimal-ws-client-rx.exe" "-t")
  set_tests_properties(ws-client-rx-warmcat PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ws-client-rx-warmcat "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/RelWithDebInfo/lws-minimal-ws-client-rx.exe" "-t")
  set_tests_properties(ws-client-rx-warmcat PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/ws-client/minimal-ws-client-rx/CMakeLists.txt;0;")
else()
  add_test(ws-client-rx-warmcat NOT_AVAILABLE)
endif()
