#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdio.h>


/*
 * current_ms function
 * Expects: NA
 * Does: Converts the current time to its millisecond representation as a long
 *
 */
unsigned long current_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000UL) + (tv.tv_usec / 1000UL);
}

/*
 * pretty_timer function
 * Expects: NA
 * Does: Prints how long in ms it's been since this method was LAST called (using a static variable)
 *
 */
void pretty_timer(bool reset) {
    static struct timespec last_call = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (reset || (last_call.tv_sec == 0 && last_call.tv_nsec == 0)) {
        last_call = now;
        if (!reset) printf("First call, starting timer...\n");
        return;
    }

    double elapsed_ms = (now.tv_sec - last_call.tv_sec) * 1000.0 +
                        (now.tv_nsec - last_call.tv_nsec) / 1000000.0;
    printf("Time since last call: %.3f ms\n", elapsed_ms);

    last_call = now;

}

/*
 * sleep_for_instruction function
 * Expects: time_per_instruction_ms to be a valid float
 * Does: Accounts for the entire time since it was last called (minus time it slept) and sleeps for a delta
 * time that accounts for variance
 */
void sleep_for_instruction(float time_per_instruction_ms) {
    static struct timespec last_time = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_time.tv_sec == 0 && last_time.tv_nsec == 0) {
        last_time = now;  // first call
    }

    long elapsed_ns = (now.tv_sec - last_time.tv_sec) * 1000000000L
                    + (now.tv_nsec - last_time.tv_nsec);

    long target_ns = (long)(time_per_instruction_ms * 1000000L);
    long sleep_ns = target_ns - elapsed_ns;

    if (sleep_ns > 0) {
        struct timespec ts;
        ts.tv_sec = sleep_ns / 1000000000L;
        ts.tv_nsec = sleep_ns % 1000000000L;
        nanosleep(&ts, NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &last_time);

}

/*
 * make_future_time function
 * Expects: ms to be a valid double
 * Does: Returns a struct timespec that has its time offset by the provided ms (milliseconds)
 */
struct timespec make_future_time(double ms) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    long ns_to_add = (long)(ms * 1000000.0);
    t.tv_nsec += ns_to_add;
    if (t.tv_nsec >= 1000000000) {
        t.tv_sec += t.tv_nsec / 1000000000;
        t.tv_nsec %= 1000000000;
    }
    return t;

}

/*
 * time_has_passed function
 * Expects: *time should be correctly initialized
 * Does: Returns true if targets time has passed else returns false
 *
 */
bool time_has_passed(struct timespec *target) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (now.tv_sec > target->tv_sec) return true;
    if (now.tv_sec == target->tv_sec && now.tv_nsec >= target->tv_nsec) return true;
    return false;

}

/*
 * track_instruction function
 * Expects: NA
 * Does: Counts how many times its called per second and prints that out every second
 * as instructions per second
 *
 */
int track_instructions() {
    static int instruction_count = 0;
    static struct timespec last_time = {0};
    int temp;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_time.tv_sec == 0) last_time = now; // init first call

    instruction_count++;

    double elapsed = (now.tv_sec - last_time.tv_sec) +
                     (now.tv_nsec - last_time.tv_nsec) / 1e9;

    if (elapsed >= 1.0) {
        printf("Instructions per second: %d\n", instruction_count);
        temp = instruction_count;
        instruction_count = 0;
        last_time = now;
        return(temp);
    }
    return 0;

}

/*
 * millis_since helper function - get_most_recent_input
 * Expects: the start timespec struct to be correctly initialized with a time
 * Does: returns how many milliseconds (int) have passed since the given timespec struct
 *
 */
int millis_since(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long diff_ms = (now.tv_sec - start.tv_sec) * 1000;
    diff_ms += (now.tv_nsec - start.tv_nsec) / 1000000;
    return (int)diff_ms;

}
