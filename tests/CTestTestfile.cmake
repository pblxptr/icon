# CMake generated Testfile for 
# Source directory: /icon/tests
# Build directory: /icon/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(UnitTests "/icon/tests/icon_unittests")
set_tests_properties(UnitTests PROPERTIES  _BACKTRACE_TRIPLES "/icon/tests/CMakeLists.txt;58;add_test;/icon/tests/CMakeLists.txt;0;")
add_test(BasicE2E "/icon/tests/icon_e2e")
set_tests_properties(BasicE2E PROPERTIES  _BACKTRACE_TRIPLES "/icon/tests/CMakeLists.txt;74;add_test;/icon/tests/CMakeLists.txt;0;")
