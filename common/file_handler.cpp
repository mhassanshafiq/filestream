#include "file_handler.h"

#define WRITE_F (ios::out | ios::app | ios::binary)
#define WRITE_NEW_F (ios::out | ios::binary)
#define READ_F (ios::in | ios::binary)

file_handler::file_handler(string filename, file_modes mode, string file_directory) :
	file_directory(file_directory),
	file_name(filename),
	file_mode(mode)
{	
	if (mode == file_handler::read || mode == file_handler::write || mode == file_handler::write_new)
	{
		if(mode==write_new) file_handle = fstream(file_directory + "//" + filename, WRITE_NEW_F);
		else if(mode == write) file_handle = fstream(file_directory + "//" + filename, WRITE_F);
		else  file_handle = fstream(file_directory + "//" + filename, READ_F);
		if (file_handle.is_open() == false)
		{
			cout << "Unable to open file: " << file_directory + "//" + filename << endl;
		}
		file_handle.seekg(0, ios::end);
		total_file_size = rem_file_size = file_handle.tellg();
		file_handle.seekg(0, ios::beg);
	}
	else
	{
		cout << "File mode is unknown.." << endl;
	}
}

file_handler::file_handler(file_handler& filehandler) : file_name(filehandler.file_name),
file_offset(filehandler.file_offset),
file_mode(filehandler.file_mode),
file_directory(filehandler.file_directory)

{
	file_handle = std::move(filehandler.file_handle);
	total_file_size = filehandler.total_file_size;
	rem_file_size = filehandler.rem_file_size;
}

file_handler::~file_handler()
{
	close_file();
}

streampos file_handler::get_eof_pos()
{
	file_handle.seekg(0, ios::end);
	file_handle.seekp(0, ios::end);
	return file_handle.tellp();
}

long long file_handler::calc_remaining_file_size()
{
	file_handle.seekg(0, ios::end);
	streampos rem_len = file_handle.tellg() - file_offset;
	file_handle.seekg(file_offset, ios::beg);

	return static_cast<long long>(rem_len);
}

void file_handler::set_offset_pos(streampos offset)
{
	if (offset <= total_file_size) {
		file_handle.seekg(offset, ios::beg);
		file_handle.seekp(offset, ios::beg);
		file_offset = file_handle.tellg();
		rem_file_size = total_file_size - file_offset;
	}
	else
	{
		cout << "offset is bigger than file size" << endl;
	}
}

int file_handler::read_file(char* buf, int len, streampos offset)
{
	if (offset != file_handle.tellg()) 
	{
		cout << "offset changed" << endl;
		file_handle.seekg(offset, ios::beg);
		file_offset = file_handle.tellg();
		rem_file_size = total_file_size - file_offset;		
	}
	if (len > rem_file_size)
		len = rem_file_size;
	file_handle.read(buf, len);
	if (!file_handle)
	{
		std::cout << "error: only " << file_handle.gcount() << " could be read";
		file_handle.close();
		rem_file_size = 0;
		return 0;
	}
	file_offset = file_handle.tellg();
	rem_file_size = total_file_size - file_offset;
	return len;
}

bool file_handler::write_file(char* buf, int len, streampos offset)
{
	TRY
	{
		streampos curr_offset = file_handle.tellp();
		if (offset != file_handle.tellp())
			file_handle.seekp(offset, ios::beg);
		file_handle.write(buf, len);
		file_offset = file_handle.tellp();
		total_file_size += len;
	}
	CATCH_STD
	{
		PRINT_STD;
		return false;
	}
	return true;
}

void file_handler::close_file()
{
	if (file_handle.is_open())
	{
		file_handle.close();
		cout << "Closing file: " << file_name << endl;
	}
}
