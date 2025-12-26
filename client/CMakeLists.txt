################################################################################
# Target
################################################################################

file(GLOB CODE
	client.h
	client.cpp
)

add_library(client STATIC ${CODE})

target_include_directories(client PUBLIC .)

target_link_libraries(client protocol)

target_compile_definitions(client PUBLIC _WIN32_WINNT=0x0601)

################################################################################
# Target
################################################################################
file(GLOB CODE
	main.cpp
)

add_executable(client_app ${CODE})

target_link_libraries(client_app client)