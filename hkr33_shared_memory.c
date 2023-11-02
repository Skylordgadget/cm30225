#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

typedef struct thread_args{
        uint16_t            thread_lim;
        uint16_t            thread_num;
        uint16_t            size_mutable;
        double              precision;
        double**            old_arr;
        double**            new_arr;
        uint16_t            start_row;
        uint16_t            end_row;   
        pthread_mutex_t*    threads_complete_mlock;
        pthread_barrier_t*  barrier;
    } t_args;

uint16_t G_num_thrds_complete = 0; 
double** G_utd_arr; // up-to-date array pointer


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
            /* if at the top or left boundary, set value based on mode
            otherwise, set value 0 */
            
            switch(mode) {
                case '1':
                    arr[i][j] = (double)(i==0 || j == 0);
                    break;
                case 'r':
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i][j] = (double)(rand() % 10);
                    else arr[i][j] = 0.0;
                    break;
                case 'q':
                    arr[i][j] = (double)(rand() % 10);
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


void write_csv(double** arr, uint16_t size, FILE* fpt) {
    for (uint16_t i=0; i<size; i++) { 
        for (uint16_t j=0; j<size; j++) fprintf(fpt, "%.12f,", arr[i][j]);
        fprintf(fpt, "\n");
    }
    fprintf(fpt, "\n");
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

    double** ro_arr = args->old_arr;
    double** wr_arr = args->new_arr;
    double** tmp_arr;

    while (true) {
        /* if the thread hasn't already finished it may continue averaging.
        Otherwise, it will skip this and hit the first barrier */
        if (!thread_done) {
            precision_met = true;
            // loop over rows thread will operate on
            for (uint16_t i=args->start_row; i<=args->end_row; i++) {
                // loop over columns
                for (uint16_t j=1; j<=args->size_mutable; j++) {
                    // add up surrounding four value and divide by four
                    wr_arr[i][j] = (ro_arr[i][j-1] + ro_arr[i][j+1] + \
                                    ro_arr[i-1][j] + ro_arr[i+1][j])/4.0;
                    /* precision_met starts as 1, if any element the thread is
                    working on has not met precision it will turn the 
                    variable 0, staying 0 until the next iteration */
                    precision_met &= \
                        fabs(ro_arr[i][j] - wr_arr[i][j]) <= args->precision;
                }
            }
        }

        // flip pointers
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        /* on the first thread, keep track of the most up-to-date array
        this increases the work of thread 0 */
        if (args->thread_num==0) G_utd_arr = ro_arr; 

        // synchronise threads
        pthread_barrier_wait(args->barrier); 

        /* one-by-one increment the count of complete threads if the
        precision has been met and the thread isn't already done*/
        if (precision_met && !thread_done) {
            thread_done = true;
            pthread_mutex_lock(args->threads_complete_mlock);
                ++G_num_thrds_complete;
            pthread_mutex_unlock(args->threads_complete_mlock);
        }

        // synchronise threads again 
        pthread_barrier_wait(args->barrier);
        /* this prevents threads leaving before others have reached the first 
        barrier as well as stopping threads updaing the array before others are
        done averaging */

        /* when all threads have met precision, break out of the loop.
        Otherwise, complete threads will busy wait at barriers*/
        if (G_num_thrds_complete>=args->thread_lim) break;
    }
    return 0;
}

