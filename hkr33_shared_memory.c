#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>
#include <unistd.h>
#include <getopt.h>

uint16_t G_num_thrds_complete;

pthread_barrier_t clc_barrier;
pthread_barrier_t cnt_barrier;

int arg_num_threads = -1;
uint16_t num_threads;
uint16_t thread_lim;
int arg_size = -1;
uint16_t size;
uint16_t size_mutable;
double arg_precision = -1.0;
char arg_mode = 'r';
double precision;

bool verbose = false;
bool print = false;

bool arr_flop = false;

int main (int argc, char **argv) {
    int opt;

	while((opt = getopt(argc, argv, ":vrp:s:t:m:")) != -1) 
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
            case 'm': 
				arg_mode = optarg[1];
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

    printf("mode %c | ", arg_mode);

    if (arg_precision == -1) {
        printf("-p required\n");
        exit(1);
    } else {
        printf("%f precision\n", arg_precision);
        precision = arg_precision;
    }

    pthread_mutex_t thrds_complete_mlock = PTHREAD_MUTEX_INITIALIZER;
 
    size_mutable = (uint16_t)(arg_size - 2);
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;
    pthread_barrier_init (&clc_barrier, NULL, thread_lim);
    pthread_barrier_init (&cnt_barrier, NULL, thread_lim);

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

    old_arr = debug_populate_array(old_arr, size, arg_mode);
    new_arr = copy_array(old_arr, new_arr, size);
    
    if (print) debug_display_array(old_arr, size);

    uint16_t str = 0;  
    uint16_t end = 0;

    G_num_thrds_complete = 0;

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
        thrd_args[i].threads_complete_mlock = &thrds_complete_mlock;

        pthread_create(&thrds[i], NULL, avg, (void*) &thrd_args[i]);
    }

    for (uint16_t i=0; i<thread_lim; i++){
        pthread_join(thrds[i], NULL);
    }

    pthread_mutex_destroy(&thrds_complete_mlock);
    pthread_barrier_destroy(&clc_barrier);
    pthread_barrier_destroy(&cnt_barrier);

    if (arr_flop) {
        if (print) debug_display_array(new_arr, size);
    } else {
        if (print) debug_display_array(old_arr ,size);
    }

    
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
    for (uint16_t i=0; i<size; i++) {
        for (uint16_t j=0; j<size; j++) {
            // if at the top or left boundary, set value based on mode
            // otherwise, set value 0
            arr[i][j] = 0.0;
            switch(mode) {
                case '1':
                    arr[i][j] = (double)(i==0 || j == 0);
                    break;
                case 'r':
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) arr[i][j] = (double)(rand() % 10);
                    break;
                default:
                    arr[i][j] = 0.0;
                    break;
            }
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
    bool precision_met = false;
    bool thread_done = false;
    
    uint32_t counter = 0;
    
    double** ro_arr = args->old_arr;
    double** wr_arr = args->new_arr;
    double** tmp_arr;

    while (true) {
        if (args->thread_num==0) arr_flop = !arr_flop;

        counter++;
        
        if (!thread_done) {
            precision_met = true;
            for (uint16_t i=(args->start_row); i<=(args->end_row); i++) {
                for (uint16_t j=1; j<=size_mutable; j++) {
                    wr_arr[i][j] = (double)((ro_arr[i][j-1] + ro_arr[i][j+1] + ro_arr[i-1][j] + ro_arr[i+1][j])/4);
                    precision_met &= fabs(ro_arr[i][j] - wr_arr[i][j]) <= precision;
                }
            }
        }
        
        if (verbose) if (precision_met && !thread_done) printf("thread %d met precision\n",args->thread_num);

        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;

        if (verbose) printf("thread %d waiting at clc_barrier 1 on iteration %d\n", args->thread_num, counter);
        pthread_barrier_wait(&cnt_barrier);
        if (verbose) if (!args->thread_num) printf("threads passing clc_barrier 1\n"); 

        if (precision_met && !thread_done) {
            thread_done = true;
            pthread_mutex_lock(args->threads_complete_mlock);
            G_num_thrds_complete++;
            pthread_mutex_unlock(args->threads_complete_mlock);
            if (verbose) printf("thread %d done, number of threads done: %d\n", args->thread_num, G_num_thrds_complete);
        }

        if (verbose) printf("thread %d waiting at clc_barrier 2 on iteration %d\n", args->thread_num, counter);
        pthread_barrier_wait (&clc_barrier);
        if (verbose) if (!args->thread_num) printf("threads passing clc_barrier 2\n"); 

        if (G_num_thrds_complete==thread_lim) {
            if (verbose) printf("       thread %d breaking\n", args->thread_num);
            break;
        }
    }
    if (args->thread_num==0) printf("arr_flop: %d\n", arr_flop);
    pthread_exit(NULL);
    return 0;
}

