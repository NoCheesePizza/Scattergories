#pragma once

#include <fstream>

#include "main.h"
#include "network.h"
#include "utility.h"

namespace itf
{
	/*! ------ server functions ------ */

	// load all questions from text files
	void load_q();

	// get type, question count, and duration from leader
	void get_game_data();

	// set random questions and letter
	void set_rand_q();

	// send whatever generated from the above function to all players
	void send_rand_q();

	void send_timer();

	// get is_shown variable
	bool check_visibility();

	/*! ------ player functions ------ */

	void print_menu();

	// load random questions using data sent from server
	void get_rand_q();

	void highlight_q(int q_num);

	void get_answer();

	void insert_answer();

	void cache_answers();

	/*! ------ display functions ------ */

	// check if names are to be shown for this whole round
	void get_visibility();

	// load all answers for one question
	void get_q_data();

	void highlight_a(int a_num);

	void get_vote();

	void update_vote(int vote, bool is_own_vote = true, int a_num = 0, int curr = 0);
}