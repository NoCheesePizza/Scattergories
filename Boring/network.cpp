#include "network.h"

namespace nwk
{
	/*! ------ private data types ------ */

	enum class Port
	{
		SERVER = 5555,
		DISPLAY,
		PLAYER
	};

	//todo remember to erase all memory in all vectors at the end of each round cos push_back was used

	struct Conn
	{
		SOCKET socket; // always use self port to send/recv msgs
		sockaddr_in addr; // always use other addr to send msgs to particular window
		bool is_ready = false;

		std::string name;
		std::vector<std::string> answers;
		std::vector<int> votes; // for one question
		int score = 0, // total accumulative score
			curr_score = EMPTY; // score this round only

		Conn() = default;

		void init(ULONG const& sin_addr, Window self)
		{
			addr.sin_addr.s_addr = sin_addr;
			addr.sin_port = htons(static_cast<u_short>(self == PLAYER ? Port::PLAYER : Port::DISPLAY));
			addr.sin_family = AF_INET;
			name = reinterpret_cast<char const*>(&sin_addr);
		}
	};

	struct Server
	{
		std::map<ULONG, Conn> players, displays;
		Conn server;
		std::queue<std::pair<ULONG, std::string>> msgs; // can't tell player and display msgs apart
		std::vector<ULONG> curr_id; // id of players who answered curr q as viewed in the same order as display
	};

	struct Player
	{
		Conn server, player, display;
		std::queue<std::string> msgs;
	};

	struct Display
	{
		Conn server, display, player;
		std::queue<std::string> msgs;
	};

	/*! ------ global variables ------ */

	sockaddr_in local_addr;

	Server server;
	Player player;
	Display display;

	bool is_ok = false;

	int counter; // count_if for get_names() function

	Phase phase = MENU;

	std::string cache = "",
				to_send = "";

	ULONG leader; // for server
	bool is_leader = false; // for player

	std::string const signals[] // for debug text
	{
		"NIL",
		"OPEN_P",
		"OPEN_D",
		"CLOSE",
		"QUIT",
		"NAME",
		"LEADER",
		"LEADER_R",
		"GOOD",
		"BAD",
		"PRINT",
		"CONNECT",
		"TRANSIT",
		"CACHE",
		"ANSWER",
		"VOTE",
		"VOTE_R",
		"READY",
		"SEND",
		"NEXT",
		"HIGHLIGHT"
	};

	/*! ------ auxiliary functions ------ */

	using namespace std::literals::chrono_literals;

	bool if_true(Conn const& conn)
	{
		return true;
	}

	bool if_not_ready(Conn const& conn)
	{
		return !conn.is_ready;
	}

	struct if_not_empty
	{
		int a_num;

		if_not_empty(int _a_num) 
			: a_num{ _a_num }
		{

		}

		bool operator()(std::pair<ULONG const, Conn> const& elem) const
		{
			return elem.second.votes[a_num - 1] != EMPTY;
		}
	};

	/*! ------ private functions ------ */

	ULONG make_id(sockaddr_in const& addr)
	{
		return addr.sin_addr.s_addr;
	}

	void print_addr(sockaddr_in const& addr)
	{
		std::cout << static_cast<unsigned>(addr.sin_addr.s_addr) << ' ' << static_cast<unsigned>(addr.sin_addr.s_host) << ' ' << static_cast<unsigned>(addr.sin_addr.s_net) << ' ' << static_cast<unsigned>(addr.sin_addr.s_imp) << ' ' << static_cast<unsigned>(addr.sin_addr.s_impno) << ' ' << static_cast<unsigned>(addr.sin_addr.s_lh) << ' ' << static_cast<unsigned>(addr.sin_port) << nl;
	}

