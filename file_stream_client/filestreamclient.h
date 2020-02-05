
#include <string>
#include "../common/config_manager.h"
#include "file_stream_client.h"

class filestreamclient
{
	config_manager config;
	std::string file_dir;
	std::string server_host;
	int server_port;
	std::string filename;
	int wait_for_ack;
	int connection_try = 0;
	int transmission_try = 0;
	int retry_interval;
	bool transmission_complete = false;

public:
	
	filestreamclient(string prefix = "") :
		config(config_manager::client)
	{

		file_dir = config.client_file_dir;
		server_host = config.server_host;
		server_port = config.server_port;
		filename = prefix + config.file_to_transfer;
		wait_for_ack = config.wait_for_ack;
		retry_interval = config.retry_interval;
	}
	
	filestreamclient(filestreamclient& client) :
		config(config_manager::client)
	{
		config = client.config;
		file_dir = client.file_dir;
		server_host = client.server_host;
		server_port = client.server_port;
		filename = client.filename;
		wait_for_ack = client.wait_for_ack;
		retry_interval = client.retry_interval;
	}

	void run()
	{
		file_stream_client client(server_host, server_port, filename, file_dir, wait_for_ack);
		if (client.is_file_open()) {

			while (!transmission_complete && !client.file_stream_rejected())
			{
				if (client.connect_to_server())
				{
					connection_try = 0;
					if (client.start_file_transfer())
					{
						transmission_complete = true;
					}
					else if (transmission_try++ < config.transmission_retry)
					{
						cout << "Unable to transmit file: " << filename << ", retrying in " << retry_interval << " milliseconds" << endl;
					}
					else break;

				}
				else
				{
					cout << "Unable to connect to host " << server_host << ":" << server_port << endl;
					connection_try++;
					if (connection_try <= config.connection_retry)
					{
						cout << "retrying connection to host" << server_host << ":" << server_port << " in " << retry_interval << " milliseconds" << endl;
					}
					else break;
				}
				Sleep(retry_interval);
			}
		}
		if (transmission_complete)
		{
			cout << "File: '" << filename << "' transmitted successfully.. :)" << endl;
		}
		else
		{
			cout << "File: '" << filename << "' transmit unsuccessful.. :(" << endl;
		}
		client.stop_client();
	}
};
