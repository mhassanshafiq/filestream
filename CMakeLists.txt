# CMakeList.txt : CMake project for FileStream, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (filestream)



# Add source to this project's executable.
 add_executable (filestream 
 FileStream.cpp)

# TODO: Add tests and install targets if needed.
target_link_libraries(${PROJECT_NAME} PRIVATE
common
)
add_subdirectory("common")
add_subdirectory("file_stream_client")
add_subdirectory("file_stream_server")
add_subdirectory("file_stream_client_load")