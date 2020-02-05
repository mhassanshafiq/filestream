// FileStream.cpp : Defines the entry point for the application.
//

#include "FileStream.h"

using namespace std;


void create_file_copies(int num_files,string filename, string dir)
{
	constexpr int BUFF_LEN = 64 * 1024;
	char BUFFER[BUFF_LEN];
	string o_dir = dir + "//loadrun";
	list<file_handler> ofiles;
	file_handler ifile(filename, file_handler::read, dir);
	for (int i = 1; i <= num_files; i++)
	{
		ofiles.emplace_back(std::move(file_handler(to_string(i) + "_" + filename, file_handler::write_new, o_dir)));

	}
	
	int len = 0;
	while (ifile.get_remaining_file_size() > 0)
	{
		len = ifile.read_file(BUFFER, BUFF_LEN, ifile.get_offset_pos());
		for (auto it = ofiles.begin(); it != ofiles.end(); ++it) 
		{
			it->write_file(BUFFER, len, it->get_offset_pos());
		}
	}
	
	ifile.close_file();
	for (auto it = ofiles.begin(); it != ofiles.end(); ++it)
	{
		it->close_file();
	}
}

void file_copy_test()
{
	char BUFFER[1024];
	file_handler ifile("testfile.jpg", file_handler::read, "D://Work//FileStream//FileStream");
	file_handler ofile("testfile_out.jpg", file_handler::write, "D://Work//FileStream//FileStream//outfile//");
	int len = 0;
	while (ifile.get_remaining_file_size() > 0)
	{
		len = ifile.read_file(BUFFER, 1024, ifile.get_offset_pos());

		ofile.write_file(BUFFER, len, ofile.get_offset_pos());
	}

	if (ifile.get_eof_pos() != ofile.get_eof_pos())
		cout << "Files not copied properly.." << endl;
	ifile.close_file();
	ofile.close_file();
}

void buffer_copy_test()
{
	std::unique_ptr<char[]> data;
	char data_buf[] = "sample data";
	int data_size = sizeof(data_buf);

	std::unique_ptr<char[]> str(new char[data_size]);
	std::copy(data_buf, data_buf + data_size, &str[0]);

	data = std::move(str);

	cout << data << endl;
	cout << data.get()<< endl;
}

void socket_transfer_test()
{
	transport_layer tl_client, tl_server;
	tl_server.start_server(9090);
	tl_server.add_on_receive_handler([&tl_server](SOCKET sock, shared_ptr<packet> packet_obj)
	{
		string reply_str = string(packet_obj->data.get()) + "_reply";
		shared_ptr<packet> reply = make_shared<packet>(packet::new_file_stream_ack, reply_str.length(),
		                                               reply_str.c_str(), 100, sock, "test");
		tl_server.send_data_to_client(sock, packet_obj->remote_host, reply);
	});

	tl_client.connect_to_server("127.0.0.1", 9090);
	string message = "this_is_a_test";
	string expected_reply = message + "_reply";
	tl_client.add_on_receive_handler([&expected_reply](SOCKET sock, shared_ptr<packet> packet_obj)
	{
		string server_reply = (packet_obj->data.get());
		if (server_reply != expected_reply)
		{
			cout << "test failed..u suck!" << endl; return;
		}
		if (packet_obj->data_size != expected_reply.length())
		{
			cout << "test failed..seriously?" << endl; return;
		}
		if (packet_obj->offset != 100)
		{
			cout << "test failed.. come on man!" << endl; return;
		}		
		cout << "test passed..u rock!" << endl;
	});

	shared_ptr<packet> test_msg = make_shared<packet>(packet::new_file_stream, message.length(),
		message.c_str(), 0);
	tl_client.send_data_to_server(test_msg);


	Sleep(1000);
	tl_server.stop_server();
	Sleep(1000);
}

int main()
{
	int i;
	//file_copy_test();
	//buffer_copy_test();
	//socket_transfer_test();
	std::cin >> i;

	return 0;
}