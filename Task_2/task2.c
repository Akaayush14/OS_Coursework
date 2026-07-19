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

// ==================== FUNCTION PROTOTYPES ====================

// Initialization functions
void init_simulation(Simulation *sim, int page_size, int num_frames, int num_pages);
void init_page_table(Simulation *sim);
void init_frames(Simulation *sim);
void init_statistics(Simulation *sim);

// Page replacement algorithms
int fifo_page_replacement(Simulation *sim, int page_number);
int lru_page_replacement(Simulation *sim, int page_number);
int find_free_frame(Simulation *sim);

// Memory operations
bool access_memory(Simulation *sim, int page_number, int algorithm);
void load_page(Simulation *sim, int page_number, int frame_index, int algorithm);
void evict_page(Simulation *sim, int frame_index);

// Reference string generation
void generate_reference_string(Simulation *sim);
void generate_reference_string_locality(Simulation *sim);
void generate_reference_string_random(Simulation *sim);
void generate_reference_string_sequential(Simulation *sim);

// Algorithm execution
void run_simulation(Simulation *sim, int algorithm);
void run_comparison(Simulation *sim);

// Statistics and display
void print_page_table(Simulation *sim);
void print_frame_state(Simulation *sim);
void print_statistics(Simulation *sim, const char *algorithm_name);
void print_comparison_report(Simulation *sim, const char *algo1, const char *algo2);

// Utility functions
void clear_screen(void);
void wait_for_user(void);
void print_header(const char *title);
void print_separator(void);
void show_menu(Simulation *sim);
void run_test_cases(Simulation *sim);

// ==================== IMPLEMENTATION ====================

/**
 * Initialize the simulation with given parameters
 */
void init_simulation(Simulation *sim, int page_size, int num_frames, int num_pages) {
    if (num_pages > MAX_PAGES) {
        printf("Warning: num_pages (%d) exceeds MAX_PAGES (%d). Truncating.\n",
               num_pages, MAX_PAGES);
        num_pages = MAX_PAGES;
    }
    if (num_frames > MAX_FRAMES) {
        printf("Warning: num_frames (%d) exceeds MAX_FRAMES (%d). Truncating.\n",
               num_frames, MAX_FRAMES);
        num_frames = MAX_FRAMES;
    }

    sim->page_size = page_size;
    sim->num_frames = num_frames;
    sim->num_pages = num_pages;
    sim->current_time = 0;
    sim->reference_length = 0;

    // Initialize queue for FIFO
    sim->fifo_front = 0;
    sim->fifo_rear = -1;
    sim->fifo_count = 0;
    init_page_table(sim);
    init_frames(sim);
    init_statistics(sim);

    // Generate default reference string
    generate_reference_string(sim);
}

/**
 * Initialize the page table with all pages not present
 */
void init_page_table(Simulation *sim) {
    for (int i = 0; i < sim->num_pages; i++) {
        sim->page_table[i].page_number = i;
        sim->page_table[i].frame_number = -1;
        sim->page_table[i].present = false;
        sim->page_table[i].last_used = -1;
        sim->page_table[i].load_time = -1;
    }
}

/**
 * Initialize all frames as empty
 */
void init_frames(Simulation *sim) {
    for (int i = 0; i < sim->num_frames; i++) {
        sim->frames[i].page_number = -1;
        sim->frames[i].occupied = false;
        sim->frames[i].load_time = -1;
        sim->frames[i].last_used = -1;
    }
}

/**
 * Initialize statistics to zero
 */
void init_statistics(Simulation *sim) {
    sim->stats.total_references = 0;
    sim->stats.page_hits = 0;
    sim->stats.page_faults = 0;
    sim->stats.hit_ratio = 0.0;
    sim->stats.miss_ratio = 0.0;
    sim->stats.page_replacements = 0;
}

/**
 * Generate a reference string with mixed locality and patterns
 */
void generate_reference_string(Simulation *sim) {
    int length = 50 + rand() % 50; // 50-100 references
    sim->reference_length = length;

    // Use a mix of patterns
    int pattern = rand() % 4;
    switch(pattern) {
        case 0:
            generate_reference_string_sequential(sim);
            break;
        case 1:
            generate_reference_string_random(sim);
            break;
        case 2:
            generate_reference_string_locality(sim);
            break;
        default:
            // Mixed pattern - combine locality and random
            for (int i = 0; i < length; i++) {
                if (i < length / 3) {
                    // Sequential access
                    sim->reference_string[i] = i % sim->num_pages;
                } else if (i < 2 * length / 3) {
                    // Locality access
                    int base = (i * 3) % sim->num_pages;
                    sim->reference_string[i] = base + (rand() % 3);
                    if (sim->reference_string[i] >= sim->num_pages) {
                        sim->reference_string[i] = sim->num_pages - 1;
                    }
                } else {
                    // Random access
                    sim->reference_string[i] = rand() % sim->num_pages;
                }
            }
            break;
    }
}

/**
 * Generate sequential reference string (0,1,2,3,4,...)
 */
