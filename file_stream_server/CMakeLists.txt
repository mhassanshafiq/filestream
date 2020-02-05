# CMakeList.txt : CMake project for FileStream, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project (file_stream_server)




# Add source to this project's executable.
add_executable (file_stream_server 
file_stream_server.cpp 
filestreamserver.cpp
)
# TODO: Add tests and install targets if needed.

target_link_libraries(${PROJECT_NAME} PRIVATE
common
)