#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <mpi.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define DEBUG
#define ROOT 0

#ifdef _WIN32
    #define SLASH "\\"
#elif __linux__
    #define SLASH "/"
#endif

void remove_spaces(char *str) {
    int count = 0;
    /* traverse str If the current character is not a space,
    move it to index count++ */
    for (int i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i];
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
    srand(3);
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

int main(int argc, char** argv){
    int process_Rank, size_Of_Cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);
    #ifdef DEBUG
        printf("%d\n", process_Rank);
    #endif

    uint16_t    size;
    uint16_t    thread_lim;
    double      precision;
    int         start_end_row[2];

    if (process_Rank == ROOT) {
        // FILE*       fpt;
        // command line arguments
        int         num_threads_arg = -1; 
        uint16_t    num_threads;
        // uint16_t    thread_lim; 

        int         size_arg = -1;
        // uint16_t    size;
        uint16_t    size_mutable;

        double      precision_arg = -1.0;
        // double      precision;

        // char*       file_path=NULL;
        // char        out_path[255];

        bool        verbose = false;
        bool        print = false;
        // bool        output = false;

        char        mode = 'g';
        
        int opt;
        
        // struct timespec timer_start, timer_end;

        // while((opt = getopt(argc, argv, ":vaom:p:s:t:f:")) != -1) 
        while((opt = getopt(argc, argv, ":vam:p:s:t:")) != -1) 
        { 
            switch(opt) 
            { 
                case 'v':
                    verbose = true;
                    break;
                case 'a':
                    print = true;
                    break; 
                // case 'o':
                //     output = true;
                //     break;
                case 's': 
                    size_arg = atoi(optarg);
                    break; 
                case 't': 
                    num_threads_arg = atoi(optarg); 
                    break; 
                case 'p': 
                    precision_arg = atof(optarg);
                    break; 
                // case 'f':
                //     remove_spaces(optarg);
                //     file_path = optarg;
                //     break;
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
        
        // snprintf(out_path, sizeof(out_path), \
        //             ".%sres_%dt_%ds_%fp.txt", \
        //                 SLASH, num_threads, size, precision);

        size_mutable = size - 2; // array borders are not mutable
        /* if there are more threads than rows in the array,
        set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
        thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;
        
        /* if thread_lim is not divisible by the mutable array size,
        threads_l = number of threads that need to do more work */
        uint16_t threads_l = size_mutable % thread_lim;

        uint16_t rows_per_thread_s = size_mutable / thread_lim;
        uint16_t rows_per_thread_l = rows_per_thread_s + 1;
        
        double** old_arr = malloc_double_array(size);
        double** new_arr = malloc_double_array(size);
        
        old_arr = debug_populate_array(old_arr, size, mode);
        new_arr = copy_array(old_arr, new_arr, size);
        
        if (print) debug_display_array(old_arr, size);

        

        //clock_gettime(CLOCK_REALTIME, &timer_start); // start timing code here
        for (uint16_t i=0; i<thread_lim; i++) {
            // evenly distribute work among available threads
            if (threads_l > 0) {
                start_end_row[0] = (i>0) ? start_end_row[1]+1 : 1;
                start_end_row[1] = (uint16_t)(start_end_row[0] + rows_per_thread_l - 1);
                threads_l--;
            } else {
                start_end_row[0] = (i>0) ? start_end_row[1]+1 : 1;
                start_end_row[1] = (uint16_t)(start_end_row[0] + rows_per_thread_s - 1);
            }

            if (verbose) {
                printf("thread %d, start row: %d | end row %d\n", i, start_end_row[0], start_end_row[1]);
            }
            MPI_Send(start_end_row, 2, MPI_INT, thread_lim+1, 99, MPI_COMM_WORLD);
        }

        // clock_gettime(CLOCK_REALTIME, &timer_end); // stop timing code
        // double time = (double)(timer_end.tv_sec - timer_start.tv_sec) 
        //                 + ((timer_end.tv_nsec - timer_start.tv_nsec)/1e+9);

        // if (verbose) printf("time: %fs\n", time);

        // if (output) {
        //     if (verbose) printf("outputting to %s\n", out_path);
        //     fpt = fopen(out_path, "w");
        //     if (fpt == NULL) {
        //         perror("fopen");
        //     } else {
        //         fprintf(fpt, "%d, %d, %f, %f\n", \
        //             num_threads, size, time, precision);
        //     }
        // }

        // if (!(file_path==NULL)) {
        //     if (verbose) printf("outputting to %s\n", file_path);
        //     fpt = fopen(file_path, "w");
        //     if (fpt == NULL) {
        //         perror("fopen");
        //     } else {
        //         write_csv(G_utd_arr, size, fpt);
        //         fclose(fpt);
        //     }
        // }

        free_double_array(old_arr, size);
        free_double_array(new_arr, size);
    } else {
        MPI_Status stat;
        MPI_Recv(start_end_row, 2, MPI_INT, ROOT, 99, MPI_COMM_WORLD, &stat);
        printf("I am thread: %d, my start row is %d, my end row is %d\n", process_Rank, start_end_row[0], start_end_row[1]);
    }
    MPI_Finalize();
    return 0;
}