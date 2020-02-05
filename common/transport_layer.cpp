#include "transport_layer.h"
#include <string>

transport_layer::transport_layer()
{
	WSAStartup(MAKEWORD(2, 0), &WSAData);
}

transport_layer::~transport_layer()
{
	if (is_server) stop_server();
	else
	{
		close_socket(server);
		running = false;
		connected_to_server = false;
	}
}

void transport_layer::remove_client(SOCKET client)
{
	if (running)
	{
		TRY
		{
			std::unique_lock<std::mutex> lck(connected_clients_lock);
			connected_clients_map.erase(client);
			lck.unlock();
		}
		CATCH_STD
		{
			PRINT_STD;
			running = false;
		}
	}
}

void transport_layer::disconnect_from_server()
{
	close_socket(server);
	running = false;
	connected_to_server = false;
}

void transport_layer::close_socket(SOCKET sock)
{
	closesocket(sock);
}

void transport_layer::stop_server()
{
	if (running)
	{
		running = false;
		TRY
		{
			close_socket(server);
			std::cout << "Stopping server.." << std::endl;
			std::unique_lock<std::mutex> lck(connected_clients_lock, std::defer_lock);

			lck.lock();
			for (auto i = connected_clients_map.begin(); i != connected_clients_map.end(); ++i)
			{
				closesocket(i->first);
			}
			connected_clients_map.clear();
			lck.unlock();
			std::cout << "Server stopped.." << std::endl;
		}
		CATCH_STD
		{
			PRINT_STD;
		}
	}
}

