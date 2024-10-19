#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <queue>
#include <thread>

#include "stdafx.h"
#include "main.h"
#include "interface.h"

namespace nwk
{
	/*! ------ public enumerations ------ */

	enum Window
	{
		SERVER,
		DISPLAY,
		PLAYER
	};

	enum Phase
	{
		MENU,
		ANSWERING,
		COLLECT, // collect responses
		VOTING,
		MAX_PHASE
	};

	enum Signal
	{
		NIL, // default (no action required)
		OPEN_P, // open connection with player
		OPEN_D, // open connection with display
		CLOSE, // inform that a connection is to be closed (from player to server only)
		QUIT, // forcefully close a connection
		NAME, // set name (for player to server only)
		LEADER, // renounce leadership (from player to server)
		LEADER_R, // LEADER response - make leader (from server to player)
		GOOD, // some action was ok
		BAD, // some action was not ok
		PRINT, // clear the screen and print the whole message onto the screen
		CONNECT, // connect player to display and vice versa once both have connected
		TRANSIT, // change game phase (server is ignored)
		CACHE, // store msg in memory as a string (server will redirect cache signals to it to player and display with corresponding msg)
		ANSWER, // indicate to server the answers are being sent
		VOTE, // indicate to server the votes are being sent
		VOTE_R, // VOTE response - total votes for a particular answer from server to display
		READY, // indicate to server that the player is ready
		SEND, // tell player to send answers
		NEXT, // tell player to go to next question
		HIGHLIGHT // tell display to highlight answer
	};

	/*! ------ networking functions ------ */

	void init_wsa();

	void init_socket(Window self);

	// player to server, player to display, display to server, display to player
	void init_outgoing_addr(Window from, Window to, std::wstring const& ip = _T(""), std::string const& addr = "");

	void bind_socket(Window self);

	void set_timeout(Window self, int ms);

	// returns true if a msg could be sent and false otherwise
	bool send_msg(Window from, Window to, Signal signal, std::string msg = "", ULONG id = 0);

	// returns true if a msg was received and false otherwise
	bool recv_msg(Window self);

	// returns true if a msg was read and false otherwise
	bool read_msg(Window self);

	void close_socket(Window self);

	/*! ------ accessors and mutators ------ */

	// only used by player to open display window
	std::string get_host_addr();

	// only used by player to check if they are the leader
	bool check_leader();

	// only used by player to set is_leader to false
	void renounce_leader();

	// should only be used by player or display 
	// ok is reset to false upon checking it
	bool check_ok();

	Phase check_phase();

	// cache is cleared upon retrieving it
	std::string retrieve();

	// only for server to transit game phase
	void transit();

	// store temporary data to be sent
	void cache_string(std::string const& to_cache);

	/*! ------ functions to interface with player and display ------ */

	// only for server to get answers from all players one-by-one
	void fill_answers();

	// only for server to go to next question
	void go_next(bool is_first = false);
}