#pragma once
#include <algorithm>
#include <memory>
#include <atlalloc.h>
#include <string>
#include"exceptions.h"
class packet
{

public:
	typedef enum
	{
		new_file_stream = 1,
		data_chunk,
		file_stream_finish,
		file_stream_rejected,
		new_file_stream_ack,		
		data_chunk_ack,		
		file_stream_finish_ack
	}packet_types;
	packet_types packet_type;
	int data_size;
	std::streampos offset;
	std::unique_ptr<char[]> data;
	SOCKET client;
	std::string remote_host;
public:
	packet(packet::packet_types packet_type, int data_size, const char* data_buf, 
		std::streampos offset = 0, SOCKET client = 0, std::string remote_host = "");
};
