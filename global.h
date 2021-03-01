/* easy*/
#define SO_MAX_TIME 3 
#define SO_NUM_G 2 
#define SO_NUM_P 10 
#define SO_MIN_HOLD_NSEC 100000000 
#define SO_N_MOVES 20 
#define SO_ROUND_SCORE 10 
#define SO_FLAG_MIN 5 
#define SO_FLAG_MAX 5 
#define SO_BASE 60 
#define SO_ALTEZZA 20 

/* hard
#define SO_MAX_TIME 1 
#define SO_NUM_G 4 
#define SO_NUM_P 100 
#define SO_MIN_HOLD_NSEC 100000000 
#define SO_N_MOVES 200 
#define SO_ROUND_SCORE 200 
#define SO_FLAG_MIN 5 
#define SO_FLAG_MAX 40 
#define SO_BASE 120
#define SO_ALTEZZA 40 
*/
#define SIZE SO_BASE * SO_ALTEZZA

#define TEST_ERROR    if (errno) {fprintf(stderr,"%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__, __LINE__, getpid(), errno, strerror(errno));}

#define true 1
#define false 0