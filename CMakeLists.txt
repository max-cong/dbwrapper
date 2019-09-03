PROJECT(medis)
cmake_minimum_required(VERSION 3.5)


SET(M_VERSION_MAJOR 1)
SET(M_VERSION_MINOR 0)
SET(M_VERSION_PATCH 0)
SET(M_VERSION ${M_VERSION_MAJOR}.${M_VERSION_MINOR}.${M_VERSION_PATCH})


SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(TESTS_DT_BINARY ${PROJECT_NAME}_dt_tests)
SET(TESTS_UT_BINARY ${PROJECT_NAME}_ut_tests)

SET(EXAMPLE_BINARY ${PROJECT_NAME}_example)


add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

IF(CODE_COVERAGE)
	SET(CCXX_FLAGS "-O0 -g -Wall -std=c++11 -fPIC -lpthread -fprofile-arcs -ftest-coverage -DBOOST_LOG_DYN_LINK  -DBOOST_ERROR_CODE_HEADER_ONLY")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CCXX_FLAGS}")
ELSE()
    SET(CCXX_FLAGS "-O2 -g -Wall -std=c++11 -fPIC -DBOOST_LOG_DYN_LINK  -DBOOST_ERROR_CODE_HEADER_ONLY")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CCXX_FLAGS}")
ENDIF()

MESSAGE(STATUS "start build")
MESSAGE(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})

ADD_SUBDIRECTORY(example)
add_subdirectory(tst)

ADD_CUSTOM_TARGET(example
	COMMAND ./${EXAMPLE_BINARY} ${CMD_ARGS}
	WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
	COMMENT "Run examples"
)

ADD_CUSTOM_TARGET(tests
        COMMAND ./${TESTS_BINARY} ${CMD_ARGS}
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMENT "Run tests"
)


ADD_CUSTOM_TARGET(memcheck
	COMMAND command -v valgrind >/dev/null 2>&1 && valgrind -v --leak-check=full --show-reachable=yes --track-fds=yes
	--undef-value-errors=yes --track-origins=yes --num-callers=50 ./${EXAMPLE_BINARY} ${CMD_ARGS}
	WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
	COMMENT "Check memory usage with valgrind"
)


ADD_CUSTOM_TARGET(cleanup
	COMMAND rm -rf *
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Remove all temp files"
)


