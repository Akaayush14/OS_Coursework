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
