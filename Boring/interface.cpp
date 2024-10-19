#include "interface.h"

using namespace std::literals::chrono_literals;

namespace itf
{
	struct Q_Data
	{
		std::string name, 
					answer;

		int own_vote,
			total_vote = 0,
			curr = 0, // current number of votes
			max; // maximum number of votes

		//todo the order of member list initialisation matters
		Q_Data(std::string _name, std::string _answer, int _own_vote, int _max)
			: name{ _name }, answer{ _answer }, own_vote{ _own_vote }
		{
			max = _max;
		}
	};

	int	rand_q_count = 12,
		duration = 30,
		is_shown = 1, // 1 == show, 2 == hide
		prev_q = 0, // previously highlighted question
		prev_a = 0; // previously highlighted answer

	char rand_letter;
	std::vector<std::string> q, // question list
							 rand_q; // random questions

	std::vector<std::pair<std::string, bool>> answers; // player answers and whether they are valid

	std::vector<Q_Data> q_data; // all player answers for one question

	/*! ------ auxiliary functions ------ */

	bool invalid_answer(std::pair<std::string, bool> const& answer)
	{
		return !answer.second;
	}

	bool invalid_vote(Q_Data const& answer)
	{
		return answer.own_vote != EMPTY; // was == EMPTY
	}

	struct identical_answer
	{
		std::string to_match;

		identical_answer(std::string const& _to_match) 
			: to_match{ _to_match }
		{

		}

		bool operator()(std::pair<std::string, bool> const& answer) const
		{
			return answer.first == to_match;
		}
	};

	/*! ------ server functions ------ */

	void load_q()
	{
		std::ifstream q_file("Questions_1.txt");
		int j;
		std::string temp;

		q.reserve(MAX_Q);
		for (j = 0; std::getline(q_file, temp); ++j)
			q.push_back(temp);

		srand(static_cast<unsigned>(time(0)));
	}

	void get_game_data()
	{
		std::istringstream iss(nwk::retrieve());
		iss >> rand_q_count >> duration >> is_shown;
	}

	void set_rand_q()
	{
		std::vector<int> rand_ints;
		rand_q.clear();

		for (int i = 0; i < rand_q_count; ++i)
		{
			bool is_dup;
			int ri;

			do
			{
				is_dup = false;
				ri = utl::rand_int(0, static_cast<int>(q.size()) - 1);

				for (int i : rand_ints)
					if (i == ri)
					{
						is_dup = true;
						break;
					}
			} while (is_dup);

			rand_ints.push_back(ri);
			rand_q.push_back(q[ri]);
		}

		static std::string const banned_letters = "jkqxyz";
		do
			rand_letter = static_cast<char>(utl::rand_int(static_cast<int>('A'), static_cast<int>('Z')));
		while (banned_letters.find(rand_letter) != std::string::npos);
	}

	void send_rand_q()
	{
		std::string to_send = std::to_string(rand_letter) + nl;
		for (std::string const& question : rand_q)
			to_send += question + nl;

		nwk::cache_string(to_send); // for server-side code to get rand_q during VOTING phase
		nwk::send_msg(nwk::SERVER, nwk::PLAYER, nwk::CACHE, to_send);
		std::this_thread::sleep_for(3s);
	}

	void send_timer()
	{
		for (int i = duration; i > 0; --i)
		{
			nwk::send_msg(nwk::SERVER, nwk::DISPLAY, nwk::PRINT, "Time left: " + std::to_string(i));
			std::this_thread::sleep_for(1s);
		}
	}

	bool check_visibility()
	{
		return is_shown == 1;
	}

	/*! ------ player functions ------ */

	void write_q(int q_num, std::string const& colour = RESET)
	{
		utl::set_cursor(q_num + HEADER_NL + 1, 5);
		std::cout << colour << std::left << std::setw(Q_WIDTH) << rand_q[q_num - 1] << (answers[q_num - 1].second ? T_GREEN : T_RED) << std::setw(Q_WIDTH) << answers[q_num - 1].first << RESET;
		utl::set_cursor(static_cast<int>(rand_q.size()) + HEADER_NL * 2 + 1, 0);
	}