	// for server use only
	void send_names()
	{
		std::ostringstream oss;
		oss << T_YELLOW << "    " << std::left << std::setw(N_WIDTH) << "Players" << "Score\n" << RESET;
		int i = 0;
		for (std::pair<ULONG const, Conn> const& player : server.players)
			oss << std::left << std::setw(2) << ++i << ". " << (player.first == leader ? T_BLUE : "") << std::setw(N_WIDTH) << player.second.name << RESET << std::setw(V_WIDTH) << player.second.score << (player.second.curr_score == EMPTY ? "\n" : " [+" + std::to_string(player.second.curr_score) + "]\n");
		send_msg(SERVER, DISPLAY, PRINT, oss.str());
	}

	// for server use only
	std::string get_names(bool(*func)(Conn const&) = if_true)
	{
		std::string names;
		counter = 0;
		for (std::pair<ULONG const, Conn> const& player : server.players)
			if (func(player.second))
				names += std::to_string(++counter) + ". " + player.second.name + nl;
		return names;
	}

	// for server use only
	void set_new_leader()
	{
		std::map<ULONG const, Conn>::iterator it = server.players.find(leader);
		if (++it == server.players.end())
			leader = server.players.begin()->first;
		else
			leader = it->first;
		send_names();
		send_msg(SERVER, PLAYER, LEADER_R, "", leader);
	}

	/*! ------ networking functions ------ */

	void init_wsa()
	{
		// initialise windows socket API
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
			debugf(std::cout << "Winsock DLL not found\n");

		// get name of local machine
		char host[MAX_CHAR];
		gethostname(host, MAX_CHAR);

		struct addrinfo hints, * result;
		ZeroMemory(&hints, sizeof(hints));

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		// get address of local machine
		getaddrinfo(host, nullptr, &hints, &result);
		char ip[MAX_CHAR]{ 0 };
		inet_ntop(AF_INET, &result->ai_addr->sa_data[2], ip, sizeof(ip));
		local_addr = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
		local_addr.sin_family = AF_INET;

		std::cout << "Local machine: " << host << "\nLocal address: " << ip << nl;
	}

	void init_socket(Window self)
	{
		// create socket using TCP because it's more reliable and easy to use
		// and 100 other jokes to tell yourself

		switch (self)
		{
		case SERVER:

			server.server.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (server.server.socket == INVALID_SOCKET)
				std::cout << "Error at server.server.socket(): " << WSAGetLastError() << nl;
			break;

		case PLAYER:

			player.player.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (player.player.socket == INVALID_SOCKET)
				std::cout << "Error at player.player.socket(): " << WSAGetLastError() << nl;
			break;

		case DISPLAY:

			display.display.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (display.display.socket == INVALID_SOCKET)
				std::cout << "Error at display.display.socket(): " << WSAGetLastError() << nl;
			break;
		}
	}

	void init_outgoing_addr(Window from, Window to, std::wstring const& ip, std::string const& addr)
	{
		switch (from)
		{
		case PLAYER:

			switch (to)
			{
			case SERVER:

				if (ip.empty())
					player.server.addr.sin_addr = *reinterpret_cast<IN_ADDR const*>(addr.c_str());
				else
					InetPton(AF_INET, ip.c_str(), &player.server.addr.sin_addr.s_addr);

				player.server.addr.sin_port = htons(static_cast<u_short>(Port::SERVER));
				player.server.addr.sin_family = AF_INET;
				break;

			case DISPLAY:

				if (ip.empty())
					player.display.addr.sin_addr = *reinterpret_cast<IN_ADDR const*>(addr.c_str());
				else
					InetPton(AF_INET, ip.c_str(), &player.display.addr.sin_addr.s_addr);

				player.display.addr.sin_port = htons(static_cast<u_short>(Port::DISPLAY));
				player.display.addr.sin_family = AF_INET;
				break;
			}
			break;

		case DISPLAY:

			switch (to)
			{
			case SERVER:

				if (ip.empty())
					display.server.addr.sin_addr = *reinterpret_cast<IN_ADDR const*>(addr.c_str());
				else
					InetPton(AF_INET, ip.c_str(), &display.server.addr.sin_addr.s_addr);
				
				display.server.addr.sin_port = htons(static_cast<u_short>(Port::SERVER));
				display.server.addr.sin_family = AF_INET;
				break;

			case PLAYER:

				if (ip.empty())
					display.player.addr.sin_addr = *reinterpret_cast<IN_ADDR const*>(addr.c_str());
				else
					InetPton(AF_INET, ip.c_str(), &display.player.addr.sin_addr.s_addr);

				display.player.addr.sin_port = htons(static_cast<u_short>(Port::PLAYER));
				display.player.addr.sin_family = AF_INET;
				break;
			}
			break;
		}
	}

