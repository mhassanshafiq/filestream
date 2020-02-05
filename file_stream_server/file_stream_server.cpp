#include "file_stream_server.h"
#include <future>


file_stream_server::file_stream_server(int port, string file_dir): file_directory(file_dir)
{
	server_port = port;
	tl_server.add_on_receive_handler([this](SOCKET sock, shared_ptr<packet> packet_obj)
	{
		on_receive_handler(sock, packet_obj);
	});
	tl_server.add_on_connect_handler([this](SOCKET const sock, string const remote_host)
	{
		on_connect_handler(sock, remote_host);
	});
	tl_server.add_on_disconnect_handler([this](SOCKET const sock, string const remote_host)
	{
		on_disconnect_handler(sock, remote_host);
	});
	running = true;
	thread(&file_stream_server::process_packet_queue, this).detach();
}

file_stream_server::~file_stream_server()
{
	tl_server.stop_server();
}

void file_stream_server::start_server()
{
	tl_server.start_server(server_port);
}

void file_stream_server::stop_server()
{
	running = false;
	fp_cv.notify_all();
	pp_cv.notify_all();
	tl_server.stop_server();
}

void file_stream_server::on_connect_handler(SOCKET sock, string remote_host)
{
	cout << "client connected: " << remote_host << endl;
	TRY
	{
		unique_lock<mutex> lck(remote_host__filename_lock);
		auto it = remote_host__filename_map.find(remote_host);
		if (it != remote_host__filename_map.end())
		{
			lck.unlock();
			tl_server.close_socket(sock);
			cout << "client connection rejected: " << remote_host << endl;
		}
	}

	CATCH_STD
	{
		PRINT_STD;
		tl_server.close_socket(sock);
	}
}

void file_stream_server::on_disconnect_handler(SOCKET sock, string const remote_host)
{
	TRY
	{
		cout << "client disconnected: " << remote_host << endl;
		string filename;
		{
			unique_lock<mutex> lck(remote_host__filename_lock);
			if (remote_host__filename_map.find(remote_host) != remote_host__filename_map.end())
			{
				filename = remote_host__filename_map[remote_host];
			}
			else return;
		}
		shared_ptr<packet> pck = make_shared<packet>(packet::file_stream_finish, sizeof(filename), filename.c_str(), 0,
		                                             sock, remote_host);
		{
			unique_lock<mutex> lck(filename__packet_q_lock);
			if (filename__packet_q_map.find(filename) != filename__packet_q_map.end())
			{
				cout << "injecting filestream finish message for client: " << remote_host << endl;
				filename__packet_q_map[filename].push(pck);
			}
		}
	}
	CATCH_STD
	{
		PRINT_STD;
	}
}

void file_stream_server::on_receive_handler(SOCKET sock, shared_ptr<packet> packet_obj)
{
	while (pending_packets_flag.test_and_set(memory_order_acquire));
	pending_packets_q.push(std::move(packet_obj));
	pending_packets_flag.clear();

	pp_cv.notify_one();
}

void file_stream_server::remove_host_and_file(string filename, string remote_host)
{
	TRY
	{
		{
			unique_lock<mutex> lck(filename__packet_q_lock);
			if (filename__packet_q_map.find(filename) != filename__packet_q_map.end())
			{
				filename__packet_q_map.erase(filename);
			}
			else cout << "PROBLEM in 'process_filestream_packet' filename entry not found: " << filename << endl;
		}
		{
			unique_lock<mutex> lck(remote_host__filename_lock);
			if (remote_host__filename_map.find(remote_host) != remote_host__filename_map.end())
				remote_host__filename_map.erase(remote_host);
			else cout << "PROBLEM in 'process_filestream_packet' client entry not found: " << remote_host << endl;
		}
	}

	CATCH_STD
	{
		PRINT_STD;
	}
}


