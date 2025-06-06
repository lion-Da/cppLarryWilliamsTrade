# CMake generated Testfile for 
# Source directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams
# Build directory: F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(api-test-secure-streams "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Debug/lws-api-test-secure-streams.exe")
  set_tests_properties(api-test-secure-streams PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(api-test-secure-streams "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/Release/lws-api-test-secure-streams.exe")
  set_tests_properties(api-test-secure-streams PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(api-test-secure-streams "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/MinSizeRel/lws-api-test-secure-streams.exe")
  set_tests_properties(api-test-secure-streams PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(api-test-secure-streams "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/bin/RelWithDebInfo/lws-api-test-secure-streams.exe")
  set_tests_properties(api-test-secure-streams PROPERTIES  TIMEOUT "20" WORKING_DIRECTORY "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams" _BACKTRACE_TRIPLES "F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;18;add_test;F:/project/cppLarryWilliamsTrade/external/libwebsockets-main/minimal-examples-lowlevel/api-tests/api-test-secure-streams/CMakeLists.txt;0;")
else()
  add_test(api-test-secure-streams NOT_AVAILABLE)
endif()