	void bind_socket(Window self)
	{
		switch (self)
		{
		case SERVER:

			local_addr.sin_port = htons(static_cast<u_short>(Port::SERVER));
			if (bind(server.server.socket, reinterpret_cast<SOCKADDR*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR)
				debugf(std::cout << "Error at server.server.socket.socket(): " << WSAGetLastError() << nl);
			break;

		case PLAYER:

			local_addr.sin_port = htons(static_cast<u_short>(Port::PLAYER));
			if (bind(player.player.socket, reinterpret_cast<SOCKADDR*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR)
				debugf(std::cout << "Error at player.player.socket.socket(): " << WSAGetLastError() << nl);
			break;

		case DISPLAY:

			local_addr.sin_port = htons(static_cast<u_short>(Port::DISPLAY));
			if (bind(display.display.socket, reinterpret_cast<SOCKADDR*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR)
				debugf(std::cout << "Error at display.display.socket.socket(): " << WSAGetLastError() << nl);
			break;
		}
	}

	void set_timeout(Window self, int ms)
	{
		setsockopt(self == SERVER ? server.server.socket : self == PLAYER ? player.player.socket : display.display.socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&ms), sizeof(int));
	}

	bool send_msg(Window from, Window to, Signal signal, std::string msg, ULONG id)
	{
		msg = std::to_string(signal) + ' ' + msg;

		switch (from)
		{
		case SERVER:

			switch (to)
			{
			case PLAYER:

				if (!id)
				{
					for (std::pair<ULONG const, Conn> const& player : server.players)
						if (sendto(server.server.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&player.second.addr), sizeof(player.second.addr)) == SOCKET_ERROR)
							return false;
				}
				else
					if (sendto(server.server.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&server.players[id].addr), sizeof(server.players[id].addr)) == SOCKET_ERROR)
						return false;

				break;

			case DISPLAY:

				if (!id)
				{
					for (std::pair<ULONG const, Conn> const& display : server.displays)
						if (sendto(server.server.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&display.second.addr), sizeof(display.second.addr)) == SOCKET_ERROR)
							return false;
				}
				else
					if (sendto(server.server.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&server.displays[id].addr), sizeof(server.displays[id].addr)) == SOCKET_ERROR)
						return false;
				break;
			}
			break;

		case PLAYER:

			switch (to)
			{
			case SERVER:

				if (sendto(player.player.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&player.server.addr), sizeof(player.server.addr)) == SOCKET_ERROR)
					return false;
				break;

			case DISPLAY:

