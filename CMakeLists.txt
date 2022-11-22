cmake_minimum_required (VERSION 3.0)
project (RN-Praxis1)
set (CMAKE_C_STANDARD 11)

add_executable (webserver webserver.c http.c http.h)

add_custom_target(print
	${CMAKE_COMMAND} -E echo $<TARGET_FILE:webserver>
    )

# Do not change beyond this point :)
# Testing
enable_testing ()
add_test(NAME Execute COMMAND ${PROJECT_SOURCE_DIR}/test/execute.py $<TARGET_FILE:webserver>)
add_test(NAME Listen COMMAND ${PROJECT_SOURCE_DIR}/test/listen.py $<TARGET_FILE:webserver>)
add_test(NAME Reply COMMAND ${PROJECT_SOURCE_DIR}/test/reply.py $<TARGET_FILE:webserver>)
add_test(NAME Packets COMMAND ${PROJECT_SOURCE_DIR}/test/packets.py $<TARGET_FILE:webserver>)
add_test(NAME HTTPReply COMMAND ${PROJECT_SOURCE_DIR}/test/httpreply.py $<TARGET_FILE:webserver>)
add_test(NAME HTTPReplies COMMAND ${PROJECT_SOURCE_DIR}/test/httpreplies.py $<TARGET_FILE:webserver>)
add_test(NAME StaticContent COMMAND ${PROJECT_SOURCE_DIR}/test/static_content.py $<TARGET_FILE:webserver>)
add_test(NAME DynamicContent COMMAND ${PROJECT_SOURCE_DIR}/test/dynamic_content.py $<TARGET_FILE:webserver>)
set_tests_properties(Listen Reply Packets HTTPReply HTTPReplies StaticContent DynamicContent PROPERTIES RUN_SERIAL TRUE)

# Packaging
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  ${CMAKE_BINARY_DIR}
  /\\..*$
)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)

