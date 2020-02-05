#include "packet.h"


packet::packet(packet::packet_types packet_type, int data_size, const char* data_buf,
	std::streampos offset, SOCKET client, std::string remote_host) :
	packet_type(packet_type),
	data_size(data_size),
	offset(offset),
	client(client),
	remote_host(remote_host)
{
	TRY
	{
		std::unique_ptr<char[]> str(new char[data_size + 1]);
		std::copy(data_buf, data_buf + data_size + 1, &str[0]);

		this->data = std::move(str);
	}
	CATCH_STD
	{
		PRINT_STD;
		THROW;
	}
}
