set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(BUILD_TESTING)
  add_executable(simple_test simple_test.c)
  target_link_libraries(simple_test "${PROJECT_NAME}")
  add_test(NAME simple_test COMMAND $<TARGET_FILE:simple_test>)

  add_executable(test_concurrency test_concurrency.c)
  target_link_libraries(test_concurrency "${PROJECT_NAME}")
  add_test(NAME test_concurrency COMMAND $<TARGET_FILE:test_concurrency>)
  
  add_executable(test_seek_equal_or_less test_seek_equal_or_less.c)
  target_link_libraries(test_seek_equal_or_less "${PROJECT_NAME}" log)
  add_test(NAME test_seek_equal_or_less COMMAND $<TARGET_FILE:test_seek_equal_or_less>)
endif()
