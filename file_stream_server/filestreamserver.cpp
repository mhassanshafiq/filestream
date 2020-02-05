#include "file_stream_server.h"
#include "..\common/config_manager.h"

int main()
{
	cout << "****************************************" << endl;
	cout << "***********file_stream_server***********" << endl;
	cout << "****************************************" << endl << endl << endl;
		
	config_manager config(config_manager::server);
	
	file_stream_server server(config.server_port, config.server_file_dir);
	
	server.start_server();

	while (1)
	{
		char key;
		cout << "press 'q' to exit server..\n"; cin >> key;
		if (key == 'q' || key == 'Q')
			break;
	}

	return 0;
}