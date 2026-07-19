#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

// ==================== CONFIGURATION ====================

#define MAX_PAGES 100
#define MAX_FRAMES 50
#define MAX_REFERENCE_STRING 1000

// ==================== DATA STRUCTURES ====================

/**
 * Page Table Entry - Represents a single page in the page table
 */
typedef struct {
    int page_number;        // Virtual page number
    int frame_number;       // Physical frame number (-1 if not in memory)
    bool present;           // Whether page is currently in memory
    int last_used;          // Timestamp for LRU algorithm
    int load_time;          // Time when page was loaded (for FIFO)
} PageTableEntry;

/**
 * Memory Frame - Represents a physical frame in memory
 */
typedef struct {
    int page_number;        // Page currently in this frame (-1 if empty)
    bool occupied;          // Whether frame is occupied
    int load_time;          // Time when frame was loaded (for FIFO)
    int last_used;          // Timestamp for LRU algorithm
} Frame;

/**
 * Simulation Statistics - Tracks performance metrics
 */
typedef struct {
    int total_references;
    int page_hits;
    int page_faults;
    float hit_ratio;
    float miss_ratio;
    int page_replacements;
} Statistics;

/**
 * Simulation Context - Holds all simulation state
 */
typedef struct {
    int page_size;              // Size of each page in bytes
    int num_frames;             // Number of physical frames
    int num_pages;              // Total number of virtual pages
    int current_time;           // Global time counter for LRU

    PageTableEntry page_table[MAX_PAGES];
    Frame frames[MAX_FRAMES];
    int reference_string[MAX_REFERENCE_STRING];
    int reference_length;

    Statistics stats;

    // FIFO queue tracking
    int fifo_queue[MAX_FRAMES];
    int fifo_front;
    int fifo_rear;
    int fifo_count;
} Simulation;
