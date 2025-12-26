set(PROJECT_NAME protocol)

file(GLOB CODE
	*.cpp
	*.h
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${CODE})

target_include_directories(${PROJECT_NAME} PUBLIC
	.
)