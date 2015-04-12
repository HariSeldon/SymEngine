set(TEST_LIST "yaml_parsing.cpp" 
              "nd_range.cpp"
              "memory_access.cpp")

set(GTEST_LIB "GTest")

add_library(${GTEST_LIB} ${GTEST_PATH}/gtest-all.cc)

include_directories(SYSTEM ${GTEST_PATH})

find_library(PTHREAD_LIB_PATH pthread
             DOC "The pthread library")
if(EXISTS ${PTHREAD_LIB_PATH})
  message(STATUS "Looking for pthread: found")
else(EXISTS ${PTHREAD_LIB_PATH})
  message(FATAL_ERROR "Looking for pthread: not found")
endif(EXISTS ${PTHREAD_LIB_PATH})

foreach(TEST_FILE ${TEST_LIST})
  get_filename_component(EXE_NAME ${TEST_FILE} NAME_WE)
  message(STATUS "Adding test:  ${EXE_NAME}")
  add_executable(${EXE_NAME} ${TEST_FILE})
  target_link_libraries(${EXE_NAME} ${GTEST_LIB} ${PTHREAD_LIB_PATH} ${SYM_ENGINE_CORE} ${REQ_LLVM_LIBS})
  add_test(NAME ${EXE_NAME} 
           COMMAND ${EXE_NAME})
endforeach(TEST_FILE)

# Copy yaml files to build directory.
file(COPY "test_hw_config.yaml" "test_kernel_arg.yaml" "test_config_opencl.yaml" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy LLVM files to build directory.
file(COPY "nd_range_test.ll" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
