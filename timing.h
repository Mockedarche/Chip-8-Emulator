#ifndef timing_h
#define timing_h
#include <sys/time.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * current_ms function
 * Expects: NA
 * Does: Converts the current time to its millisecond representation as a long
 *
 */
unsigned long current_ms();

/*
 * pretty_timer function
 * Expects: NA
 * Does: Prints how long in ms it's been since this method was LAST called (using a static variable)
 *
 */
void pretty_timer(bool reset);

/*
 * sleep_for_instruction function
 * Expects: time_per_instruction_ms to be a valid float
 * Does: Accounts for the entire time since it was last called (minus time it slept) and sleeps for a delta
 * time that accounts for variance
 */
void sleep_for_instruction(float time_per_instruction_ms);

/*
 * make_future_time function
 * Expects: ms to be a valid double
 * Does: Returns a struct timespec that has its time offset by the provided ms (milliseconds)
 */
struct timespec make_future_time(double ms);

/*
 * time_has_passed function
 * Expects: *time should be correctly initialized
 * Does: Returns true if targets time has passed else returns false
 *
 */
bool time_has_passed(struct timespec *target);

/*
 * track_instruction function
 * Expects: NA
 * Does: Counts how many times its called per second and prints that out every second
 * as instructions per second
 *
 */
int track_instructions();

/*
 * millis_since helper function - get_most_recent_input
 * Expects: the start timespec struct to be correctly initialized with a time
 * Does: returns how many milliseconds (int) have passed since the given timespec struct
 *
 */
int millis_since(struct timespec start);

#endif /* timing_h */
