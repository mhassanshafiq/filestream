#pragma once
#include "../common/file_handler.h"
#include "../common/transport_layer.h"
#include <queue>
#include <atomic>
#include <unordered_map>
using namespace std;

class file_stream_server
{
private:
	string file_directory;
	transport_layer tl_server;	
	int server_port;
	bool running = false;

	void on_receive_handler(SOCKET sock, shared_ptr<packet> packet_obj);
	void on_connect_handler(SOCKET sock, string remote_host);
	void on_disconnect_handler(SOCKET sock, string remote_host);

	void process_filestream_packet(string filename, string remote_host);
	void process_packet_queue();
	void send_filestream_rejected(SOCKET sock, string const remote_host, string const filename);
	void remove_host_and_file(string filename, string remote_host);

	unordered_map<string, string> remote_host__filename_map;
	unordered_map<string, queue<shared_ptr<packet>>> filename__packet_q_map;
	queue<shared_ptr<packet>> pending_packets_q;
	atomic_flag pending_packets_flag = ATOMIC_FLAG_INIT;
	mutex remote_host__filename_lock;
	mutex filename__packet_q_lock;
	mutex pp_m;
	condition_variable pp_cv;
	mutex fp_m;
	condition_variable fp_cv;
	

public:
	file_stream_server(int port, string file_dir = "");
	~file_stream_server();

	void start_server();
	void stop_server();
};