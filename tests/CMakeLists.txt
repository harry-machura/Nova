
# CTest configuration for nova
enable_testing()

add_test(NAME compile_hello
  COMMAND $<TARGET_FILE:novac> ${CMAKE_SOURCE_DIR}/examples/hello.nova ${CMAKE_BINARY_DIR}/hello.nvc
)
add_test(NAME run_hello
  COMMAND $<TARGET_FILE:novavm> ${CMAKE_BINARY_DIR}/hello.nvc
)
set_tests_properties(run_hello PROPERTIES PASS_REGULAR_EXPRESSION "Hello, Nova!|42")


add_test(NAME compile_loop
  COMMAND $<TARGET_FILE:novac> ${CMAKE_SOURCE_DIR}/examples/loop.nova ${CMAKE_BINARY_DIR}/loop.nvc
)
add_test(NAME run_loop
  COMMAND $<TARGET_FILE:novavm> ${CMAKE_BINARY_DIR}/loop.nvc
)
set_tests_properties(run_loop PROPERTIES
  PASS_REGULAR_EXPRESSION "i=0;i=1;i=2;i=3;i=4"
)
