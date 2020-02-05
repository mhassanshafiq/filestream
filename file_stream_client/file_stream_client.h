#pragma once
#include "../common/file_handler.h"
#include "../common/transport_layer.h"
using namespace std;

constexpr int FILE_STREAM_START_TIMEOUT = 10;
constexpr int FILE_STREAM_FINISH_TIMEOUT = 30;
class file_stream_client
{
private:
	char buffer[BUFFER_SIZE];
private:
	transport_layer tl_client;
	file_handler file_reader;
	string server_host;
	int server_port;
	string file_name;
	bool wait_for_trans_finish_ack;
	bool filestream_rejected = false;

	void on_receive_handler(SOCKET sock, shared_ptr<packet> packet_obj);

	std::mutex m;
	std::condition_variable cv;

public:
	file_stream_client(string host, int port, string filename, string file_dir = "", bool wait_for_ack = false);
	~file_stream_client();

	bool file_stream_rejected() const { return filestream_rejected; }
	void update_filename(string filename);
	bool connect_to_server();
	bool start_file_transfer();
	bool is_file_open() const { return file_reader.is_file_open(); }
	void stop_client();
};