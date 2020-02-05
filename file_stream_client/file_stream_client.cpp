#include "file_stream_client.h"


file_stream_client::file_stream_client(string host, int const port, string filename, string file_dir,
                                       bool const wait_for_ack) :
	file_reader(filename, file_handler::read, file_dir),
	file_name(filename),
	server_host(host),
	server_port(port),
	wait_for_trans_finish_ack(wait_for_ack)
{
	tl_client.add_on_receive_handler([this](SOCKET const sock, shared_ptr<packet> packet_obj)
	{
		on_receive_handler(sock, std::move(packet_obj));
	});
}

file_stream_client::~file_stream_client()
{
	stop_client();
}

void file_stream_client::stop_client()
{
	file_reader.close_file();
	tl_client.disconnect_from_server();
}

void file_stream_client::update_filename(string filename)
{
	file_name = std::move(filename);
}

bool file_stream_client::connect_to_server()
{
	filestream_rejected = false;
	if (! tl_client.is_connected_to_server())
		return tl_client.connect_to_server(server_host, server_port);
	return true;
}

bool file_stream_client::start_file_transfer()
{
	int stream_size = 0;
	streampos data_offset = 0;
	int filename_len = static_cast<int>(file_name.length());
	filestream_rejected = false;
	shared_ptr<packet> pck = make_shared<packet>(packet::new_file_stream, filename_len, file_name.c_str(),
	                                             file_reader.get_total_file_size());

	cout << "sending file stream start, stream size: " << filename_len << " file size: " << file_reader.
		get_total_file_size() << endl;
	if (! tl_client.send_data_to_server(pck))
		return false;

	{
		std::unique_lock<std::mutex> lck(m);
		if (cv.wait_for(lck, std::chrono::seconds(FILE_STREAM_START_TIMEOUT)) == std::cv_status::timeout)
		{
			cout << "Server did not respond to file stream start message within timeout duration: " <<
				FILE_STREAM_START_TIMEOUT << std::endl;
			return false;
		}
	}
	if (filestream_rejected)
	{
		filestream_rejected = false;
		tl_client.disconnect_from_server();
		return false;
	}
	while (file_reader.get_remaining_file_size() > 0 && tl_client.is_connected_to_server())
	{
		data_offset = file_reader.get_offset_pos();
		stream_size = file_reader.read_file(buffer, sizeof(buffer), data_offset);

		pck = make_shared<packet>(packet::data_chunk, stream_size, buffer, data_offset);
		cout << "sending data chunk, stream size: " << stream_size << " data_offset: " << data_offset << endl;
		if (! tl_client.send_data_to_server(pck))
			return false;
	}
	if (file_reader.get_remaining_file_size() == 0)
	{
		cout << "sending file stream end, stream size: " << filename_len << " file size: " << file_reader.
			get_total_file_size() << endl;
		pck = make_shared<packet>(packet::file_stream_finish, filename_len, file_name.c_str(),
		                          file_reader.get_total_file_size());
		if (!tl_client.send_data_to_server(pck))
			return false;
		cout << "File transfer complete" << endl;
		if (wait_for_trans_finish_ack)
		{
			cout << "Waiting for transfer complete acknowledgment from server.." << endl;
			{
				std::unique_lock<std::mutex> lck(m);
				if (cv.wait_for(lck, std::chrono::seconds(FILE_STREAM_FINISH_TIMEOUT)) == std::cv_status::timeout)
				{
					cout << "Server did not respond with file stream finish ack within timeout duration: " <<
						FILE_STREAM_FINISH_TIMEOUT << std::endl;
					return false;
				}
				cout << "server response received.." << endl;
			}
		}
	}
	return true;
}

void file_stream_client::on_receive_handler(SOCKET sock, shared_ptr<packet> const packet_obj)
{
	TRY
	{
		switch (packet_obj->packet_type)
		{
		case packet::new_file_stream_ack:
			if (packet_obj->offset != 0)
			{
				cout << "File transfer will resume from offset: " << to_string(packet_obj->offset) << endl;
				file_reader.set_offset_pos(packet_obj->offset);
			}
			else
			{
				cout << "File transfer will start from the beginning.." << endl;
			}
			cv.notify_one();
			break;
		case packet::data_chunk_ack:
			//nothing to do right now
			break;
		case packet::file_stream_finish_ack:
			if (packet_obj->offset != file_reader.get_total_file_size())
				cout << "File transfer processed at server, file size: " << packet_obj->offset <<
					", different than actual file size: " << file_reader.get_total_file_size() << endl;
			else
				cout << "File transfer processed at server, file transferred successfully" << endl;
			cv.notify_one();
			stop_client();
			break;
		case packet::file_stream_rejected:
			filestream_rejected = true;
			cv.notify_one();
			cout << "File transfer rejected by the server.." << endl;
			break;
		default:
			cout << "Unexpected packet type received: " << to_string(packet_obj->packet_type) << endl;
			break;
		}
	}
	CATCH_STD
	{
		PRINT_STD;
		cv.notify_all();
		stop_client();
	}
	CATCH
	{
		PRINT_EX;
		cv.notify_all();
		stop_client();
	}
}
