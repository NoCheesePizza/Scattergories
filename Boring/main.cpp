#include <iostream>
#include <thread>

#include "stdafx.h"
#include "main.h"
#include "network.h"
#include "loader.h"
#include "utility.h"

/*! ------ Notes ------ */

/* 
	q = questions
	a = answers
*/

/*! ------ Global variables ------ */

nwk::Window window;

/*! ------ Global functions ------ */

using namespace std::literals::chrono_literals;

void recv_read_msg(nwk::Window self, nwk::Phase curr)
{
	while (nwk::check_phase() == curr)
	{
		nwk::recv_msg(self);
		nwk::read_msg(self);
	}
}

int main(int argc, char *argv[])
{
	// set window name
	wchar_t consoleTitle[MAX_CHAR];
	wsprintf(consoleTitle, _T("Really Boring Console"));
	SetConsoleTitle(static_cast<LPCTSTR>(consoleTitle));

	// print sensitive info to flex
	std::cout << T_MAGENTA;
	nwk::init_wsa();
	SYSTEM_POWER_STATUS status;
	GetSystemPowerStatus(&status);
	std::cout << "Battery power: " << static_cast<int>(status.BatteryLifePercent) << "%\n\n" << RESET;

	if (argc == 1) // server/player
	{
		// print main menu
		std::cout << "1: Host game\n2: Join game\n3: Leave game\n";
		int choice = utl::get_int(1, 3);
		if (choice == 3)
			exit(0);
		utl::clear_screen();
		window = choice == 1 ? nwk::SERVER : nwk::PLAYER;
	}
	else if (argc == 2) // player
		window = nwk::PLAYER;
	else if (argc == 3) // display
		window = nwk::DISPLAY;
	else
		exit(0);

	nwk::init_socket(window);
	nwk::bind_socket(window);

	switch (window)
	{
	case nwk::SERVER:
	{
		nwk::set_timeout(nwk::SERVER, TO_LONG);
		system("start Boring.exe 1"); // open another instance in "player" window
		itf::load_q();

		while (true)
		{
			recv_read_msg(nwk::SERVER, nwk::MENU);
			
			itf::get_game_data();
			itf::set_rand_q();
			itf::send_rand_q();
			itf::send_timer();

			// answer -> collect -> ready -> load responses -> vote

			nwk::transit();
			nwk::send_msg(nwk::SERVER, nwk::PLAYER, nwk::TRANSIT);
			nwk::send_msg(nwk::SERVER, nwk::DISPLAY, nwk::TRANSIT);
			recv_read_msg(nwk::SERVER, nwk::COLLECT);

			nwk::fill_answers();
			nwk::go_next(true);
			recv_read_msg(nwk::SERVER, nwk::VOTING);
		}

		// server sets curr_q to 1
		// server sends NEXT signal to display (display also stores curr_q), and display prints appropriate q
		// server sends player names and non-empty answers to display, and display stores them in a vector of structs (the vector is erased at the start of each q)
		// server also creates vector of player ids containing only those that answered? - ids
		// player constantly awaits for user input
		// if input starts with a slash, an appropriate command is sent to display, else the vote is sent to server
		// eg. 2 when answer 1 is highlighted -> server.players.votes[curr_q][input1] = input2
		// add the votes for 1 answer: for each player, votes += player.votes[curr_q][answer]
		// server sends back sum of votes for that answer, and display updates the sum if it has changed

		break;
	}
	case nwk::PLAYER:
	{
		std::cout << "Enter the IPv4 address of the server:\n";
		nwk::set_timeout(nwk::PLAYER, TO_SHORT);

		do
		{
			nwk::init_outgoing_addr(nwk::PLAYER, nwk::SERVER, utl::get_wstring());
			nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::OPEN_P);
			nwk::recv_msg(nwk::PLAYER);
			nwk::read_msg(nwk::PLAYER);
		} while (!nwk::check_ok());

		nwk::set_timeout(nwk::PLAYER, TO_LONG);
		utl::clear_screen();
		std::cout << "Enter your username:\n";
		nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::NAME, utl::get_string(N_WIDTH));

		// open another instance in "display" window
		system(std::string("start Boring.exe 1 " + nwk::get_host_addr()).c_str());

		while (true)
		{
			std::thread menu(recv_read_msg, nwk::PLAYER, nwk::MENU); // in case player becomes leader
			std::this_thread::sleep_for(1s); // wait for leader message
			itf::print_menu(); // will only print "waiting for players..." if player is ready
			menu.join();
			utl::clear_screen();

			std::thread answer(recv_read_msg, nwk::PLAYER, nwk::ANSWERING);

			std::cout << "Loading questions.";
			std::this_thread::sleep_for(1s);
			std::cout << '.';
			std::this_thread::sleep_for(1s);
			std::cout << '.';
			std::this_thread::sleep_for(1s);
			utl::clear_screen();

			itf::get_rand_q();
			while (nwk::check_phase() == nwk::ANSWERING)
				itf::get_answer();

			answer.join();

			utl::clear_screen();
			itf::cache_answers();

			send_msg(nwk::PLAYER, nwk::SERVER, nwk::READY);
			recv_read_msg(nwk::PLAYER, nwk::COLLECT);
			utl::clear_screen();

			std::thread vote(recv_read_msg, nwk::PLAYER, nwk::VOTING);

			std::cout << "Loading responses.";
			std::this_thread::sleep_for(1s);
			std::cout << '.';
			std::this_thread::sleep_for(1s);
			std::cout << '.';
			std::this_thread::sleep_for(1s);
			utl::clear_screen();

			std::cout << "Rate your friends' answers on a scale of -2 to 2\n";
			while (nwk::check_phase() == nwk::VOTING)
				itf::get_vote();

			vote.join();
		}

		break;
	}
	case nwk::DISPLAY:
	{
		nwk::set_timeout(nwk::DISPLAY, TO_LONG);
		nwk::init_outgoing_addr(nwk::DISPLAY, nwk::SERVER, L"", argv[2]);
		nwk::send_msg(nwk::DISPLAY, nwk::SERVER, nwk::OPEN_D);

		// test for connection (sometimes doesn't connect for no reason)
		std::cout << "Due to network congestion, a connection between the display console and the server could not be established\nTry closing this window and opening another display console";

		while (true)
		{
			recv_read_msg(nwk::DISPLAY, nwk::MENU);
			recv_read_msg(nwk::DISPLAY, nwk::ANSWERING);
			std::cout << "\nEnter any key to proceed\nAny changes made from now onwards will not be recorded\n";
			recv_read_msg(nwk::DISPLAY, nwk::COLLECT);

			std::this_thread::sleep_for(3s);
			itf::get_visibility();
			while (nwk::check_phase() == nwk::VOTING)
			{
				itf::get_q_data();
				while (!nwk::check_ok())
				{
					recv_msg(nwk::DISPLAY);
					read_msg(nwk::DISPLAY);
				}
			}
		}

		break;
	}
	}

	debugf(std::cout << "fin\n");
	debugf(std::cin.get());
}