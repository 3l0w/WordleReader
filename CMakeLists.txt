cmake_minimum_required(VERSION 3.10)

# set the project name
project(WordleReader)

if(NOT TARGET library_WordleSolver)
	add_subdirectory(lib/WordleSolver/src)
endif()

if(NOT TARGET library_uiohook)
	add_subdirectory(lib/libuiohook)
endif()

if(NOT TARGET library_color)
	add_subdirectory(lib/color)
endif()


# add the executable
add_executable(WordleReader wordlereader.cpp)
target_link_libraries(WordleReader PRIVATE color)
target_link_libraries(WordleReader PRIVATE WordleSolver)
target_link_libraries(WordleReader PRIVATE uiohook)

