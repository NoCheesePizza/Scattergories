#pragma once

/*! ------ sorry I lazy type inverted commas and backslash ------ */

#define nl "\n"

/*! ------ limits ------ */

#define MAX_Q 300 // not hard limit, just how much space to reserve for vector for efficiency
#define MAX_CHAR 1024 // largest message size
#define MIN_RAND_Q 3
#define MAX_RAND_Q 18
#define MIN_TIME 30
#define MAX_TIME 180

/*! ------ special numbers ------ */

#define Q_WIDTH 50 // question/answer width
#define V_WIDTH 15 // vote width
#define N_WIDTH 25 // name width
#define UP -1
#define DOWN -2
#define EMPTY -3 // empty vote
#define OWN -4 // own answer
#define HEADER_NL 2 // number of newlines after header line (inclusive) in answering/voting phase
#define TO_SHORT 100 // short timeout (ms)
#define TO_LONG 240 * 1000 // long timeout (ms)

/*! ------ text colours ------ */

#define RESET "\033[0m"
#define T_BLACK "\033[90m"
#define T_RED "\033[91m"
#define T_GREEN "\033[92m"
#define T_YELLOW "\033[93m"
#define T_BLUE "\033[94m"
#define T_MAGENTA "\033[95m"
#define T_CYAN "\033[96m"
#define T_WHITE "\033[97m"

/*! ------ background colours ------ */

#define B_BLACK "\033[40m"
#define B_RED "\033[41m"
#define B_GREEN "\033[42m"
#define B_YELLOW "\033[43m"
#define B_BLUE "\033[44m"
#define B_MAGENTA "\033[45m"
#define B_CYAN "\033[46m"
#define B_WHITE "\033[47m"

/*! ------ print debug messages if DEBUGF is defined ------ */

//#define DEBUGF

#ifdef DEBUGF
#define debugf(x) x
#else
#define debugf(x) if (0) x
#endif