				if (sendto(player.player.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&player.display.addr), sizeof(player.display.addr)) == SOCKET_ERROR)
					return false;
				break;
			}
			break;

		case DISPLAY:

			switch (to)
			{
			case SERVER:

				if (sendto(display.display.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&display.server.addr), sizeof(display.server.addr)) == SOCKET_ERROR)
					return false;
				break;

			case PLAYER:

				if (sendto(display.display.socket, msg.c_str(), MAX_CHAR, 0, reinterpret_cast<SOCKADDR const*>(&display.player.addr), sizeof(display.player.addr)) == SOCKET_ERROR)
					return false;
				break;
			}
			break;
		}

		if (from == SERVER)
			std::cout << "SERVER sent msg to " << (to == PLAYER ? "PLAYER" : "DISPLAY") << " with " << signals[signal] << " signal\n";
		else
			debugf(std::cout << (from == PLAYER ? "PLAYER" : "DISPLAY") << " sent msg to " << (to == SERVER ? "SERVER" : to == PLAYER ? "PLAYER" : "DISPLAY") << " with " << signals[signal] << " signal\n");
		return true;
	}

	bool recv_msg(Window self)
	{
		sockaddr_in addr;
		int addr_len = sizeof(addr);
		char msg[MAX_CHAR]{ 0 };

		switch (self)
		{
		case SERVER:

			// server doesn't know whether it's receiving a msg from player or display so it's up to the signal to inform it
			if (recvfrom(server.server.socket, msg, MAX_CHAR, 0, reinterpret_cast<SOCKADDR*>(&addr), &addr_len) == SOCKET_ERROR)
				return false;
			server.msgs.push({ make_id(addr), msg });	
			break;

		case PLAYER:

			if (recvfrom(player.player.socket, msg, MAX_CHAR, 0, reinterpret_cast<SOCKADDR*>(&addr), &addr_len) == SOCKET_ERROR)
				return false;
			player.msgs.push(msg);
			break;

		case DISPLAY:

			if (recvfrom(display.display.socket, msg, MAX_CHAR, 0, reinterpret_cast<SOCKADDR*>(&addr), &addr_len) == SOCKET_ERROR)
				return false;
			display.msgs.push(msg);
			break;
		}

		if (self == SERVER)
			std::cout << "SERVER received msg: " << msg << nl;
		else
			debugf(std::cout << (self == PLAYER ? "PLAYER" : "DISPLAY") << " received msg: " << msg << nl);
		return true;
	}

	bool read_msg(Window self)
	{
		std::pair<ULONG, std::string> s_msg; // server message
		std::string msg;
		int signal = NIL;

		switch (self)
		{
		case SERVER:
		{
			if (server.msgs.empty())
				return false;

			s_msg = server.msgs.front();
			server.msgs.pop();
			std::istringstream iss(s_msg.second);
			iss >> signal;
			s_msg.second = s_msg.second.substr(static_cast<int>(iss.tellg()) + 1);
			break;
		}
		case PLAYER:
		{
			if (player.msgs.empty())
				return false;

			msg = player.msgs.front();
			player.msgs.pop();
			std::istringstream iss(msg);
			iss >> signal;
			msg = msg.substr(static_cast<int>(iss.tellg()) + 1);
			break;
		}
		case DISPLAY:

			if (display.msgs.empty())
				return false;

			msg = display.msgs.front();
			display.msgs.pop();
			std::istringstream iss(msg);
			iss >> signal;
			msg = msg.substr(static_cast<int>(iss.tellg()) + 1);
			break;
		}

		if (self == SERVER)
			std::cout << "SERVER read msg with " << signals[signal] << " signal\n";
		else
			debugf(std::cout << (self == SERVER ? "SERVER" : self == PLAYER ? "PLAYER" : "DISPLAY") << " read msg with " << signals[signal] << " signal\n");

		switch (signal)
		{
		case NIL:

			break;

		case OPEN_P:
		{
			server.players[s_msg.first].init(s_msg.first, PLAYER);
			send_msg(SERVER, PLAYER, GOOD, "", s_msg.first);
			break;
		}
		case OPEN_D:
		{
			// make current player leader if they are the only player who has entered their name
			if (server.players.size() == 1)
			{
				leader = s_msg.first;
				send_msg(SERVER, PLAYER, LEADER_R);
			}

			server.displays[s_msg.first].init(s_msg.first, DISPLAY);
			send_names();

			// connect display with player once the display window is opened
			char addr[MAX_CHAR]{ 0 };
			memcpy(addr, &server.displays[s_msg.first].addr.sin_addr, sizeof(server.displays[s_msg.first].addr.sin_addr));
			send_msg(SERVER, PLAYER, CONNECT, addr, s_msg.first);

			ZeroMemory(addr, MAX_CHAR);
			memcpy(addr, &server.players[s_msg.first].addr.sin_addr, sizeof(server.players[s_msg.first].addr.sin_addr));
			send_msg(SERVER, DISPLAY, CONNECT, addr, s_msg.first);
			
			break;
		}
		case CLOSE:

			if (s_msg.first == leader)
				set_new_leader();

			send_msg(SERVER, PLAYER, QUIT, "", s_msg.first);
			send_msg(SERVER, DISPLAY, QUIT, "", s_msg.first);
			server.players.erase(s_msg.first);
			server.displays.erase(s_msg.first);
			send_names();
			break;

		case QUIT:

			close_socket(self);
			break;

		case NAME:
		
			server.players[s_msg.first].name = s_msg.second;
			send_names();
			break;
		
		case LEADER:
		
			set_new_leader();
			break;
		
		case LEADER_R:

			is_leader = true;
			break;

		case GOOD:

			is_ok = true;
			break;

		case BAD:

			is_ok = false;
			break;

		case PRINT:

			utl::clear_screen();
			std::cout << msg;
			break;
			
		case CONNECT:

			if (self == PLAYER)
				init_outgoing_addr(PLAYER, DISPLAY, _T(""), msg);
			else if (self == DISPLAY)
				init_outgoing_addr(DISPLAY, PLAYER, _T(""), msg);
			break;

		case TRANSIT:

			transit();
			break;
		
		case CACHE:

			if (self == SERVER)
				cache += s_msg.second + nl; // no longer reroutes cache signals to player and display
			else
				cache += msg + nl;
			break;

		case ANSWER:
		{
			std::istringstream iss(s_msg.second);
			std::string answer;
			server.players[s_msg.first].answers.clear();
			while (std::getline(iss, answer))
				server.players[s_msg.first].answers.push_back(answer);
			break;
		}
		case VOTE:
		{
			if (self == DISPLAY)
			{
				std::istringstream iss(msg);
				int vote;
				iss >> vote; // if this fails, vote will be 0
				if (iss)
					itf::update_vote(vote);
			}
			else if (self == SERVER)
			{	
				std::istringstream iss(s_msg.second);
				int a_num, vote;
				iss >> a_num >> vote;
				server.players[s_msg.first].votes[a_num - 1] = vote;

				// sum votes for answer a_num and send to display
				int sum = 0;
				for (std::pair<ULONG const, Conn> const& player : server.players)
					sum += (player.second.votes[a_num - 1] <= EMPTY ? 0 : player.second.votes[a_num - 1]);
				send_msg(SERVER, DISPLAY, VOTE_R, std::to_string(a_num) + nl + std::to_string(sum) + nl + std::to_string(std::count_if(server.players.begin(), server.players.end(), if_not_empty(a_num))));
			}
			break;
		}
		case VOTE_R:
		{
			std::istringstream iss(msg);
			int a_num, vote, curr;
			iss >> a_num >> vote >> curr;
			itf::update_vote(vote, false, a_num, curr);
			break;
		}
		case READY:

			server.players[s_msg.first].is_ready = true;
			for (std::pair<ULONG const, Conn> const& player : server.players)
				if (player.second.is_ready)
					send_msg(SERVER, PLAYER, PRINT, "Waiting for:\n" + get_names(if_not_ready), player.first);

			if (!counter)
			{
				transit();
				send_msg(SERVER, PLAYER, TRANSIT);
				send_msg(SERVER, DISPLAY, TRANSIT);

				// reset readiness
				for (std::pair<ULONG const, Conn>& player : server.players)
					player.second.is_ready = false;
			}

			break;

		case SEND:

			send_msg(PLAYER, SERVER, ANSWER, to_send);
			break;

		case NEXT:

			if (s_msg.first == leader) // only leader can go to next question
				go_next();
			break;

		case HIGHLIGHT:
		{
			std::istringstream iss(msg);
			int a_num;
			iss >> a_num;

			if (iss)
				itf::highlight_a(a_num);
		}
		}

		return true;
	}

	void close_socket(Window self)
	{
		if (self == SERVER)
		{
			send_msg(SERVER, PLAYER, QUIT);
			send_msg(SERVER, DISPLAY, QUIT);
		}

		// technically the cpu will automatically free up resources eventually so these two steps are really just optional
		closesocket(self == SERVER ? server.server.socket : self == PLAYER ? player.player.socket : display.display.socket);
		WSACleanup();
		exit(0);
	}

	/*! ------ accessors and mutators ------ */

	std::string get_host_addr()
	{
		return reinterpret_cast<char const*>(&player.server.addr.sin_addr);
	}

	bool check_leader()
	{
		return is_leader;
	}

	void renounce_leader()
	{
		is_leader = false;
	}

	bool check_ok()
	{
		bool ret = is_ok;
		is_ok = false;
		return ret;
	}

	Phase check_phase()
	{
		return phase;
	}

	std::string retrieve()
	{
		std::string ret = cache;
		cache = "";
		return ret;
	}
	
	void transit()
	{
		phase = static_cast<Phase>((phase + 1) % MAX_PHASE);
	}

	void cache_string(std::string const& to_cache)
	{
		to_send = to_cache;
	}

	/*! ------ functions to interface with player and display ------ */

	void fill_answers()
	{
		for (std::pair<ULONG const, Conn> const& player : server.players)
		{
			send_msg(SERVER, PLAYER, SEND, "", player.first);
			recv_msg(SERVER);
			read_msg(SERVER);
		}
	}

	void go_next(bool is_first)
	{
		static int curr_q = 0; // current question number - 1 (ie 0 = question 1, size() - 1 = last question)
		static std::vector<std::string> rand_q;

		if (is_first) // load questions (not answers)
		{
			curr_q = 0;
			rand_q.clear();

			std::istringstream iss(to_send);
			std::string q;
			int rand_letter; // not used
			iss >> rand_letter; 
			iss.get(); // remove leading \n

			while (getline(iss, q))
				rand_q.push_back(q);

			// tell display whether or not to show names
			send_msg(SERVER, DISPLAY, itf::check_visibility() ? GOOD : BAD);

			// reset current score
			for (std::pair<ULONG const, Conn>& player : server.players)
				player.second.curr_score = 0;
		}
		else // reward players with positive votes (> 0) and increment current question number
		{
			for (int i = 0; i < server.curr_id.size(); ++i) // for each answer
			{
				int sum = 0;
				for (std::pair<ULONG const, Conn> const& player : server.players) // for each player's vote
					sum += (player.second.votes[i] <= EMPTY ? 0 : player.second.votes[i]);
				if (sum > 0)
				{
					++server.players[server.curr_id[i]].score;
					++server.players[server.curr_id[i]].curr_score;
				}
			}
			++curr_q;

			if (curr_q >= rand_q.size())
			{
				transit();
				send_msg(SERVER, PLAYER, TRANSIT);
				send_msg(SERVER, DISPLAY, TRANSIT);
				send_msg(SERVER, DISPLAY, GOOD); // escape infinite while loop
				send_names();
				return;
			}

			send_msg(SERVER, DISPLAY, GOOD); // escape infinite while loop
		}

		// only server needs to know the questions
		send_msg(SERVER, DISPLAY, PRINT, T_CYAN + rand_q[curr_q] + " (" + std::to_string(curr_q + 1) + "/" + std::to_string(rand_q.size()) + ")\n\n" + RESET);
		
		// load answers for current question
		server.curr_id.clear();
		int max = static_cast<int>(server.players.size()) - 1; // maximum number of votes per question 

		// send message tailored to each display (to differentiate between own and other answers)
		for (std::pair<ULONG const, Conn> const& display : server.displays)
		{
			std::string q_data = "";
			for (std::pair<ULONG const, Conn> const& player : server.players)
			{
				if (player.second.answers[curr_q] != "/")
				{
					server.curr_id.push_back(player.first);
					q_data += player.second.name + nl + player.second.answers[curr_q] + nl + (display.first == player.first ? std::to_string(EMPTY) : std::to_string(OWN)) + nl + std::to_string(max); // no nl after the number
				}
			}
			send_msg(SERVER, DISPLAY, CACHE, q_data, display.first);
		}

		// set server.players.votes to appropriate size
		for (std::pair<ULONG const, Conn>& player : server.players)
		{
			player.second.votes.resize(server.curr_id.size());
			for (int& vote : player.second.votes)
				vote = EMPTY;
		}
	}
}