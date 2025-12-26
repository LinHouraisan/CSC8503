set(PROJECT_NAME server)

file(GLOB CODE
	*.cpp
	*.h
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME}  ${CODE})

target_link_libraries(${PROJECT_NAME} protocol)