#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>

#ifndef HKR33_SHARED_MEMORY_H
    #define HKR33_SHARED_MEMORY_H

    double**    malloc_double_array(uint16_t size);
    
    void        free_double_array(double** arr, uint16_t size);
    
    double**    debug_populate_array(double** arr, uint16_t size, char mode);
    
    void        debug_display_array(double** arr, uint16_t size);

    double**    copy_array(double** arr1, double** arr2, uint16_t size);

    void*       avg(void* thrd_args);

    void        write_csv(double** arr, uint16_t size, FILE* fpt);

    typedef struct thread_args{
        uint16_t            thread_lim;
        uint16_t            thread_num;
        uint16_t            size_mutable;
        bool                verbose;
        double              precision;
        double**            old_arr;
        double**            new_arr;
        double**            utd_arr;
        uint16_t            start_row;
        uint16_t            end_row;   
        pthread_mutex_t*    threads_complete_mlock;
        pthread_barrier_t*  barrier;
    } t_args;
#endif