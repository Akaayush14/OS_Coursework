#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* ─────────────────────────────────────────────────
   CONSTANTS
───────────────────────────────────────────────── */
#define NUM_THREADS 3
#define TIME_SLICE  1    /* seconds each thread works per round */
#define TOTAL_WORK  3    /* rounds each thread must complete    */
#define BAR_WIDTH   20   /* character width of progress bars   */

/* ─────────────────────────────────────────────────
   ANSI ESCAPE CODES for adding colours and effects
───────────────────────────────────────────────── */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

#define CLEAR   "\033[2J\033[H"

/* One colour per thread so they're visually distinct */
const char *T_COLOR[3] = {CYAN, MAGENTA, YELLOW};
const char *T_NAME[3]  = {"Thread-0", "Thread-1", "Thread-2"};

/* ─────────────────────────────────────────────────
   SHARED DATA + MUTEX
───────────────────────────────────────────────── */
int shared_counter = 0;
pthread_mutex_t counter_mutex;

/* ─────────────────────────────────────────────────
   SEMAPHORE - for resource control
   Allows only 2 threads to access resource simultaneously
───────────────────────────────────────────────── */
sem_t resource_semaphore;

/* ─────────────────────────────────────────────────
   ROUND-ROBIN SCHEDULER STATE
───────────────────────────────────────────────── */
int current_turn = 0;
pthread_mutex_t scheduler_mutex;
pthread_cond_t  turn_cond;

/* ─────────────────────────────────────────────────
   DEADLOCK-PREVENTION RESOURCES
───────────────────────────────────────────────── */
pthread_mutex_t resource_A;
pthread_mutex_t resource_B;

/* ─────────────────────────────────────────────────
   THREAD ARGUMENT + STATE STRUCT
───────────────────────────────────────────────── */
typedef struct {
    int thread_id;
    int rounds_done;
    char state[16]; /* "WAITING"|"RUNNING"|"LOCKING"|"SEM"|"RES-A"|"RES-B"|"DONE" */
} ThreadArgs;

ThreadArgs targs[NUM_THREADS];

/* ─────────────────────────────────────────────────
   MUTEX DISPLAY STATE
───────────────────────────────────────────────── */
int counter_mutex_locked = 0;
int counter_mutex_holder = -1;
int sched_mutex_locked   = 0;
int sched_mutex_holder   = -1;
int resA_locked = 0;
int resA_holder = -1;
int resB_locked = 0;
int resB_holder = -1;
int sem_available = 2;  /* semaphore count for display */

pthread_mutex_t ui_mutex;

/* ─────────────────────────────────────────────────
   RACE CONDITION DEMONSTRATION VARIABLES
───────────────────────────────────────────────── */
int unsafe_counter = 0;  /* Without synchronization - for race demo */
pthread_mutex_t race_mutex;  /* For safe version */
