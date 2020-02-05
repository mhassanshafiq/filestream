#pragma once
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include"exceptions.h"
using namespace std;
class config_manager
{
public:
	typedef enum
	{
		server,
		client
	}process_types;
	process_types processtype;
	int server_port = 5050;
	string server_host = "127.0.0.1";
	string server_file_dir = "";

	string file_to_transfer = "testfile.jpg";
	string client_file_dir = "";
	int connection_retry = 3;
	int transmission_retry = 5;
	int retry_interval = 5 * 1000;
	bool wait_for_ack = false;


	config_manager::config_manager(config_manager& config_mgr)
	{
		processtype = config_mgr.processtype;
		server_port = config_mgr.server_port;
		server_host = config_mgr.server_host;
		server_file_dir = config_mgr.server_file_dir;

		file_to_transfer = config_mgr.file_to_transfer;
		client_file_dir = config_mgr.client_file_dir;
		connection_retry = config_mgr.connection_retry;
		transmission_retry = config_mgr.transmission_retry;
		retry_interval = config_mgr.retry_interval;
		wait_for_ack = config_mgr.wait_for_ack;
	}
	config_manager::config_manager(process_types type): processtype(type)
	{
		char buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		string::size_type pos = string(buffer).find_last_of("\\/");
		string config_file = string(buffer).substr(0, pos) + "\\" + "config.txt";
		ifstream file(config_file, ios_base::in);
		if (file.is_open())
		{
			string config_string;
			while (!file.eof())
			{
				file >> config_string;
				if (config_string == "server_port")
					file >> server_port;
				else if (config_string == "server_host")
					file >> server_host;
				else if (config_string == "server_file_dir")
					file >> server_file_dir;
				else if (config_string == "file_to_transfer")
					file >> file_to_transfer;
				else if (config_string == "client_file_dir")
					file >> client_file_dir;
				else if (config_string == "connection_retry")
					file >> connection_retry;
				else if (config_string == "transmission_retry")
					file >> transmission_retry;
				else if (config_string == "retry_interval")
				{
					file >> retry_interval;
					retry_interval *= 1000;
				}
				else if (config_string == "wait_for_ack")
					file >> wait_for_ack;

			}
			file.close();
			cout << "configurations loaded.." << endl;
			if(processtype)cout << "server_host: " << server_host << endl;
			cout << "server_port: " << server_port << endl;			
			cout << "server_file_dir: " << server_file_dir << endl;
			if (processtype)
			{
				cout << "file_to_transfer: " << file_to_transfer << endl;
				cout << "client_file_dir: " << client_file_dir << endl;
				cout << "connection_retry: " << connection_retry << endl;
				cout << "transmission_retry: " << transmission_retry << endl;
				cout << "retry_interval: " << retry_interval << endl;
				cout << "wait_for_ack: " << wait_for_ack << endl;
			}
			
		}
		else
			cout << "unable to open config file.."<<endl;
	}
};