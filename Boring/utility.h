#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include "main.h"

namespace utl
{
	int to_num(char c);

	char to_lower(char c);

	int rand_int(int min, int max);

	void set_cursor(int row, int column);

	void clear_line();

	void clear_screen();

	int get_int(int min, int max);

	std::string get_string(unsigned max_len = 0);

	std::wstring get_wstring(unsigned max_len = 0);
}