void transport_layer::start_server(int port)
{
	is_server = true;
	TRY
	{
		std::thread server_thread([=]()
		{
			SOCKADDR_IN serverAddr;
			std::unique_lock<std::mutex> lck(connected_clients_lock, std::defer_lock);
			server = socket(AF_INET, SOCK_STREAM, 0);
			if (server == INVALID_SOCKET)
			{
				std::cout << "socket " << WSAGetLastError() << std::endl;
			}
			serverAddr.sin_addr.s_addr = INADDR_ANY;
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons(port);

			if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0)
			{
				std::cout << "bind " << WSAGetLastError() << " sizeof " << sizeof(serverAddr) << std::endl;
				std::cout << "cannot start server" << std::endl;
				return;
			}
			if (listen(server, SOMAXCONN) != 0)
			{
				std::cout << "listen " << WSAGetLastError() << std::endl;
				std::cout << "cannot start server" << std::endl;
				return;
			}
			std::cout << "Listening for incoming connections on port: " << port << std::endl;

			running = true;
			int clientAddrSize = sizeof(serverAddr);
			while (running)
			{
				SOCKADDR_IN clientAddr;
				SOCKET client;
				std::string remote_host;
				if ((client = accept(server, (SOCKADDR*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET)
				{
					remote_host = std::to_string(clientAddr.sin_addr.s_addr) + ":" + std::
						to_string(clientAddr.sin_port);
					std::cout << "New connection on port: " << port << ", from " << remote_host << ", client: " <<
						client << std::endl;
					if (m_on_connect_handler)
						m_on_connect_handler(client, remote_host);
					std::thread(&transport_layer::listener, this, client, remote_host).detach();
					lck.lock();
					connected_clients_map.emplace(client, remote_host);
					lck.unlock();
				}
				else
				{
					std::cout << "accept " << WSAGetLastError() << std::endl;
					running = false;
					return;
				}
			}
		});
		server_thread.detach();
	}
	CATCH_STD
	{
		PRINT_STD;
	}
	CATCH
	{
		PRINT_EX;
	}
	running = false;
}

bool read(SOCKET client, char* buf, int len)
{
	int len_read = 0;
	while (len_read != len)
	{
		len_read += recv(client, buf + len_read, len - len_read, 0);
		if (len_read <= 0)
		{
			std::cout << "Connection closed, client: " << client << ", error: " << WSAGetLastError() << std::endl;
			return false;
		}
	}
	return true;
}

void transport_layer::listener(SOCKET sock, std::string const remote_host)
{
	char buffer[BUFFER_SIZE + 1];
	char msg_type_buff[2], msg_size_buff[6], file_offset[16];
	int msg_size;
	std::streampos offset;
	short msg_type;
	bool no_error = true;
	std::shared_ptr<packet> pck;

	TRY
	{
		while (running & no_error)
		{
			if (!read(sock, msg_type_buff, sizeof(msg_type_buff)))
			{
				std::cout << "Error reading msg_type, client: " << sock << std::endl;
				no_error = false;
				break;
			}
			if (!read(sock, msg_size_buff, sizeof(msg_size_buff)))
			{
				std::cout << "Error reading msg_size, client: " << sock << std::endl;
				no_error = false;
				break;
			}
			if (!read(sock, file_offset, sizeof(file_offset)))
			{
				std::cout << "Error reading file_offset, client: " << sock << std::endl;
				no_error = false;
				break;
			}
			if ((msg_type = std::stoi(msg_type_buff)) == 0 || (msg_size = std::stoi(msg_size_buff)) == 0)
			{
				std::cout << "Error parsing msg_size/msg_type, client: " << sock << ", msg_type/msg_size: " <<
					msg_type_buff << "/" << msg_size_buff << std::endl;
				no_error = false;
				break;
			}
			offset = static_cast<std::streampos>(std::stoll(file_offset));
			if (!read(sock, buffer, msg_size))
			{
				std::cout << "Error reading data, client: " << sock << std::endl;
				no_error = false;
				break;
			}
			buffer[msg_size] = 0;
			switch (msg_type)
			{
			case packet::new_file_stream:
			case packet::data_chunk:
			case packet::data_chunk_ack:
			case packet::file_stream_finish:
			case packet::file_stream_rejected:
			case packet::new_file_stream_ack:
			case packet::file_stream_finish_ack:
				std::cout << "new msg: " << msg_type << ", client: " << sock << ", msg_type/msg_size/offset: " <<
					msg_type_buff << "/" << msg_size_buff << "/" << offset << std::endl;
				pck = std::make_shared<packet>(static_cast<packet::packet_types>(msg_type), msg_size, buffer, offset,
				                               sock, remote_host);
				if (m_on_receive_handler)
				{
					m_on_receive_handler(sock, std::move(pck));
				}
				else std::cout << "no receive handler, client: " << sock << std::endl;
				break;
			default:
				std::cout << "Unknown msg_type: " << msg_type << ", client: " << sock << std::endl;
				no_error = false;
				break;
			}
		}
	}
	CATCH_STD
	{
		PRINT_STD;
	}
	CATCH
	{
		PRINT_EX;
	}
	if (is_server)
	{
		remove_client(sock);
		if (m_on_disconnect_handler)
			m_on_disconnect_handler(sock, remote_host);
	}
	else
	{
		connected_to_server = false;
		close_socket(sock);
	}
}

bool write(SOCKET sock, const char* buf, int len)
{
	int len_sent = 0;
	while (len_sent != len)
	{
		len_sent += send(sock, buf + len_sent, len - len_sent, 0);
		if (len_sent <= 0)
		{
			std::cout << "Connection closed, client: " << sock << ", error: " << WSAGetLastError() << std::endl;
			return false;
		}
	}
	return true;
}

bool transport_layer::send_data_to_client(SOCKET client, std::string remote_host,
                                          std::shared_ptr<packet> const packet_obj)
{
	std::unique_lock<std::mutex> lck(connected_clients_lock);
	if (connected_clients_map.find(client) != connected_clients_map.end())
	{
		if (connected_clients_map[client] == remote_host)
		{
			lck.unlock();
			return send_data(client, packet_obj);
		}
		std::cout << "client is already closed: " << remote_host << std::endl;
		return false;
	}
	return false;
}

bool transport_layer::send_data(SOCKET sock, std::shared_ptr<packet> const packet_obj)
{
	std::string temp;
	char msg_type_buff[2];
	char msg_size_buff[6];
	char file_offset[16];
	bool error = false;
	TRY
	{
		temp = std::to_string(packet_obj->packet_type);
		strncpy(msg_type_buff, temp.c_str(), sizeof(msg_type_buff));


		temp = std::to_string(packet_obj->data_size);
		strncpy(msg_size_buff, temp.c_str(), sizeof(msg_size_buff));


		temp = std::to_string(static_cast<long long>(packet_obj->offset));
		strncpy(file_offset, temp.c_str(), sizeof(file_offset));


		if (!write(sock, msg_type_buff, sizeof(msg_type_buff)))
		{
			error = true;
			std::cout << "Error writing msg_type, client: " << sock << std::endl;
		}
		if (!write(sock, msg_size_buff, sizeof(msg_size_buff)))
		{
			error = true;
			std::cout << "Error writing msg_size, client: " << sock << std::endl;
		}
		if (!write(sock, file_offset, sizeof(file_offset)))
		{
			error = true;
			std::cout << "Error writing file_offset, client: " << sock << std::endl;
		}
		if (!write(sock, packet_obj->data.get(), packet_obj->data_size))
		{
			error = true;
			std::cout << "Error writing data, client: " << sock << std::endl;
		}
	}
	CATCH_STD
	{
		PRINT_STD;
		error = true;
	}
	CATCH
	{
		PRINT_EX;
		error = true;
	}
	if (error)
	{
		connected_to_server = false;
		if (!is_server)close_socket(sock);
		return false;
	}
	return true;
}

bool transport_layer::connect_to_server(std::string host, int port)
{
	server = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN serverAddr;
	inet_pton(AF_INET, host.c_str(), &(serverAddr.sin_addr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	std::cout << "trying connection at: " << host << ":" << port << std::endl;
	if (connect(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == 0)
	{
		connected_to_server = true;
		running = true;
		std::thread(&transport_layer::listener, this, server, host + ":" + std::to_string(port)).detach();
		std::cout << "Connected to server!" << std::endl;
		return true;
	}
	std::cout << "unable to connect to server!" << std::endl;
	return false;
}

void transport_layer::add_on_connect_handler(std::function<void(SOCKET client_sock, std::string remote_host)> handler)
{
	m_on_connect_handler = handler;
}

void transport_layer::add_on_disconnect_handler(
	std::function<void(SOCKET client_sock, std::string remote_host)> handler)
{
	m_on_disconnect_handler = handler;
}

void transport_layer::add_on_receive_handler(
	std::function<void(SOCKET client_sock, std::shared_ptr<packet> packet_obj)> handler)
{
	m_on_receive_handler = handler;
}
