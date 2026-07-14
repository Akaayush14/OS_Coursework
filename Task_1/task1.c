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

/* ─────────────────────────────────────────────────
   DRAW HELPERS
───────────────────────────────────────────────── */

void print_bar(int done, int total, const char *color) {
    int filled = (done * BAR_WIDTH) / total;
    printf("%s[", color);
    for (int i = 0; i < BAR_WIDTH; i++)
        printf(i < filled ? "\xe2\x96\x88" : "\xe2\x96\x91");
    printf("]" RESET);
}

void print_mutex_row(const char *label, int locked, int holder) {
    printf("    %-16s: ", label);
    if (locked)
        printf(RED "[LOCKED]" RESET "  held by %s%s\n" RESET,
               T_COLOR[holder], T_NAME[holder]);
    else
        printf(GREEN "[FREE]" RESET "\n");
}

/* ─────────────────────────────────────────────────
   DRAW UI
───────────────────────────────────────────────── */
void draw_ui(void) {
    printf(CLEAR);

    /* Header */
    printf(BOLD BLUE
        "\n"
        "  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n"
        "  \xe2\x95\x91  ST5004CEM \xe2\x80\x94 Task 1: Complete Thread Management   \xe2\x95\x91\n"
        "  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n"
        RESET "\n");

    /* Shared counter */
    printf(BOLD "  Shared Counter : " RESET GREEN BOLD "%d" RESET
           DIM "  (target: %d  =  %d threads x %d rounds)\n\n" RESET,
           shared_counter,
           NUM_THREADS * TOTAL_WORK, NUM_THREADS, TOTAL_WORK);

    /* Thread table header */
    printf(BOLD "  %-12s  %-8s  %-24s  %s\n" RESET,
           "Thread", "Rounds", "Progress", "State");
    printf(DIM "  ────────────  ────────  ────────────────────────  ─────────\n" RESET);

    /* One row per thread */
    for (int i = 0; i < NUM_THREADS; i++) {
        ThreadArgs *t = &targs[i];

        const char *sc =
            strcmp(t->state, "DONE")    == 0 ? GREEN        :
            strcmp(t->state, "RUNNING") == 0 ? T_COLOR[i]   :
            strcmp(t->state, "LOCKING") == 0 ? YELLOW       :
            strcmp(t->state, "SEM")     == 0 ? MAGENTA      :
            strcmp(t->state, "RES-A")   == 0 ? YELLOW       :
            strcmp(t->state, "RES-B")   == 0 ? YELLOW       : DIM;

        printf("  %s%-12s" RESET "  %s%d / %d%s  ",
               T_COLOR[i], T_NAME[i],
               BOLD, t->rounds_done, TOTAL_WORK, RESET);

        print_bar(t->rounds_done, TOTAL_WORK, T_COLOR[i]);
        printf("  %s%-9s" RESET "\n", sc, t->state);
    }

    /* Scheduler current turn */
    printf("\n" BOLD "  Scheduler turn  : " RESET);
    printf("%s%s\n" RESET, T_COLOR[current_turn], T_NAME[current_turn]);

    /* Semaphore status */
    printf("\n" BOLD "  Semaphore count  : " RESET);
    if (sem_available > 0)
        printf(GREEN "%d" RESET " (available)\n", sem_available);
    else
        printf(RED "%d" RESET " (fully used)\n", sem_available);

    /* Mutex status panel */
    printf("\n" BOLD "  Mutex locks:\n" RESET);
    print_mutex_row("counter_mutex",   counter_mutex_locked, counter_mutex_holder);
    print_mutex_row("scheduler_mutex", sched_mutex_locked,   sched_mutex_holder);
    print_mutex_row("resource_A",      resA_locked,          resA_holder);
    print_mutex_row("resource_B",      resB_locked,          resB_holder);

    /* Features Legend */
    printf("\n" BOLD "  Features:\n" RESET);
    printf(DIM "    ├─ " GREEN "✓" RESET DIM " Round-robin scheduling\n");
    printf(DIM "    ├─ " GREEN "✓" RESET DIM " Mutex synchronization\n");
    printf(DIM "    ├─ " GREEN "✓" RESET DIM " Semaphore (max 2 concurrent access)\n");
    printf(DIM "    ├─ " GREEN "✓" RESET DIM " Deadlock prevention (ordered locking)\n");
    printf(DIM "    └─ " GREEN "✓" RESET DIM " Race condition handling\n" RESET);
}
