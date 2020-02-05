#include "file_stream_client.h"
#include "../common/config_manager.h"
#include "filestreamclient.h"

int main()
{
	cout << "****************************************" << endl;
	cout << "***********file_stream_client***********" << endl;
	cout << "****************************************" << endl << endl << endl;

	filestreamclient client;
	client.run();
	
	while (1)
	{
		char key;
		cout << "press 'q' to exit server..\n"; cin >> key;
		if (key == 'q' || key == 'Q')
			break;
	}
	return 0;
}
