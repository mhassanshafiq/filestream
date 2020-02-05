#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include"exceptions.h"
using namespace std;

class file_handler
{
public:
	typedef enum
	{
		read=0,
		write,
		write_new
	}file_modes;
	file_handler(string filename, file_modes mode, string file_directory = "");
	file_handler(file_handler& filehandler);
	~file_handler();
	
private:
	string file_name;
	fstream file_handle;
	streampos file_offset;
	file_modes file_mode;
	long long total_file_size;
	long long rem_file_size;
	string file_directory;
	
public:

	void set_file_directory(string dir) { file_directory = dir; }
	string get_file_name() const { return file_name; }
	bool is_file_open() const { return file_handle.is_open(); }
	streampos get_offset_pos() const { return file_offset; }	
	long long get_remaining_file_size() const { return rem_file_size; }
	long long get_total_file_size() const { return total_file_size; }
	streampos get_eof_pos();
	long long calc_remaining_file_size();
	void set_offset_pos(streampos offset);
	int read_file(char * buf, int len, streampos offset);
	bool write_file(char* buf, int len, streampos offset);
	void close_file();
	
};