	void print_menu()
	{
		while (true)
		{
			utl::clear_screen();
			std::cout << "1: Open display console\n2: Rename\n3: Start game\n4: Leave game\n";
			int choice = 0;
			if (nwk::check_leader())
			{
				std::cout << "5: View setup\n6: Edit setup\n7: Renounce leadership\n8: Close server\n";
				choice = utl::get_int(1, 8);
			}
			else
				choice = utl::get_int(1, 4);
			utl::clear_screen();

			if (choice == 1)
			{
				system(std::string("start Boring.exe 1 " + nwk::get_host_addr()).c_str());
				std::this_thread::sleep_for(1s);
			}
			else if (choice == 2)
			{
				std::cout << "Enter your new name:\n";
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::NAME, utl::get_string(N_WIDTH));
				utl::clear_screen();
			}
			else if (choice == 3)
			{
				if (nwk::check_leader())
					nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::CACHE, std::to_string(rand_q_count) + nl + std::to_string(duration) + nl + std::to_string(is_shown)); // send game info
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::READY);
				break;
			}
			else if (choice == 4)
			{
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::CLOSE);
			}
			else if (choice == 5)
			{
				std::cout << "1: Go back\n" << std::left << std::setw(N_WIDTH) << "Number of questions" << "- " << rand_q_count << nl << std::setw(N_WIDTH) << "Duration" << "- " << duration << nl << std::setw(N_WIDTH) << "Names are shown" << (is_shown == 1 ? "- Yes\n" : "- No\n");
				utl::get_int(1, 1);
				utl::clear_screen();
			}
			else if (choice == 6)
			{
				std::cout << "1: Set number of questions\n2: Set duration\n3: Set visibility\n4: Go back\n";
				int choice = utl::get_int(1, 4);
				utl::clear_screen();

				if (choice == 1)
				{
					std::cout << "Enter an integer between " << MIN_RAND_Q << " and " << MAX_RAND_Q << nl;
					rand_q_count = utl::get_int(MIN_RAND_Q, MAX_RAND_Q);
				}
				else if (choice == 2)
				{
					std::cout << "Enter an integer between " << MIN_TIME << " and " << MAX_TIME << nl;
					duration = utl::get_int(MIN_TIME, MAX_TIME);
				}
				else if (choice == 3)
				{
					std::cout << "1: Show names\n2: Hide names\n";
					is_shown = utl::get_int(1, 2);
				}

				utl::clear_screen();
			}
			else if (choice == 7)
			{
				nwk::renounce_leader();
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::LEADER);
				std::this_thread::sleep_for(1s);
			}
			else if (choice == 8)
			{
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::QUIT);
			}
		}

		utl::clear_screen();
	}

	void get_rand_q()
	{
		std::string retrieved = nwk::retrieve();
		std::istringstream iss(retrieved.substr(0, retrieved.size() - 2)); // remove trailing \n

		int temp;
		iss >> temp;
		rand_letter = static_cast<char>(temp);

		rand_q.clear();
		answers.clear();

		std::string question;
		iss.get(); // remove leading \n
		while (std::getline(iss, question))
			rand_q.push_back(question);
		answers.resize(rand_q.size());

		std::cout << T_CYAN << "Fill in words or phrases that start with the letter '" << rand_letter << "'\n\n" << T_YELLOW << "    " << std::left << std::setw(Q_WIDTH) << "Question" << "Your answer" << RESET << nl;
		for (size_t i = 0; i < rand_q.size(); ++i)
			std::cout << std::left << std::setw(2) << i + 1 << ": " << rand_q[i] << nl;

		prev_q = 0;
		itf::highlight_q(1); // highlight first line
	}

	void highlight_q(int q_num)
	{
		// automatically set highlighted text to the next empty answer
		if (q_num == EMPTY)
		{
			q_num = static_cast<int>(std::find_if(answers.begin() + prev_q, answers.end(), invalid_answer) - answers.begin()) + 1; // from 1 past itself to 1 past the last (inclusive)
			if (q_num == static_cast<int>(answers.size()) + 1) // from the first to 1 before itself (inclusive)
				q_num = static_cast<int>(std::find_if(answers.begin(), answers.begin() + prev_q - 1, invalid_answer) - answers.begin()) + 1;
			//todo std::find returns an iterator to one past the last element of the range you specified and not of the actual container
			if (q_num == prev_q)
				q_num = 0;
		}

		if (prev_q)
			write_q(prev_q);

		if (!q_num)
		{
			prev_q = q_num;
			return;
		}

		if (q_num == UP)
			if (!prev_q)
				return;
			else
				q_num = (prev_q == 1 ? static_cast<int>(rand_q.size()) : prev_q - 1);
		else if (q_num == DOWN)
			if (!prev_q)
				return;
			else
				q_num = (prev_q == rand_q.size() ? 1 : prev_q + 1);
		
		write_q(q_num, B_BLUE);
		prev_q = q_num;
	}

	void get_answer()
	{
		std::string input = utl::get_string(Q_WIDTH);
		if (nwk::check_phase() != nwk::ANSWERING)
			return;

		if (input[0] == '/')
		{
			int q_num = 0;

			if (input[1] == 'w')
				q_num = UP;
			else if (input[1] == 's')
				q_num = DOWN;
			else
			{
				std::istringstream iss(input.substr(1));
				iss >> q_num;
				if (q_num > 0 && q_num <= static_cast<int>(rand_q.size()) && iss)
					highlight_q(q_num);
				return;
			}

			highlight_q(q_num);
		}
		else
		{
			if (!prev_q)
				return;

			// check if the first letter is correct and if the answer is unique 
			if (utl::to_lower(input[0]) == utl::to_lower(rand_letter) && std::find_if(answers.begin(), answers.end(), identical_answer(input)) == answers.end())
				answers[prev_q - 1].second = true;
			else
				answers[prev_q - 1].second = false;
			answers[prev_q - 1].first = input;

			insert_answer();
		}
	}

	void insert_answer()
	{
		write_q(prev_q);

		// continue highlighting same question if it contains the last invalid answer
		if (!answers[prev_q - 1].second && static_cast<int>(std::count_if(answers.begin(), answers.end(), invalid_answer)) == 1)
		{
			highlight_q(prev_q);
			return;
		}

		highlight_q(EMPTY);
	}

	void cache_answers()
	{
		std::string to_cache = "";
		for (std::pair<std::string, bool> const& answer : answers)
			to_cache += (answer.second ? answer.first : "/") + nl;
		nwk::cache_string(to_cache);
	}

	/*! ------ display functions ------ */

	void write_a(int a_num, std::string colour = RESET)
	{
		utl::set_cursor(a_num + HEADER_NL + 1, 5);
		std::cout << colour << std::left << std::setw(Q_WIDTH) << q_data[a_num - 1].answer;
		if (is_shown == 1)
			std::cout << std::left << std::setw(N_WIDTH) << q_data[a_num - 1].name;
		std::cout << std::left << (q_data[a_num - 1].own_vote > 0 ? T_GREEN : "") << std::setw(V_WIDTH) << (q_data[a_num - 1].own_vote == EMPTY ? "" : q_data[a_num - 1].own_vote == OWN ? "-" : std::to_string(q_data[a_num - 1].own_vote)) << RESET << colour << (q_data[a_num - 1].total_vote > 0 ? T_GREEN : "") << std::setw(V_WIDTH) << q_data[a_num - 1].total_vote << RESET << (q_data[a_num - 1].curr == q_data[a_num - 1].max ? "" : T_RED) << " [" << q_data[a_num - 1].curr << "/" << q_data[a_num - 1].max << "]\n" << RESET;
		utl::set_cursor(static_cast<int>(q_data.size()) + HEADER_NL * 2 + 1, 0);
	}

	void get_visibility()
	{
		// recv msg with GOOD (show names) or BAD (hide names) signal
		recv_msg(nwk::DISPLAY);
		read_msg(nwk::DISPLAY);
		is_shown = nwk::check_ok() ? 1 : 2;
	}

	// load data for one question
	void get_q_data()
	{
		// recv msg with PRINT signal
		recv_msg(nwk::DISPLAY);
		read_msg(nwk::DISPLAY);

		// recv msg with CACHE signal
		recv_msg(nwk::DISPLAY);
		read_msg(nwk::DISPLAY);

		std::string retrieved = nwk::retrieve();
		std::istringstream iss(retrieved.substr(0, retrieved.size() - 1));
		std::string name, answer;
		int own_vote, max;

		int i = 0;
		q_data.clear();

		if (retrieved != nl) // retrieved is not empty 
		{
			std::cout << "    " << T_YELLOW << std::left << std::setw(Q_WIDTH) << "Responses";
			if (is_shown == 1)
				std::cout << std::left << std::setw(N_WIDTH) << "Written by";
			std::cout << std::left << std::setw(V_WIDTH) << "Your vote" << "Total vote\n" << RESET;

			while (std::getline(iss, name))
			{
				std::getline(iss, answer);
				iss >> own_vote >> max;
				q_data.push_back({ name, answer, own_vote, max });
				std::cout << std::left << std::setw(2) << i + 1 << ": " << std::setw(Q_WIDTH) << q_data[i].answer;
				if (is_shown == 1)
					std::cout << std::left << std::setw(N_WIDTH) << q_data[i].name;
				std::cout << std::left << std::setw(V_WIDTH) << (q_data[i].own_vote == EMPTY ? "" : "-") << std::setw(V_WIDTH) << 0 << T_RED << " [0/" << q_data[i++].max << "]\n" << RESET;
			}

			prev_a = 1;
			highlight_a(EMPTY);
		}
	}

	void highlight_a(int a_num)
	{
		// find next non-empty, non-own answer to highlight
		if (a_num == EMPTY)
		{
			a_num = static_cast<int>(std::find_if(q_data.begin() + prev_a, q_data.end(), invalid_vote) - q_data.begin()) + 1;
			if (a_num == static_cast<int>(q_data.size()) + 1) 
				a_num = static_cast<int>(std::find_if(q_data.begin(), q_data.end(), invalid_vote) - q_data.begin()) + 1;
			if (a_num == static_cast<int>(q_data.size()) + 1)
				a_num = 0;
		}

		if (a_num < DOWN || a_num > q_data.size())
			return;

		if (prev_a)
			write_a(prev_a);

		if (!a_num)
		{
			prev_a = a_num;
			return;
		}

		if (a_num == UP)
			if (!prev_a)
				return;
			else
				a_num = (prev_a == 1 ? static_cast<int>(q_data.size()) : prev_a - 1);
		else if (a_num == DOWN)
			if (!prev_a)
				return;
			else
				a_num = (prev_a == q_data.size() ? 1 : prev_a + 1);

		write_a(a_num, B_BLUE);
		prev_a = a_num;
	}

	void get_vote()
	{
		std::string input = utl::get_string(V_WIDTH);
		if (nwk::check_phase() != nwk::VOTING)
			return;

		if (input[0] == '/') // eg /s
		{
			if (input[1] == 'w')
				nwk::send_msg(nwk::PLAYER, nwk::DISPLAY, nwk::HIGHLIGHT, std::to_string(UP));
			else if (input[1] == 's')
				nwk::send_msg(nwk::PLAYER, nwk::DISPLAY, nwk::HIGHLIGHT, std::to_string(DOWN));
			else if (input[1] == 'n')
				nwk::send_msg(nwk::PLAYER, nwk::SERVER, nwk::NEXT);
			else // eg /5
				nwk::send_msg(nwk::PLAYER, nwk::DISPLAY, nwk::HIGHLIGHT, input.substr(1));
		}
		else // eg -2
			nwk::send_msg(nwk::PLAYER, nwk::DISPLAY, nwk::VOTE, input);
	}

	// must clear previous vote too in case fewer digits
	void update_vote(int vote, bool is_own_vote, int a_num, int curr)
	{
		// if player is voting client-side, the vote is between -2 and 2 inclusive, and it's not the player's own answer
		if (is_own_vote && vote > -3 && vote < 3 && prev_a /*&& q_data[prev_a - 1].own_vote != OWN*/)
		{
			q_data[prev_a - 1].own_vote = vote;
			//write_a(prev_a);
			nwk::send_msg(nwk::DISPLAY, nwk::SERVER, nwk::VOTE, std::to_string(prev_a) + nl + std::to_string(vote));
			highlight_a(EMPTY);
		}
		else if (!is_own_vote)
		{
			q_data[a_num - 1].total_vote = vote;
			q_data[a_num - 1].curr = curr;
			write_a(a_num, a_num == prev_a ? B_BLUE : RESET);
		}
	}
}