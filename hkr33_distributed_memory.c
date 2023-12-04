#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <mpi.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#undef DEBUG
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

int malloc2ddouble(double ***arr, uint32_t size) {
    double *p = (double *)malloc(size*size*sizeof(double));
    if (!p) return -1;

    /* allocate the row pointers into the memory */
    (*arr) = (double **)malloc(size*sizeof(double*));
    if (!(*arr)) {
       free(p);
       return -1;
    }

    /* set up the pointers into the contiguous memory */
    for (uint32_t i=0; i<size; i++) 
       (*arr)[i] = &(p[i*size]);

    return 0;
}

int free2ddouble(double ***arr) {
    /* free the memory - the first element of the array is at the start */
    free(&((*arr)[0][0]));

    /* free the pointers into the memory */
    free(*arr);

    return 0;
}

// populate 2D array based on selected mode
double** debug_populate_array(double** arr, int size, char mode) {
    srand(3);
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
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
void debug_display_array(double** arr, int size) {
    for (int i=0; i<size; i++) { 
        for (int j=0; j<size; j++) printf("%.12f ", arr[i][j]);
        printf("\n");
    }
    printf("\n");
}

// take 2D array and write contents into file using a file pointer
void write_csv(double** arr, int size, FILE* fpt) {
    for (int i=0; i<size; i++) { 
        for (int j=0; j<size; j++) fprintf(fpt, "%.12f,", arr[i][j]);
        fprintf(fpt, "\n");
    }
    fprintf(fpt, "\n");
}

// duplicate contents of 2D array arr_a into arr_b
double** copy_array(double** arr_a, double** arr_b, int size) {
    for (int i=0; i<size; i++)  
        for (int j=0; j<size; j++) arr_b[i][j] = arr_a[i][j];
    return arr_b;
}

/* /////////////////////////////////////////////////////////////////////////////
   //                                                                         //
   // Relax                                                                   //
   //                                                                         //
*/ /////////////////////////////////////////////////////////////////////////////
double** avg(int rank, double** wr_arr, double** ro_arr, double** res_arr, int start_row, \
                int end_row, int thread_lim, int size, double precision) {

    MPI_Status stat;
    #ifdef DEBUG
        int counter = 0;
    #endif
    int precision_met = 0;
    double** tmp_arr;
    double** utd_arr;
    int size_mutable = size - 2;
    int num_rows = end_row - start_row + 1;

    while (true) {
        #ifdef DEBUG
            // debug count of number of iterations thread 0 performs
            if (rank==ROOT) counter++;
        #endif
        precision_met = 1;
        // loop over rows thread will operate on
        for (int i=start_row; i<=end_row; i++) {
            // loop over columns
            for (int j=1; j<=size_mutable; j++) {
                // add up surrounding four value and divide by four
                wr_arr[i][j] = (ro_arr[i][j-1] + ro_arr[i][j+1] + \
                                ro_arr[i-1][j] + ro_arr[i+1][j])/4.0;             
                /* precision_met starts as 1, if any element the thread is
                working on has not met precision it will turn the 
                variable 0, staying 0 until the next iteration */
                precision_met &= \
                    fabs(ro_arr[i][j] - wr_arr[i][j]) < precision;
            }
        }

        
        // exchange start and end rows to adjacent threads
        // this is a slow operation -- dominating the execution time    
        if (rank != thread_lim-1) {
            #ifdef DEBUG
                printf("I am thread: %d -- sending row %d to rank %d with tag \
                            %d\n\n", rank, end_row, rank+1, rank*10+(rank+1));
            #endif
            MPI_Send(&wr_arr[end_row][0], size, MPI_DOUBLE, rank+1, \
                        rank*10 + (rank+1), MPI_COMM_WORLD);      
        }
        if (rank != ROOT) {
            #ifdef DEBUG
                printf("I am thread: %d -- sending row %d to rank %d with tag \
                            %d\n\n", rank, start_row, rank-1, rank*10+(rank-1));
            #endif
            MPI_Send(&wr_arr[start_row][0], size, MPI_DOUBLE, rank-1, \
                        rank*10 + (rank-1), MPI_COMM_WORLD);  
        }
        if (rank != thread_lim-1) {
            #ifdef DEBUG
                printf("I am thread: %d -- receiving row %d from rank %d \
                            with tag %d\n\n", rank, end_row+1, rank+1, \
                            (rank+1)*10+rank);
            #endif
            MPI_Recv(&wr_arr[end_row+1][0], size, MPI_DOUBLE, rank+1, \
                        (rank+1)*10 + rank, MPI_COMM_WORLD, &stat);
        }
        if (rank != ROOT) {
            #ifdef DEBUG
                printf("I am thread: %d -- receiving row %d from rank %d \
                        with tag %d\n\n", rank, start_row-1, rank-1, \
                        (rank-1)*10 + rank);
            #endif
            MPI_Recv(&wr_arr[start_row-1][0], size, MPI_DOUBLE, rank-1, \
                        (rank-1)*10 + rank, MPI_COMM_WORLD, &stat);
        }
        // flip pointers
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        // keep track of the most up-to-date array
        utd_arr = ro_arr; 

        int precision_met_all;
        // Sum the precision met flag from all threads  
        MPI_Reduce(&precision_met, &precision_met_all, 1, MPI_INT, MPI_SUM, \
                        ROOT, MPI_COMM_WORLD);
        // Broadcast the result to all threads
        MPI_Bcast(&precision_met_all, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
        // If precision has been met on all threads, break out of the while loop
        if (precision_met_all == thread_lim) break;
    }



    MPI_Gather(&utd_arr[start_row][0], size*num_rows, MPI_DOUBLE, &res_arr[1][0], size*num_rows, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
    
    if (rank==ROOT) {
        res_arr[0] = ro_arr[0];
        res_arr[size-1] = ro_arr[size-1];
    }

    #ifdef DEBUG
        if (rank==ROOT) printf("iterations: %d\n", counter);
    #endif
    return res_arr;
}

int main(int argc, char** argv){
    int rank, size_of_cluster;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size_of_cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    #ifdef DEBUG
        printf("%d\n", rank);
    #endif

    FILE*       fpt;
    // command line arguments
    int         num_threads = -1;
    int         thread_lim;

    int         size = -1;   
    int         size_mutable;

    // double      precision_arg = -1.0;
    double      precision = -1.0;

    char*       file_path=NULL;

    bool        verbose = false;
    bool        print = false;

    char        mode = 'g';
    
    int opt;
    
    // struct timespec timer_start, timer_end;

    // while((opt = getopt(argc, argv, ":vaom:p:s:t:f:")) != -1) 
    while((opt = getopt(argc, argv, ":vam:p:s:t:f:")) != -1) 
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
                size = atoi(optarg);
                break; 
            case 't': 
                num_threads = atoi(optarg); 
                break; 
            case 'p': 
                precision = atof(optarg);
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
    
    if (rank==ROOT) {
        // optind is for the extra arguments which are not parsed
        for(; optind < argc; optind++){	 
            printf("WARNING extra arguments: %s\n", argv[optind]); 
        } 

        /* num_threads, size and precision are required
        if -1 is not overwritten, the program will exit */
        if (num_threads == -1) {
            printf("ERROR -t required\n");     
            exit(1);   
        } else {
            printf("using %d threads | ", num_threads);
        }

        if (size == -1) {
            printf("ERROR -s required\n");
            exit(1);
        } else {
            printf("size of %dx%d | ", size, size);
        }

        if (precision == -1) {
            printf("ERROR -p required\n");
            exit(1);
        } else {
            printf("%f precision | ", precision);
        }

        printf("mode %c\n", mode);
    }

    size_mutable = size - 2; // array borders are not mutable
    /* if there are more threads than rows in the array,
    set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;

    if (size_of_cluster > thread_lim) {
        printf("Application started with too many threads.\nLimit for size %d: %d", size, thread_lim);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    /* if thread_lim is not divisible by the mutable array size,
    threads_l = number of threads that need to do more work */
    int threads_l = size_mutable % thread_lim;

    int rows_per_thread_s = size_mutable / thread_lim;
    
    int rows_per_thread_l = rows_per_thread_s + 1;

    int start_row = 0;
    int end_row = 0;

    // evenly distribute work among available threads
    if (rank*rows_per_thread_l < rows_per_thread_l*threads_l) {
        start_row = (rank*rows_per_thread_l)+1;
        end_row = (rank+1)*rows_per_thread_l;
    } else {
        start_row = (rows_per_thread_l*threads_l + (rank-threads_l)*rows_per_thread_s)+1;
        end_row = rows_per_thread_l*threads_l + (rank-threads_l+1)*rows_per_thread_s;
    }

    if (verbose) printf("I am thread: %d, my start row is %d, my end row is %d\n", rank, start_row, end_row);

    double **wr_arr;
    malloc2ddouble(&wr_arr, (uint32_t)(size));
    double **ro_arr;
    malloc2ddouble(&ro_arr, (uint32_t)(size));
    double** res_arr;
    malloc2ddouble(&res_arr, (uint32_t)(size));

    ro_arr = debug_populate_array(ro_arr, size, mode);
    wr_arr = copy_array(ro_arr, wr_arr, size);
    
    wr_arr = avg(rank, wr_arr, ro_arr, res_arr, start_row, end_row, thread_lim, size, precision);

    if (rank==ROOT) {
        if (!(file_path==NULL)) {
            if (verbose) printf("outputting to %s\n", file_path);
            fpt = fopen(file_path, "w");
            if (fpt == NULL) {
                perror("fopen");
            } else {
                write_csv(wr_arr, size, fpt);
                fclose(fpt);
            }
        }

        if (print) debug_display_array(wr_arr, size);
    }

    free2ddouble(&ro_arr);
    free2ddouble(&wr_arr);
    free2ddouble(&res_arr);

    MPI_Finalize();
    return 0;
}