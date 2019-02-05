/*
 * Color library for C/C++
 * Use like this;
 * printf(RED("We'll print this in RED!  x = %d\n"), 5);
 * Some of the colors are just here for the sake of completeness (e.g. BBLACK)
 * In general, these have "normal", dark, and light versions.
 */

#define BLACK(X)	"\x1b[0;30m"X"\x1b[1;0m"	
#define RED(X)		"\x1b[0;31m"X"\x1b[1;0m"
#define GREEN(X)	"\x1b[0;32m"X"\x1b[1;0m"
#define YELLOW(X)	"\x1b[0;33m"X"\x1b[1;0m"
#define BLUE(X)	"\x1b[0;34m"X"\x1b[1;0m"
#define PURPLE(X)	"\x1b[0;35m"X"\x1b[1;0m"	
#define CYAN(X)	"\x1b[0;36m"X"\x1b[1;0m"
#define WHITE(X)	"\x1b[0;37m"X"\x1b[1;0m"

#define BBLACK(X)		"\x1b[1;30m"X"\x1b[1;0m"	
#define BRED(X)		"\x1b[1;31m"X"\x1b[1;0m"
#define BGREEN(X)		"\x1b[1;32m"X"\x1b[1;0m"	
#define BYELLOW(X)	"\x1b[1;33m"X"\x1b[1;0m"	
#define BBLUE(X)		"\x1b[1;34m"X"\x1b[1;0m"
#define BPURPLE(X)	"\x1b[1;35m"X"\x1b[1;0m"	
#define BCYAN(X)		"\x1b[1;36m"X"\x1b[1;0m"
#define BWHITE(X)		"\x1b[1;37m"X"\x1b[1;0m"	

#define DBLACK(X)		"\x1b[2;30m"X"\x1b[1;0m"	
#define DRED(X)		"\x1b[2;31m"X"\x1b[1;0m"
#define DGREEN(X)		"\x1b[2;32m"X"\x1b[1;0m"
#define DYELLOW(X)	"\x1b[2;33m"X"\x1b[1;0m"
#define DBLUE(X)		"\x1b[2;34m"X"\x1b[1;0m"
#define DPURPLE(X)	"\x1b[2;35m"X"\x1b[1;0m"
#define DCYAN(X)		"\x1b[2;36m"X"\x1b[1;0m"
#define DWHITE(X)		"\x1b[2;37m"X"\x1b[1;0m"
