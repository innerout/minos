set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(BUILD_TESTING)
  add_executable(simple_test simple_test.c)
  target_link_libraries(simple_test PRIVATE "${PROJECT_NAME}" Threads::Threads)
  add_test(NAME simple_test COMMAND $<TARGET_FILE:simple_test>)

  add_executable(test_concurrency test_concurrency.c)
  target_link_libraries(test_concurrency PRIVATE "${PROJECT_NAME}" Threads::Threads)
  add_test(NAME test_concurrency COMMAND $<TARGET_FILE:simple_test>)
  

  add_executable(test_seek_equal_or_less test_seek_equal_or_less.c)
  target_link_libraries(test_seek_equal_or_less PRIVATE "${PROJECT_NAME}" log Threads::Threads)
  add_test(NAME test_seek_equal_or_less
         COMMAND test_seek_equal_or_less 8192)


  
  add_executable(test_updates test_updates.c)
  target_link_libraries(test_updates PRIVATE "${PROJECT_NAME}" log Threads::Threads)
  add_test(NAME test_updates
         COMMAND test_updates 8192)
endif()
