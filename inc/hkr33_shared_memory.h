#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>

#ifndef HKR33_SHARED_MEMORY_H
    #define HKR33_SHARED_MEMORY_H

    double** malloc_double_array(uint16_t size);
    void free_double_array(double** arr, uint16_t size);
    double** debug_populate_array(double** arr, uint16_t size, char mode);
    void debug_display_array(double** arr, uint16_t size);

    typedef struct {
        pthread_mutex_t*    lock;
        uint16_t            thread_num;
        double**            old_arr;
        double**            new_arr;
        double              diff;
        bool                thread_done;
        uint16_t            start_row;
        uint16_t            end_row;
    } thread_args;
#endif