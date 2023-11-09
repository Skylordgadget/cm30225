#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#define DEBUG 1 

#ifdef _WIN32
    #define SLASH "\\"
#elif __linux__
    #define SLASH "/"
#endif

typedef struct thread_args {
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

void remove_spaces(char *str)
{
    // To keep track of non-space character count
    int count = 0;
    // Traverse the provided string. If the current character is not a space,
    //move it to index 'count++'.
    for (int i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i]; // here count is incremented
    str[count] = '\0';
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

// populate 2D array based on selected mode
double** debug_populate_array(double** arr, uint16_t size, char mode) {
    //srand(1);
    for (uint16_t i=0; i<size; i++) {
        for (uint16_t j=0; j<size; j++) {
            /* if at the top or left boundary, set value based on mode
            otherwise, set value 0 */
            
            switch(mode) {
                case '1': // 1s along the top and left, 0s everywhere else
                    if (i==0 || j == 0) arr[i][j] = 1.0; 
                    else arr[i][j] = 0.0;
                    break;
                case 'r': // random numbers between 0 and 9 on the borders
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i][j] = (double)(rand() % 10);
                    else arr[i][j] = 0.0;
                    break;
                case 'q': // random numbers between 0 and 9 for every element
                    arr[i][j] = (double)(rand() % 10);
                    break;
                case 'u': // on the borders, each element = x_idx*y_idx else 0s 
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i][j] = (double)(i*j);
                    else arr[i][j] = 0.0;
                    break;
                case 'g':
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i][j] = 1.0;
                    else arr[i][j] = (double)(rand() % 10);
                    break;
                default: // 0s everywhere
                    printf("mode %c unrecognised", mode);
                    exit(1);
                    break;
            }
        }
    }
    return arr;
}

// print contents of 2D array
void debug_display_array(double** arr, uint16_t size) {
    for (uint16_t i=0; i<size; i++) { 
        for (uint16_t j=0; j<size; j++) printf("%.12f ", arr[i][j]);
        printf("\n");
    }
    printf("\n");
}

// take 2D array and write contents into file using a file pointer
void write_csv(double** arr, uint16_t size, FILE* fpt) {
    for (uint16_t i=0; i<size; i++) { 
        for (uint16_t j=0; j<size; j++) fprintf(fpt, "%.12f,", arr[i][j]);
        fprintf(fpt, "\n");
    }
    fprintf(fpt, "\n");
}

// duplicate contents of 2D array arr_a into arr_b
double** copy_array(double** arr_a, double** arr_b, uint16_t size) {
    for (uint16_t i=0; i<size; i++)  
        for (uint16_t j=0; j<size; j++) arr_b[i][j] = arr_a[i][j];
    return arr_b;
}

// relaxation method function given to each thread
void* avg(void* thrd_args) {
    #ifdef DEBUG
        int counter = 0;
    #endif

    t_args* args = (t_args*) thrd_args;
    bool precision_met = false;

    double** ro_arr = args->old_arr;
    double** wr_arr = args->new_arr;
    double** tmp_arr;

    while (true) {
        #ifdef DEBUG
            // debug count of number of iterations thread 0 performs
            if (args->thread_num==0) counter++;
        #endif


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
                    fabs(ro_arr[i][j] - wr_arr[i][j]) < args->precision;
            }
        }

        // flip pointers
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        /* on the first thread, keep track of the most up-to-date array
        this marginally increases the work of thread 0 */
        if (args->thread_num==0) G_utd_arr = ro_arr; 

        // synchronise threads
        pthread_barrier_wait(args->barrier); 

        /* one-by-one increment the count of complete threads if the
        precision has been met and the thread isn't already done*/
        if (precision_met) {
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
        Otherwise, complete threads will busy wait at barriers
        
        Note that, this shared variable is not mutexed as it is only read after
        threads synchronise at the second barrier and before threads synchronise 
        at the first barrier, hence, it is safe to read without a mutex */
        if (G_num_thrds_complete>=args->thread_lim) break;
        else if (args->thread_num==0) G_num_thrds_complete = 0;

    }

    #ifdef DEBUG
        if (args->thread_num==0) printf("counter: %d\n", counter);
    #endif

    return 0;
}

