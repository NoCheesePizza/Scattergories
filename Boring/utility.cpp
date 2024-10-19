#include "utility.h"

namespace utl
{
	int to_num(char c)
	{
		return (c >= '0' && c <= '9') ? c - '0' : c;
	}

	char to_lower(char c)
	{
		return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
	}

	int rand_int(int min, int max)
	{
		return static_cast<int>(rand() / static_cast<double>(RAND_MAX) * (max - min) + min);
	}

	void set_cursor(int row, int column)
	{
		std::cout << "\033[" << row << ';' << column << 'H';
	}

	void clear_line()
	{
		std::cout << "\033[A\33[2K";
	}

	void clear_screen()
	{
		system("cls");
	}

	int get_int(int min, int max)
	{
		int choice;
		std::string temp;

		do
		{	
			while (true)
			{
				std::istringstream iss;
				std::getline(std::cin, temp);
				clear_line();
				iss.str(temp);
				iss >> choice;
				if (iss)
					break;
			}
		} while (choice < min || choice > max);

		return choice;
	}

	std::string get_string(unsigned max_len)
	{
		std::string ret;
		while (!ret.size()) // reprompt if player just presses enter
		{
			std::getline(std::cin, ret);
			clear_line();
		}
		if (ret.size() >= max_len && max_len)
			ret.erase(max_len - 1);
		return ret;
	}

	std::wstring get_wstring(unsigned max_len)
	{
		std::wstring ret;
		while (!ret.size()) // disallow empty response
		{
			std::getline(std::wcin, ret);
			clear_line();
		}
		if (ret.size() >= max_len && max_len)
			ret.erase(max_len - 1);
		return ret;
	}
}