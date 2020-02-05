# CMakeList.txt : CMake project for FileStream, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)


project (file_stream_client_load)
add_executable (file_stream_client_load
../file_stream_client/file_stream_client.cpp 
filestreamclientload.cpp
)
# TODO: Add tests and install targets if needed.

target_link_libraries(${PROJECT_NAME} PRIVATE
common
)