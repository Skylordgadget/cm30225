#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

uint16_t G_num_thrds_complete = 0; 

int main (int argc, char **argv) {
    const char  mode = '1';
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

    bool        verbose = false;
    bool        print = false;
    bool        csv = false;
    
    int opt;
    

    while((opt = getopt(argc, argv, ":cvap:s:t:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'c':
                csv = true;
                break;
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
            case '?': 
                printf("unknown option: %c\n", optopt); 
                break; 
            case ':': 
                printf("missing argument: -%c\n", optopt); 
                break; 
        } 
    } 
    
    // optind is for the extra arguments 
    // which are not parsed 
    for(; optind < argc; optind++){	 
        printf("extra arguments: %s\n", argv[optind]); 
    } 

    // num_threads, size and precision are required
    // if -1 is not overwritten, the program will exit
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
    // if there are more threads than rows in the array,
    // set thread_lim = size_mutable, otherwise thread_lim = num_threads   
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;
    
    pthread_barrier_init(&barrier, NULL, thread_lim);

    // if thread_lim is not divisible by the mutable array size,
    // threads_l = number of threads that need to do more work
    uint16_t threads_l = size_mutable % thread_lim;
    double rows_per_thread = (double)size_mutable / thread_lim;

    uint16_t rows_per_thread_l = (uint16_t)ceil(rows_per_thread);
    uint16_t rows_per_thread_s = (uint16_t)floor(rows_per_thread);
    
    t_args      thrd_args[thread_lim];
    pthread_t   thrds[thread_lim];

    double** old_arr = malloc_double_array(size);
    double** new_arr = malloc_double_array(size);
    double** utd_arr = old_arr; // up-to-date array pointer (initialise as old)

    old_arr = debug_populate_array(old_arr, size, mode);
    new_arr = copy_array(old_arr, new_arr, size);
    
    if (print) debug_display_array(old_arr, size);

    uint16_t str = 0;  
    uint16_t end = 0;

    for (uint16_t i=0; i<thread_lim; i++) {
        // evenly distribute work among available threads
        if (threads_l > 0) {
            str = (i>0) ? end+1 : 1;
            end = (uint16_t)(str+rows_per_thread_l-1);
            threads_l--;
        } else {
            str = (i>0) ? end+1 : 1;
            end = (uint16_t)(str+rows_per_thread_s-1);
        }
        
        thrd_args[i].thread_lim = thread_lim;
        thrd_args[i].thread_num = i;
        thrd_args[i].size_mutable = size_mutable;
        thrd_args[i].precision  = precision;
        thrd_args[i].old_arr    = old_arr;
        thrd_args[i].new_arr    = new_arr;
        thrd_args[i].start_row  = str;
        thrd_args[i].end_row    = end;
        thrd_args[i].barrier    = &barrier;
        thrd_args[i].verbose    = verbose;
        thrd_args[i].threads_complete_mlock = &thrds_complete_mlock;

        pthread_create(&thrds[i], NULL, avg, (void*) &thrd_args[i]);
    }

    for (uint16_t i=0; i<thread_lim; i++){
        pthread_join(thrds[i], NULL);
    }

    pthread_mutex_destroy(&thrds_complete_mlock);
    pthread_barrier_destroy(&barrier);

    if (print) debug_display_array(utd_arr, size);

    if (csv) {
        fpt = fopen(".\\bin\\hkr33_result.csv", "w");
        if (fpt == NULL) perror("fopen");
        write_csv(utd_arr, size, fpt);
        fclose(fpt);
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
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
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
        if (!thread_done) {
            precision_met = true;
            for (uint16_t i=(args->start_row); i<=(args->end_row); i++) {
                for (uint16_t j=1; j<=args->size_mutable; j++) {
                    wr_arr[i][j] = \
                        (double)((ro_arr[i][j-1] + ro_arr[i][j+1] + \
                                  ro_arr[i-1][j] + ro_arr[i+1][j])/4);
                    precision_met &= \
                        fabs(ro_arr[i][j] - wr_arr[i][j]) <= args->precision;
                }
            }
        }
        
        if (args-> verbose) 
            if (precision_met && !thread_done) 
                printf("thread %d met precision\n",args->thread_num);

        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        args->utd_arr = ro_arr;

        pthread_barrier_wait(args->barrier); 

        if (precision_met && !thread_done) {
            thread_done = true;
            pthread_mutex_lock(args->threads_complete_mlock);
            G_num_thrds_complete++;
            pthread_mutex_unlock(args->threads_complete_mlock);
        }

        pthread_barrier_wait(args->barrier);

        if (G_num_thrds_complete==args->thread_lim) {
            break;
        }
    }
    pthread_exit(NULL);
    return 0;
}

