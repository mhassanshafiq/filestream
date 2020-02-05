
#include <string>
#include <vector>
#include "..\file_stream_client/filestreamclient.h"
using namespace std;


void create_file_copies(int num_files, string filename, string dir)
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
		streampos offset = ifile.get_offset_pos();
		len = ifile.read_file(BUFFER, BUFF_LEN, offset);
		for (auto it = ofiles.begin(); it != ofiles.end(); ++it)
		{
			it->write_file(BUFFER, len, offset);
		}
	}

	ifile.close_file();
	for (auto it = ofiles.begin(); it != ofiles.end(); ++it)
	{
		it->close_file();
	}
}

void run(filestreamclient& client)
{
	client.run();
}
void multiple_clients()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	int num_clients = 100;
	//string dir;// = "D://Work//FileStream//FileStream";
	string filename = "testfile.jpg";
	
	string dir = string(buffer).substr(0, pos);
	
	
	create_file_copies(num_clients, filename, dir);
	vector<filestreamclient> clients;
	vector<thread> threads;

	for (int i = 1; i <= num_clients; i++)
	{
		clients.emplace_back(filestreamclient(to_string(i) + "_"));

	}
	for (int i = 0; i < clients.size(); i++)
	{
		threads.emplace_back(thread(run, clients[i]));
	}

	for (int i = 0; i < clients.size(); i++)
	{
		threads[i].join();
	}
}

int main()
{
	int i;
	multiple_clients();
	std::cin >> i;

	return 0;
}