void generate_reference_string_sequential(Simulation *sim) {
    for (int i = 0; i < sim->reference_length; i++) {
        sim->reference_string[i] = i % sim->num_pages;
    }
}

/**
 * Generate random reference string
 */
void generate_reference_string_random(Simulation *sim) {
    for (int i = 0; i < sim->reference_length; i++) {
        sim->reference_string[i] = rand() % sim->num_pages;
    }
}

/**
 * Generate reference string with locality (repeating patterns)
 */
void generate_reference_string_locality(Simulation *sim) {
    int locality_size = sim->num_pages / 4 + 1;
    if (locality_size < 2) locality_size = 2;

    for (int i = 0; i < sim->reference_length; i++) {
        // Choose a locality set
        int set = rand() % 4;
        int base = (set * locality_size) % sim->num_pages;
        int offset = rand() % locality_size;
        int page = (base + offset) % sim->num_pages;
        sim->reference_string[i] = page;
    }
}

/**
 * Find a free frame in memory
 * Returns the index of a free frame, or -1 if none are free
 */
int find_free_frame(Simulation *sim) {
    for (int i = 0; i < sim->num_frames; i++) {
        if (!sim->frames[i].occupied) {
            return i;
        }
    }
    return -1;
}

/**
 * FIFO page replacement algorithm
 * Returns the frame index where the page should be loaded
 */
int fifo_page_replacement(Simulation *sim, int page_number) {
    // Check if there's a free frame
    int free_frame = find_free_frame(sim);
    if (free_frame != -1) {
        return free_frame;
    }

    // No free frame - use FIFO to evict
    // Get the frame at the front of the FIFO queue
    int frame_to_evict = sim->fifo_queue[sim->fifo_front];
    sim->fifo_front = (sim->fifo_front + 1) % MAX_FRAMES;
    sim->fifo_count--;

    return frame_to_evict;
}

/**
 * LRU page replacement algorithm
 * Returns the frame index where the page should be loaded
 */
int lru_page_replacement(Simulation *sim, int page_number) {
    // Check if there's a free frame
    int free_frame = find_free_frame(sim);
    if (free_frame != -1) {
        return free_frame;
    }

    // No free frame - use LRU to evict
    // Find the frame with the oldest last_used timestamp
    int oldest_time = INT_MAX;
    int frame_to_evict = 0;

    for (int i = 0; i < sim->num_frames; i++) {
        if (sim->frames[i].occupied && sim->frames[i].last_used < oldest_time) {
            oldest_time = sim->frames[i].last_used;
            frame_to_evict = i;
        }
    }

    return frame_to_evict;
}

/**
 * Load a page into a specific frame
 */
void load_page(Simulation *sim, int page_number, int frame_index, int algorithm) {
    // If the frame is occupied, evict the current page
    if (sim->frames[frame_index].occupied) {
        evict_page(sim, frame_index);
        sim->stats.page_replacements++;
    }

    // Load the new page
    sim->frames[frame_index].occupied = true;
    sim->frames[frame_index].page_number = page_number;
    sim->frames[frame_index].load_time = sim->current_time;
    sim->frames[frame_index].last_used = sim->current_time;

    // Update page table
    sim->page_table[page_number].present = true;
    sim->page_table[page_number].frame_number = frame_index;
    sim->page_table[page_number].load_time = sim->current_time;
    sim->page_table[page_number].last_used = sim->current_time;

    // For FIFO, add the frame to the queue
    if (algorithm == 0) { // FIFO
        sim->fifo_rear = (sim->fifo_rear + 1) % MAX_FRAMES;
        sim->fifo_queue[sim->fifo_rear] = frame_index;
        sim->fifo_count++;
    }
}

/**
 * Evict a page from a frame
 */
void evict_page(Simulation *sim, int frame_index) {
    if (sim->frames[frame_index].occupied) {
        int page_number = sim->frames[frame_index].page_number;
        sim->page_table[page_number].present = false;
        sim->page_table[page_number].frame_number = -1;
        sim->frames[frame_index].occupied = false;
        sim->frames[frame_index].page_number = -1;
    }
}

/**
 * Access memory - returns true if hit, false if fault
 */
bool access_memory(Simulation *sim, int page_number, int algorithm) {
    sim->current_time++;
    sim->stats.total_references++;
    // Check if page is in memory
    if (page_number >= 0 && page_number < sim->num_pages &&
        sim->page_table[page_number].present) {
        // Page hit - update access time
        int frame_index = sim->page_table[page_number].frame_number;
        sim->frames[frame_index].last_used = sim->current_time;
        sim->page_table[page_number].last_used = sim->current_time;
        sim->stats.page_hits++;
        return true; // Hit
    } else {
        // Page fault
        sim->stats.page_faults++;

        // Determine which frame to use
        int frame_index;
        if (algorithm == 0) {
            frame_index = fifo_page_replacement(sim, page_number);
        } else {
            frame_index = lru_page_replacement(sim, page_number);
        }

        // Load the page into the frame
        load_page(sim, page_number, frame_index, algorithm);
        return false; // Fault
    }
}
