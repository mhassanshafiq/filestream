FILES:
Two different zips included, file_stream_code.zip and file_stream_exe.zip 

PLATFORM:
The project is coded for x64 windows platform, linux support was not added at this point.


BUILD:
"file_stream_code.zip"
I used MS-Visual-Studio19 with integrated CMAKE support to build/compile this project.
To build this project, simply open code folder "file_stream_code" in VisualStuodio having CMAKE support, and build all.

If CMAKE is not avaialble I can prepare a standard visual studio solution and send that as well.

EXECUTABLES:
"file_stream_exe.zip"
This zip contains three executable projects. You can run these with some modifications to the "config.txt" in each project folder.
A 'testfile' is needed to run these, and it's details are to be provided in the config file.
The three projects are;
file_stream_client => client program 
file_stream_server => server program
file_stream_client_load => a small testing program I used to simulate multiple clients


CONFIG.TXT:
Same config file can be used for all the projects with modifications to the actuall configs, each project will just use the configs it needs.
The config file has following configurations, a brief description is provided for each;

server_port => The port on which server is to be run, or the port on which the client should connect.
server_host => The ip address of the server process machine.
server_file_dir => The directory where the server will store the transmitted files.
file_to_transfer => The file name of the file to be transmitted.
client_file_dir => The path to the file which is to be transmitted.
connection_retry => Retry tries the client will make to connect to server.
transmission_retry => Retry tries the client will make to transmit the file.
retry_interval => Interval in seconds after the connection/transmission retries will be made.
wait_for_ack => Flag indicating that client should wait for the final acknowledgment from server after transmission is complete.

BRIEF PROJECT OVERVIEW AND ASSUMPTIONS:
The project is using TCP sockets for transmission, and upon disconnection/reconnection the file transmission will resume at the current offset of the file at server.
The project currently does not support sending of the same file using multiple clients, though this functionality can be added with very little modifications to the server program.
The project currently assumes that all the required directories are created already.
The file in the main folder "FileStream.cpp" was used to run some basic functionality tests for the file handler and transport layer. 