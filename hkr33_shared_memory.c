#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>

#define DEBUG_VERBOSE 1

#define DEBUG 1
#define DEBUG_NUM_THREADS 4
#define DEBUG_PRECISION 1e-12
#define DEBUG_SIZE 8
#define DEBUG_SIZE_MUTABLE (DEBUG_SIZE - 2)

pthread_cond_t G_thrds_done;
uint16_t G_num_thrds_waiting;
uint16_t G_num_thrds_complete;

int main (int argc, char *argv[]) {
    printf("Hello, World! \n");

    pthread_mutex_t thrds_waiting_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t thrds_complete_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&G_thrds_done, NULL);

    uint16_t thread_lim = (DEBUG_SIZE_MUTABLE < DEBUG_NUM_THREADS) ?
                            DEBUG_SIZE_MUTABLE : DEBUG_NUM_THREADS;

    uint16_t threads_l = DEBUG_SIZE_MUTABLE % thread_lim;
    uint16_t threads_s = thread_lim - threads_l;
    double rows_per_thread = (double)DEBUG_SIZE_MUTABLE/thread_lim;
    uint16_t rows_per_thread_l = (uint16_t)ceil(rows_per_thread);
    uint16_t rows_per_thread_s = (uint16_t)floor(rows_per_thread);
    
    t_args      thrd_args[thread_lim];
    pthread_t   thrds[thread_lim];

    printf("thread_lim: %d | rows_per_thread: %f\nthreads_l: %d | rows_per_thread_l: %d\nthreads_s: %d | rows_per_thread_s: %d\n ", thread_lim, rows_per_thread, threads_l, rows_per_thread_l, threads_s, rows_per_thread_s);

    double** old_arr = malloc_double_array(DEBUG_SIZE);
    double** new_arr = malloc_double_array(DEBUG_SIZE);

    old_arr = debug_populate_array(old_arr, DEBUG_SIZE, 'r');
    new_arr = copy_array(old_arr, new_arr, DEBUG_SIZE);
    
    debug_display_array(old_arr, DEBUG_SIZE);

    uint16_t str = 0;  
    uint16_t end = 0;

    G_num_thrds_complete = 0;
    G_num_thrds_waiting = 0;

    for (uint16_t i=0; i<thread_lim; i++) {
        if (threads_l > 0) {
            str = (i>0) ? end+1 : 1;
            end = (uint16_t)(str+rows_per_thread_l-1);
            threads_l--;
        } else {
            str = (i>0) ? end+1 : 1;
            end = (uint16_t)(str+rows_per_thread_s-1);
        }
        printf("i: %d | str: %d | end: %d\n", i, str, end);
        
        thrd_args[i].thread_lim = thread_lim;
        thrd_args[i].thread_num = i;
        thrd_args[i].old_arr    = old_arr;
        thrd_args[i].new_arr    = new_arr;
        thrd_args[i].start_row  = str;
        thrd_args[i].end_row    = end;
        thrd_args[i].threads_waiting_mlock = &thrds_waiting_mlock;
        thrd_args[i].threads_complete_mlock = &thrds_complete_mlock;

        pthread_create(&thrds[i], NULL, avg, (void*) &thrd_args[i]);
    }

    for (uint16_t i=0; i<thread_lim; i++){
        pthread_join(thrds[i], NULL);
    }

    pthread_mutex_destroy(&thrds_waiting_mlock);
    pthread_mutex_destroy(&thrds_complete_mlock);
    pthread_cond_destroy(&G_thrds_done);

    debug_display_array(new_arr, DEBUG_SIZE);
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
                    case '0':
                        arr[i][j] = 0.0;
                        break;
                    case '1':
                        arr[i][j] = 1.0;
                        break;
                    case 'r':
                        // TODO expand this so that the right and bottom boundary are included (also make less shit)
                        // cast random number between 0 and size to double

                        arr[i][j] = (double)(rand() % size);
                        break;
                    default:
                        arr[i][j] = 0.0;
                        break;
                    }
            } else if (i == size-1 || j == size-1) {
                switch(mode) {
                    case '0':
                        arr[i][j] = 0.0;
                        break;
                    case 'r':
                        // TODO expand this so that the right and bottom boundary are included (also make less shit)
                        // cast random number between 0 and size to double
                        arr[i][j] = (double)(rand() % size);
                        break;
                    default:
                        arr[i][j] = 0.0;
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

double** copy_array(double** arr1, double** arr2, uint16_t size) {
    for (uint16_t i=0; i<size; i++)  
        for (uint16_t j=0; j<size; j++) arr2[i][j] = arr1[i][j];
    return arr2;
}

void* avg(void* thrd_args) {
    t_args* args = (t_args*) thrd_args;
    bool precision_met;
    bool thread_done = false;
    //debug_display_array(args->old_arr, DEBUG_SIZE);
    //debug_display_array(args->new_arr, DEBUG_SIZE);

    //printf("hello from thread %d, start row: %d, end row %d\n", args->thread_num, args->start_row, args->end_row);

    while (true) {

        
        for (uint16_t i=(args->start_row); i<=(args->end_row); i++) {
            for (uint16_t j=1; j<=DEBUG_SIZE_MUTABLE; j++) {
                args->new_arr[i][j] = (double)((args->old_arr[i][j-1] + args->old_arr[i][j+1] + args->old_arr[i-1][j] + args->old_arr[i+1][j])/4);
                //printf("loop 1 - old value: %f | new value %f\n", args->old_arr[i][j], args->new_arr[i][j] );
                
            }
        }
        pthread_mutex_lock(args->threads_waiting_mlock);
        G_num_thrds_waiting++;



        if (G_num_thrds_waiting < args->thread_lim - G_num_thrds_complete) {
            pthread_cond_wait(&G_thrds_done, args->threads_waiting_mlock);
        } else {
            pthread_cond_broadcast(&G_thrds_done);
            G_num_thrds_waiting = 0;
        }
        pthread_mutex_unlock(args->threads_waiting_mlock);

        

        precision_met = true;
        for (uint16_t i=(args->start_row); i<=(args->end_row); i++) {
            for (uint16_t j=1; j<=DEBUG_SIZE_MUTABLE; j++) {
                precision_met &= fabs(args->old_arr[i][j] - args->new_arr[i][j]) <= DEBUG_PRECISION;
                args->old_arr[i][j]=args->new_arr[i][j];
            }
        }

        pthread_mutex_lock(args->threads_waiting_mlock);
        G_num_thrds_waiting++;

        pthread_mutex_lock(args->threads_complete_mlock);
        //printf("thread %d waiting, number of threads waiting: %d\n", args->thread_num, G_num_thrds_waiting);
        if (precision_met) {
            G_num_thrds_complete++;
            thread_done = true;
            //printf("thread %d done, number of threads done: %d\n", args->thread_num, G_num_thrds_complete);
        }
        pthread_mutex_unlock(args->threads_complete_mlock);

        if (G_num_thrds_waiting < args->thread_lim - G_num_thrds_complete) {
            pthread_cond_wait(&G_thrds_done, args->threads_waiting_mlock);
        } else {
            pthread_cond_broadcast(&G_thrds_done);
            G_num_thrds_waiting = 0;
        }
        pthread_mutex_unlock(args->threads_waiting_mlock);

        if (thread_done) break;
    }

    pthread_exit(NULL);
    return 0;
}