/* /////////////////////////////////////////////////////////////////////////////
   //                                                                         //
   // main                                                                    //
   //                                                                         //
*/ /////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv) {
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

    char*       file_path=NULL;
    char        out_path[255];

    bool        verbose = false;
    bool        print = false;
    bool        output = false;

    char        mode = 'g';
    
    int opt;
    
    struct timespec timer_start, timer_end;

    while((opt = getopt(argc, argv, ":vaom:p:s:t:f:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'v':
                verbose = true;
                break;
            case 'a':
                print = true;
                break; 
            case 'o':
                output = true;
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
                remove_spaces(optarg);
                file_path = optarg;
                break;
            case 'm':
                remove_spaces(optarg);
                mode = (char)(optarg[0]);
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
        printf("WARNING extra arguments: %s\n", argv[optind]); 
    } 

    /* num_threads, size and precision are required
    if -1 is not overwritten, the program will exit */
    if (num_threads_arg == -1) {
        printf("ERROR -t required\n");     
        exit(1);   
    } else {
        printf("using %d threads | ", num_threads_arg);
        num_threads = (uint16_t)(num_threads_arg);
    }

    if (size_arg == -1) {
        printf("ERROR -s required\n");
        exit(1);
    } else {
        printf("size of %dx%d | ", size_arg, size_arg);
        size = (uint16_t)(size_arg);
    }

    if (precision_arg == -1) {
        printf("ERROR -p required\n");
        exit(1);
    } else {
        printf("%f precision | ", precision_arg);
        precision = precision_arg;
    }

    printf("mode %c\n", mode);
    
    snprintf(out_path, sizeof(out_path), \
                "..%sout%sres_%dt_%ds_%fp.txt", \
                    SLASH, SLASH, num_threads, size, precision);

    pthread_mutex_t thrds_complete_mlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_barrier_t barrier;
 
    size_mutable = size - 2; // array borders are not mutable
    /* if there are more threads than rows in the array,
    set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;
    
    if (pthread_barrier_init(&barrier, NULL, thread_lim) != 0) {
        printf("ERROR pthread barrier failed to initialise\n");
        exit(1);
    }

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

        // spin up threads
        if (pthread_create(&thrds[i], NULL, avg, (void*) &thrd_args[i]) != 0) {
            printf("ERROR pthread create failed at thread %d\n", i);
            exit(1);
        }
    }

    // wait for all threads to finish before continuing  
    for (uint16_t i=0; i<thread_lim; i++) {
        if (pthread_join(thrds[i], NULL) != 0) {
            printf("ERROR pthread join failed at thread %d\n", i);
            exit(1);
        }
    }

    // destroy mutex  
    if (pthread_mutex_destroy(&thrds_complete_mlock) != 0) {
        printf("ERROR pthread mutex destroy failed\n");
        exit(1);
    }
    // destroy barrier  
    if (pthread_barrier_destroy(&barrier) != 0) {
        printf("ERROR pthread barrier destroy failed\n");
        exit(1);
    }

    clock_gettime(CLOCK_REALTIME, &timer_end); // stop timing code
    double time = (double)(timer_end.tv_sec - timer_start.tv_sec) 
                    + ((timer_end.tv_nsec - timer_start.tv_nsec)/1e+9);

    if (verbose) printf("time: %fs\n", time);

    if (print) debug_display_array(G_utd_arr, size);

    if (output) {
        if (verbose) printf("outputting to %s\n", out_path);
        fpt = fopen(out_path, "w");
        if (fpt == NULL) {
            perror("fopen");
        } else {
            fprintf(fpt, "%d, %d, %f, %f", num_threads, size, time, precision);
        }
    }

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