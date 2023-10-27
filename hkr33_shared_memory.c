#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>
#include <unistd.h>
#include <getopt.h>

pthread_cond_t G_thrds_done;
uint16_t G_num_thrds_waiting;
uint16_t G_num_thrds_complete;

int arg_num_threads = -1;
uint16_t num_threads;
int arg_size = -1;
uint16_t size;
uint16_t size_mutable;
double arg_precision = -1.0;
double precision;

bool verbose = false;
bool print = false;

int main (int argc, char **argv) {
    int opt;

	while((opt = getopt(argc, argv, ":vrp:s:t:")) != -1) 
	{ 
		switch(opt) 
		{ 
			case 'v':
                verbose = true;
                break;
			case 'r':
                print = true;
                break; 
			case 's': 
				arg_size = atoi(optarg);
				break; 
            case 't': 
				arg_num_threads = atoi(optarg); 
				break; 
			case 'p': 
				arg_precision = atof(optarg);
				break; 
			case '?': 
				printf("unknown option: %c\n", optopt); 
                break; 
            case ':': 
				printf("missing argument: -%c\n", optopt); 
                break; 
            default:
                printf("unknown error");
                break;
		} 
	} 
	
	// optind is for the extra arguments 
	// which are not parsed 
	for(; optind < argc; optind++){	 
		printf("extra arguments: %s\n", argv[optind]); 
	} 

    if (arg_num_threads == -1) {
        printf("-t required\n");     
        exit(1);   
    } else {
        printf("using %d threads | ", arg_num_threads);
        num_threads = (uint16_t)(arg_num_threads);
    }

    if (arg_size == -1) {
        printf("-s required\n");
        exit(1);
    } else {
        printf("size of %dx%d | ", arg_size, arg_size);
        size = (uint16_t)(arg_size);
    }

    if (arg_precision == -1) {
        printf("-p required\n");
        exit(1);
    } else {
        printf("%f precision\n", arg_precision);
        precision = arg_precision;
    }

    pthread_mutex_t thrds_waiting_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t thrds_complete_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&G_thrds_done, NULL);
    
    size_mutable = (uint16_t)(arg_size - 2);
    uint16_t thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;

    uint16_t threads_l = size_mutable % thread_lim;
    uint16_t threads_s = thread_lim - threads_l;
    double rows_per_thread = (double)size_mutable/thread_lim;
    uint16_t rows_per_thread_l = (uint16_t)ceil(rows_per_thread);
    uint16_t rows_per_thread_s = (uint16_t)floor(rows_per_thread);
    
    t_args      thrd_args[thread_lim];
    pthread_t   thrds[thread_lim];

    printf("thread_lim: %d | rows_per_thread: %f\nthreads_l: %d | rows_per_thread_l: %d\nthreads_s: %d | rows_per_thread_s: %d\n ", thread_lim, rows_per_thread, threads_l, rows_per_thread_l, threads_s, rows_per_thread_s);

    double** old_arr = malloc_double_array(size);
    double** new_arr = malloc_double_array(size);

    old_arr = debug_populate_array(old_arr, size, '1');
    new_arr = copy_array(old_arr, new_arr, size);
    
    //debug_display_array(old_arr, size);

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

    //debug_display_array(new_arr, size);
    free_double_array(old_arr, size);
    free_double_array(new_arr, size);
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
    srand(1);
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

                        arr[i][j] = (double)(rand() % 10);
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
                        arr[i][j] = (double)(rand() % 10);
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
    uint32_t counter = 0;
    //debug_display_array(args->old_arr, size);
    //debug_display_array(args->new_arr, size);

    //printf("hello from thread %d, start row: %d, end row %d\n", args->thread_num, args->start_row, args->end_row);

    while (true) {

        if (counter % 1000 == 0) printf("thread %d has done %d iterations\n", args->thread_num, counter);
        counter++;
        for (uint16_t i=(args->start_row); i<=(args->end_row); i++) {
            for (uint16_t j=1; j<=size_mutable; j++) {
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
            for (uint16_t j=1; j<=size_mutable; j++) {
                precision_met &= fabs(args->old_arr[i][j] - args->new_arr[i][j]) <= precision;
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