void file_stream_server::process_filestream_packet(string filename, string remote_host)
{
	bool filestream_active = true;
	file_handler filehandle(filename, file_handler::write, file_directory);
	streampos remaining_file_size;
	bool queue_empty = true;
	TRY
	{
		while (running && filestream_active)
		{
			shared_ptr<packet> packet_obj;
			{
				unique_lock<mutex> lck(filename__packet_q_lock);
				if (filename__packet_q_map.find(filename) == filename__packet_q_map.end())
				{
					cout << "PROBLEM in 'process_filestream_packet' filename entry not found: " << filename << endl;
					return;
				}
				if (!filename__packet_q_map[filename].empty())
				{
					queue_empty = false;
					packet_obj = std::move(filename__packet_q_map[filename].front());
					filename__packet_q_map[filename].pop();
				}
				else queue_empty = true;
			}
			if (queue_empty)
			{
				unique_lock<mutex> fp_l(fp_m);
				fp_cv.wait_for(fp_l, chrono::milliseconds(10));
				continue;
			}
			std::cout << "new packet: " << packet_obj->packet_type << ", client: " << remote_host << std::endl;
			switch (packet_obj->packet_type)
			{
			case packet::new_file_stream:
				[&]()
				{
					streampos curr_offset = filehandle.get_eof_pos();
					remaining_file_size = packet_obj->offset - curr_offset;
					cout << "file: " << filename << ", transmission started, total file size: " << remaining_file_size
						<< ", current eof: " << filehandle.get_eof_pos() << endl;
					shared_ptr<packet> pck = make_shared<packet>(packet::new_file_stream_ack,
					                                             filehandle.get_file_name().length(),
					                                             filehandle.get_file_name().c_str(), curr_offset,
					                                             packet_obj->client, packet_obj->remote_host);


					tl_server.send_data_to_client(packet_obj->client, packet_obj->remote_host, std::move(pck));
				}();
				break;
			case packet::data_chunk:
				[&]()
				{
					if (filehandle.get_eof_pos() != packet_obj->offset)
					{
						cout << "that's weird, file offset: " << packet_obj->offset << " is not what expected: " <<
							filehandle.get_offset_pos() << ", closing connection to sync again" << endl;
						tl_server.close_socket(packet_obj->client);
					}
					else
					{
						filehandle.write_file(packet_obj->data.get(), packet_obj->data_size, packet_obj->offset);
						remaining_file_size -= packet_obj->data_size;

						shared_ptr<packet> pck = make_shared<packet>(packet::data_chunk_ack,
						                                             filehandle.get_file_name().length(),
						                                             filehandle.get_file_name().c_str(),
						                                             filehandle.get_offset_pos(), packet_obj->client,
						                                             packet_obj->remote_host);

						tl_server.send_data_to_client(packet_obj->client, packet_obj->remote_host, std::move(pck));
					}
				}();
				break;
			case packet::file_stream_finish:
				[&]()
				{
					if (remaining_file_size > 0)
					{
						cout << "file: " << filename <<
							", transmission ended before complete transfer, remaining size: "
							<< remaining_file_size << ", curr eof: " << filehandle.get_offset_pos() << endl;
					}
					else
						cout << "file: " << filename << ", transmission completed successfully, total file size: " <<
							filehandle.get_offset_pos() << endl;
					shared_ptr<packet> pck = make_shared<packet>(packet::file_stream_finish_ack,
					                                             filehandle.get_file_name().length(),
					                                             filehandle.get_file_name().c_str(),
					                                             filehandle.get_eof_pos(), packet_obj->client,
					                                             packet_obj->remote_host);

					tl_server.send_data_to_client(packet_obj->client, packet_obj->remote_host, std::move(pck));

					filehandle.close_file();
				}();
				filestream_active = false;
				remove_host_and_file(filename, remote_host);
				continue;
			default:
				cout << "unhandled packet type..";
			}
		}
	}

	CATCH_STD
	{
		PRINT_STD;
		filehandle.close_file();
	}
}

void file_stream_server::send_filestream_rejected(SOCKET client, string const remote_host, string const filename)
{
	shared_ptr<packet> pck = make_shared<packet>(packet::file_stream_rejected, filename.length(), filename.c_str());

	tl_server.send_data_to_client(client, remote_host, pck);
}

void file_stream_server::process_packet_queue()
{
	shared_ptr<packet> pck;
	while (running)
	{
		TRY
		{
			while (!pending_packets_q.empty())
			{
				string filename;
				while (pending_packets_flag.test_and_set(memory_order_acquire));
				pck = std::move(pending_packets_q.front());
				pending_packets_q.pop();
				pending_packets_flag.clear();

				if (pck->packet_type == packet::new_file_stream)
				{
					filename = string(static_cast<char*>(pck->data.get()));
					{
						unique_lock<mutex> lck(filename__packet_q_lock);
						if (filename__packet_q_map.find(filename) != filename__packet_q_map.end())
						{
							lck.unlock();
							send_filestream_rejected(pck->client, pck->remote_host, filename);
							continue;
						}
						filename__packet_q_map.emplace(filename, queue<shared_ptr<packet>>());
						filename__packet_q_map[filename].push(pck);
						thread(&file_stream_server::process_filestream_packet, this, filename,
						       pck->remote_host).detach();
						lck.unlock();
						{
							unique_lock<mutex> lck(remote_host__filename_lock);
							if (remote_host__filename_map.find(pck->remote_host) != remote_host__filename_map.end())
								cout << "PROBLEM..client entry already in map: " << pck->remote_host << endl;
							remote_host__filename_map[pck->remote_host] = filename;
						}
					}
				}
				else
				{
					{
						unique_lock<mutex> lck(remote_host__filename_lock);
						if (remote_host__filename_map.find(pck->remote_host) != remote_host__filename_map.end())
							filename = remote_host__filename_map[pck->remote_host];
						else cout << "PROBLEM..client entry not found: " << pck->remote_host << endl;
					}
					{
						unique_lock<mutex> lck(filename__packet_q_lock);
						if (filename__packet_q_map.find(filename) != filename__packet_q_map.end())
							filename__packet_q_map[filename].push(pck);
						else cout << "PROBLEM..filename entry not found: " << filename << endl;
					}
				}
				fp_cv.notify_all();
			}
		}
		CATCH_STD
		{
			PRINT_STD;
		}
		{
			unique_lock<mutex> pp_l(pp_m);
			pp_cv.wait_for(pp_l, std::chrono::milliseconds(10));
		}
	}
}
