#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>

#define DEBUG_VERBOSE 1

#define DEBUG 1
#define DEBUG_NUM_THREADS 50
#define DEBUG_PRECISION 0.0001
#define DEBUG_SIZE 100
#define DEBUG_SIZE_MUTABLE (DEBUG_SIZE - 2)

pthread_barrier_t barrier;

int main (int argc, char *argv[]) {
    printf("Hello, World! \n");

    uint16_t thread_lim = (DEBUG_SIZE_MUTABLE < DEBUG_NUM_THREADS) ?
                            DEBUG_SIZE_MUTABLE : DEBUG_NUM_THREADS;
    
    pthread_barrier_init(&barrier, NULL, thread_lim);

    uint16_t threads_l = DEBUG_SIZE_MUTABLE % DEBUG_NUM_THREADS;
    uint16_t threads_s = DEBUG_NUM_THREADS - threads_l;
    double rows_per_thread = (double)DEBUG_SIZE_MUTABLE/DEBUG_NUM_THREADS;
    uint16_t rows_per_thread_l = (uint16_t)ceil(rows_per_thread);
    uint16_t rows_per_thread_s = (uint16_t)floor(rows_per_thread);
    
    thread_args thrd_args[thread_lim];
    pthread_t   thrds[thread_lim];

    printf("thread_lim: %d | rows_per_thread: %f\nthreads_l: %d | rows_per_thread_l: %d\nthreads_s: %d | rows_per_thread_s: %d\n ", thread_lim, rows_per_thread, threads_l, rows_per_thread_l, threads_s, rows_per_thread_s);

    double** old_arr = malloc_double_array(DEBUG_SIZE);
    double** new_arr = malloc_double_array(DEBUG_SIZE);

    old_arr = debug_populate_array(old_arr, DEBUG_SIZE, '1');

    uint16_t str = 0;
    uint16_t end = 0;
    for (uint16_t i=0; i<thread_lim; i++) {
        if (threads_l > 0) {
            str = (i>0) ? end+1 : 1;
            end = str+rows_per_thread_l-1;
            threads_l--;
        } else {
            str = (i>0) ? end+1 : 0;
            end = str+rows_per_thread_s-1;
        }
        printf("i: %d | str: %d | end: %d\n", i, str, end);
        
        thrd_args[i].thread_num = i;
        thrd_args[i].old_arr    = old_arr;
        thrd_args[i].new_arr    = new_arr;
        thrd_args[i].diff       = 0.0;
        thrd_args[i].start_row  = str;
        thrd_args[i].end_row    = end;
    }

    pthread_barrier_destroy(&barrier);

    //debug_display_array(arr, DEBUG_SIZE);
    free_double_array(old_arr, DEBUG_SIZE);
    free_double_array(new_arr, DEBUG_SIZE);
    return 0;
}

double** malloc_double_array(uint16_t size) {
    // malloc for the pointers to each row
    double** arr = (double**)malloc(size*sizeof(double*));
    // malloc for each element in each row  
    for (uint16_t i=0; i<size; i++) 
        arr[i] = (double*)malloc(size*sizeof(double));

    return arr;
}

void free_double_array(double** arr, uint16_t size) {
    // free memory for each element in each row
    for (uint16_t i=0; i<size; i++) free(arr[i]);
    // free memory for the pointers to each row
    free(arr);
}

double** debug_populate_array(double** arr, uint16_t size, char mode) {
    for (uint16_t i=0; i<size; i++) {
        for (uint16_t j=0; j<size; j++) {
            // if at the top or left boundary, set value based on mode
            // otherwise, set value 0
            if (i == 0 || j == 0) {
                switch(mode) {
                    case '1':
                        arr[i][j] = 1.0;
                        break;
                    case 'r':
                        // TODO expand this so that the right and bottom boundary are included (also make less shit)
                        // cast random number between 0 and size to double
                        arr[i][j] = (double)(rand() % size);
                        break;
                    default:
                        arr[i][j] = 1.0;
                        break;
                    }
            } else arr[i][j] = 0.0;
        }
    }
    return arr;
}

void debug_display_array(double** arr, uint16_t size) {
    for (uint16_t i=0; i<size; i++) { 
        for (uint16_t j=0; j<size; j++) printf("%.12f ", arr[i][j]);
        printf("\n");
    }
    printf("\n");
}


void* avg(void* thrd_args) {
    thread_args* args = (thread_args*) thrd_args;
    bool thread_done = false;
    while (true) {
        for (uint16_t i=(args->start_row); i<(args->end_row); i++) {
            for (uint16_t j=1; j<DEBUG_SIZE_MUTABLE; j++) {
                args->new_arr[i][j] = (double)((args->old_arr[i][j-1] + args->old_arr[i][j+1] + args->old_arr[i-1][j] + args->old_arr[i+1][j])/4); 
            }
        }

        for (uint16_t i=(args->start_row); i<(args->end_row); i++) {
            for (uint16_t j=1; j<DEBUG_SIZE_MUTABLE; j++) {
                args->diff += fabs(args->old_arr[i][j] - args->new_arr[i][j]);
                args->old_arr[i][j]=args->new_arr[i][j];
            }
        }

    }

    



}