int main (int argc, char **argv) {
    const char  mode = 'q';
    FILE*       fpt;
    // command line arguments
    int         num_threads_arg = -1; 
    uint16_t    num_threads;
    uint16_t    thread_lim; 

    int         size_arg = -1;
    uint16_t    size;
    uint16_t    size_mutable;

    double      precision_arg = -1.0;
    double      precision;

    char*       file_path;

    bool        verbose = false;
    bool        print = false;
    
    int opt;
    
    struct timespec timer_start, timer_end;

    while((opt = getopt(argc, argv, ":vap:s:t:f:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'v':
                verbose = true;
                break;
            case 'a':
                print = true;
                break; 
            case 's': 
                size_arg = atoi(optarg);
                break; 
            case 't': 
                num_threads_arg = atoi(optarg); 
                break; 
            case 'p': 
                precision_arg = atof(optarg);
                break; 
            case 'f':
                file_path = optarg;
                break;
            case '?': 
                printf("unknown option: %c\n", optopt); 
                break; 
            case ':': 
                printf("missing argument: -%c\n", optopt); 
                break; 
        } 
    } 
    
    // optind is for the extra arguments which are not parsed
    for(; optind < argc; optind++){	 
        printf("extra arguments: %s\n", argv[optind]); 
    } 

    /* num_threads, size and precision are required
    if -1 is not overwritten, the program will exit */
    if (num_threads_arg == -1) {
        printf("-t required\n");     
        exit(1);   
    } else {
        printf("using %d threads | ", num_threads_arg);
        num_threads = (uint16_t)(num_threads_arg);
    }

    if (size_arg == -1) {
        printf("-s required\n");
        exit(1);
    } else {
        printf("size of %dx%d | ", size_arg, size_arg);
        size = (uint16_t)(size_arg);
    }

    if (precision_arg == -1) {
        printf("-p required\n");
        exit(1);
    } else {
        printf("%f precision | ", precision_arg);
        precision = precision_arg;
    }

    printf("mode %c\n", mode);

    pthread_mutex_t thrds_complete_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_barrier_t barrier;
 
    size_mutable = size - 2; // array borders are not mutable
    /* if there are more threads than rows in the array,
    set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;
    
    pthread_barrier_init(&barrier, NULL, thread_lim);

    /* if thread_lim is not divisible by the mutable array size,
    threads_l = number of threads that need to do more work */
    uint16_t threads_l = size_mutable % thread_lim;

    double rows_per_thread = (double)size_mutable / thread_lim;

    uint16_t rows_per_thread_l = (uint16_t)ceil(rows_per_thread);
    uint16_t rows_per_thread_s = (uint16_t)floor(rows_per_thread);
    
    t_args      thrd_args[thread_lim];
    pthread_t   thrds[thread_lim];

    double** old_arr = malloc_double_array(size);
    double** new_arr = malloc_double_array(size);
    
    old_arr = debug_populate_array(old_arr, size, mode);
    new_arr = copy_array(old_arr, new_arr, size);
    
    if (print) debug_display_array(old_arr, size);

    uint16_t start = 0;  
    uint16_t end = 0;

    clock_gettime(CLOCK_REALTIME, &timer_start); // start timing code here
    for (uint16_t i=0; i<thread_lim; i++) {
        // evenly distribute work among available threads
        if (threads_l > 0) {
            start = (i>0) ? end+1 : 1;
            end = (uint16_t)(start + rows_per_thread_l - 1);
            threads_l--;
        } else {
            start = (i>0) ? end+1 : 1;
            end = (uint16_t)(start + rows_per_thread_s - 1);
        }

        if (verbose) {
            printf("thread %d, start row: %d | end row %d\n", i, start, end);
        }
        
        thrd_args[i].thread_lim = thread_lim;
        thrd_args[i].thread_num = i;
        thrd_args[i].size_mutable = size_mutable;
        thrd_args[i].precision = precision;
        thrd_args[i].old_arr = old_arr;
        thrd_args[i].new_arr = new_arr;
        thrd_args[i].start_row = start;
        thrd_args[i].end_row = end;
        thrd_args[i].barrier = &barrier;
        thrd_args[i].threads_complete_mlock = &thrds_complete_mlock;

        pthread_create(&thrds[i], NULL, avg, (void*) &thrd_args[i]);
    }

    for (uint16_t i=0; i<thread_lim; i++){
        pthread_join(thrds[i], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &timer_end); // stop timing code
    printf("time: %fs\n", (timer_end.tv_nsec - timer_start.tv_nsec)/1e+9);

    pthread_mutex_destroy(&thrds_complete_mlock);
    pthread_barrier_destroy(&barrier);

    if (print) debug_display_array(G_utd_arr, size);

    if (!(file_path==NULL)) {
        if (verbose) printf("outputting to %s\n", file_path);
        fpt = fopen(file_path, "w");
        if (fpt == NULL) {
            perror("fopen");
        } else {
            write_csv(G_utd_arr, size, fpt);
            fclose(fpt);
        }
    }

    free_double_array(old_arr, size);
    free_double_array(new_arr, size);
    return 0;
}