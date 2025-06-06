# CMake generated Testfile for 
# Source directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc
# Build directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(api-test-jrpc "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Debug/lws-api-test-jrpc.exe")
  set_tests_properties(api-test-jrpc PROPERTIES  _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;72;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(api-test-jrpc "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Release/lws-api-test-jrpc.exe")
  set_tests_properties(api-test-jrpc PROPERTIES  _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;72;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(api-test-jrpc "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/MinSizeRel/lws-api-test-jrpc.exe")
  set_tests_properties(api-test-jrpc PROPERTIES  _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;72;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(api-test-jrpc "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/RelWithDebInfo/lws-api-test-jrpc.exe")
  set_tests_properties(api-test-jrpc PROPERTIES  _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;72;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-jrpc/CMakeLists.txt;0;")
else()
  add_test(api-test-jrpc NOT_AVAILABLE)
endif()
