#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <mpi.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#undef DEBUG // define to enable extra printed debug info
#define ROOT 0 // MPI root process

#ifdef _WIN32
    #define SLASH "\\"
#elif __linux__
    #define SLASH "/"
#endif

// removes spaces from a given string
void remove_spaces(char *str) {
    int count = 0;
    /* traverse str If the current character is not a space,
    move it to index count++ */
    for (int i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i];
    str[count] = '\0';
}

// // allocate contiguous memory for a 2D array of doubles
// int malloc2ddouble(double ***arr, uint32_t size) {
//     double *p = (double *)malloc(size*size*sizeof(double));
//     if (!p) return -1;

//     // allocate the row pointers into the memory
//     (*arr) = (double **)malloc(size*sizeof(double*));
//     if (!(*arr)) {
//        free(p);
//        return -1;
//     }

//     // set up the pointers into the contiguous memory
//     for (uint32_t i=0; i<size; i++) 
//        (*arr)[i] = &(p[i*size]);

//     return 0;
// }

// // free memory allocated for a 2D array of doubles
// int free2ddouble(double ***arr) {
//     // free the memory - the first element of the array is at the start
//     free(&((*arr)[0][0]));

//     // free the pointers into the memory
//     free(*arr);

//     return 0;
// }

// populate 2D array based on selected mode
double* debug_populate_array(double* arr, int size, char mode) {
    srand(3); // seed for random numbers
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
            /* if at the top or left boundary, set value based on mode
            otherwise, set value 0 */
            
            switch(mode) {
                case '1': // 1s along the top and left, 0s everywhere else
                    if (i==0 || j == 0) arr[i * size + j] = 1.0; 
                    else arr[i * size + j] = 0.0;
                    break;
                case 'r': // random numbers between 0 and 9 on the borders
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i * size + j] = (double)(rand() % 10);
                    else arr[i * size + j] = 0.0;
                    break;
                case 'q': // random numbers between 0 and 9 for every element
                    arr[i * size + j] = (double)(rand() % 10);
                    break;
                case 'u': // on the borders, each element = x_idx*y_idx else 0s 
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i * size + j] = (double)(i*j);
                    else arr[i * size + j] = 0.0;
                    break;
                case 'g':
                    if (i == 0 || j == 0 || i == size-1 || j == size-1) 
                        arr[i * size + j] = 1.0;
                    else arr[i * size + j] = (double)(rand() % 10);
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
void debug_display_array(double* arr, int size) {
    for (int i=0; i<size; i++) { 
        for (int j=0; j<size; j++) printf("%.12f ", arr[i * size + j]);
        printf("\n");
    }
    printf("\n");
}

// take 2D array and write contents into file using a file pointer
void write_csv(double* arr, int size, FILE* fpt) {
    for (int i=0; i<size; i++) { 
        for (int j=0; j<size; j++) fprintf(fpt, "%.12f,", arr[i * size + j]);
        fprintf(fpt, "\n");
    }
    fprintf(fpt, "\n");
}

// duplicate contents of 2D array arr_a into arr_b
double* copy_array(double* arr_a, double* arr_b, int size) {
    for (int i=0; i<size; i++)  
        for (int j=0; j<size; j++) arr_b[i * size + j] = arr_a[i * size + j];
    return arr_b;
}

/* /////////////////////////////////////////////////////////////////////////////
   //                                                                         //
   // Relax                                                                   //
   //                                                                         //
   /////////////////////////////////////////////////////////////////////////////
    Takes an array of doubles and repeatedly replaces each with the average of 
    its four neighbours; excepting boundary values (row 0, end row, col 0, 
    end col), which remain fixed. This is repeated until all values settle down 
    to within a given precision. 

    rank:       process rank of each MPI thread
    wr_arr:     2D array written to by threads whilst relaxing
    ro_arr:     2D array read from by threads whilst relaxing
    res_arr:    final result of relaxation
    start_row:  index in ro_arr/wr_arr that each thread starts relaxing from
    end_row:    index in ro_arr/wr_arr that each thread stops relaxing at
    thread_lim: maximum number of threads used in the application
    size:       width and height of wr_arr/ro_arr/res_arr
    precision:  precision to which the relaxtion is calculated to
*/
void avg(int rank, double* wr_arr, double* ro_arr, double* res_arr, \
            int start_row, int end_row, int thread_lim, int size, \
            double precision) {
    MPI_Status stat;
    #ifdef DEBUG
        int counter = 0;
    #endif
    int precision_met = 0;
    double* tmp_arr;
    double* utd_arr;
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
                wr_arr[i * size + j] = (ro_arr[i * size + (j-1)] + ro_arr[i * size + (j+1)] + \
                                ro_arr[(i-1) * size + j] + ro_arr[(i+1) * size + j])/4.0;             
                /* precision_met starts as 1, if any element the thread is
                working on has not met precision it will turn the 
                variable 0, staying 0 until the next iteration */
                precision_met &= \
                    fabs(ro_arr[i * size + j] - wr_arr[i * size + j]) < precision;
            }
        } 

        /* Exchange start and end rows of adjacent threads.
        This is a slow operation -- likely dominating execution time
        
        rank != thread_lim-1:
            prevent top/bottom thread from trying to send to a thread that's 
            out of bounds 
        rank != ROOT:
            prevent top/bottom thread trying to receive from a thread that's 
            out of bounds

        Both sends are carried out before both receives to prevent a deadlock
        where a blocking receive may be called before its companion send,
        thus blocking forever.   
        */
        if (rank != thread_lim-1) {
            #ifdef DEBUG
                printf("I am thread: %d -- sending row %d to rank %d with tag \
                            %d\n\n", rank, end_row, rank+1, rank*10+(rank+1));
            #endif
            // send bottom row to next rank
            MPI_Send(&wr_arr[end_row * size], size, MPI_DOUBLE, rank+1, \
                        rank*10 + (rank+1), MPI_COMM_WORLD);      
        }
        if (rank != ROOT) {
            #ifdef DEBUG
                printf("I am thread: %d -- sending row %d to rank %d with tag \
                            %d\n\n", rank, start_row, rank-1, rank*10+(rank-1));
            #endif
            // send top row to previous rank
            MPI_Send(&wr_arr[start_row * size], size, MPI_DOUBLE, rank-1, \
                        rank*10 + (rank-1), MPI_COMM_WORLD);  
        }
        if (rank != thread_lim-1) {
            #ifdef DEBUG
                printf("I am thread: %d -- receiving row %d from rank %d \
                            with tag %d\n\n", rank, end_row+1, rank+1, \
                            (rank+1)*10+rank);
            #endif
            // receive bottom row + 1 from next rank 
            MPI_Recv(&wr_arr[(end_row+1) * size], size, MPI_DOUBLE, rank+1, \
                        (rank+1)*10 + rank, MPI_COMM_WORLD, &stat);
        }
        if (rank != ROOT) {
            #ifdef DEBUG
                printf("I am thread: %d -- receiving row %d from rank %d \
                        with tag %d\n\n", rank, start_row-1, rank-1, \
                        (rank-1)*10 + rank);
            #endif
            // receive top row + 1 from previous rank
            MPI_Recv(&wr_arr[(start_row-1) * size], size, MPI_DOUBLE, rank-1, \
                        (rank-1)*10 + rank, MPI_COMM_WORLD, &stat);
        }
        // flip pointers
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        // keep track of the most up-to-date array
        utd_arr = ro_arr; 

        int precision_met_all;
        // logical AND the precision met flag from all threads  
        MPI_Reduce(&precision_met, &precision_met_all, 1, MPI_INT, MPI_LAND, \
                        ROOT, MPI_COMM_WORLD);
        // broadcast the result to all threads
        MPI_Bcast(&precision_met_all, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
        // if precision has been met on all threads, break out of the while loop
        if (precision_met_all) break;
    }

    // gather all rows from the most up-to-date array and store them in res_arr
    MPI_Gather(&utd_arr[start_row * size], size*num_rows, MPI_DOUBLE, \
                &res_arr[size], size*num_rows, MPI_DOUBLE, \
                ROOT, MPI_COMM_WORLD);

    #ifdef DEBUG
        if (rank==ROOT) printf("iterations: %d\n", counter);
    #endif
    
}

/* /////////////////////////////////////////////////////////////////////////////
   //                                                                         //
   // Main                                                                    //
   //                                                                         //
*/ /////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv){
    int rank, size_of_cluster;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size_of_cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    #ifdef DEBUG
        printf("I am rank %d\n", rank);
    #endif

    FILE*       fpt;
    // command line arguments
    int         num_threads = -1;
    int         thread_lim;

    int         size = -1;   
    int         size_mutable;

    double      precision = -1.0;

    char*       file_path=NULL;

    bool        verbose = false;
    bool        print = false;

    char        mode = 'g';
    
    int opt;
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
    
    size_mutable = size - 2; // array borders are not mutable

    /* if there are more threads than rows in the array,
    set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
    thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads;

    // exit the program if there are too many threads being used
    if (size_of_cluster > thread_lim) {
        printf("Application started with too many threads.\n \
                Limit for size %d: %d", size, thread_lim);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    /* ========================================================================= 
    Evenly distribute work among available threads.

    The start and end row for each thread is calculated using only it's rank. 
    For the first 'thread_l' number of threads, each thread has
    'rows_per_thread_l' rows. For the remaining threads, 
    each thread has 'rows_per_thread_s' rows.

    For example:
    size = 10; thread_lim = 3;
    
    size_mutable: 8
    thread_l: 2
    rows_per_thread_s: 2
    rows_per_thread_l: 3

    thread 0 | start row 1 | end row 3 | row width 3
    thread 1 | start row 4 | end row 6 | row width 3
    thread 2 | start row 7 | end row 8 | row width 2 
    */

    /* if thread_lim is not divisible by the mutable array size,
    threads_l = number of threads that need to do more work */
    int threads_l = size_mutable % thread_lim;

    int rows_per_thread_s = size_mutable / thread_lim;
    int rows_per_thread_l = rows_per_thread_s + 1;

    int start_row = 0;
    int end_row = 0;

    if (rank*rows_per_thread_l < rows_per_thread_l*threads_l) {
        start_row = (rank*rows_per_thread_l)+1;
        end_row = (rank+1)*rows_per_thread_l;
    } else {
        start_row = (rows_per_thread_l*threads_l + \
                        (rank-threads_l)*rows_per_thread_s)+1;
        end_row = rows_per_thread_l*threads_l + \
                        (rank-threads_l+1)*rows_per_thread_s;
    }
    // =========================================================================

    if (verbose) printf("I am thread: %d, my start row is %d, \
                            my end row is %d\n", rank, start_row, end_row);

    // allocate memory for wr_arr/ro_arr/res_arr
    // double **wr_arr;
    // malloc2ddouble(&wr_arr, (uint32_t)(size));
    // double **ro_arr;
    // malloc2ddouble(&ro_arr, (uint32_t)(size));
    // double** res_arr;
    // malloc2ddouble(&res_arr, (uint32_t)(size));

    double *wr_arr = malloc(sizeof *wr_arr * (uint32_t)(size * size));
    double *ro_arr = malloc(sizeof *ro_arr * (uint32_t)(size * size));
    double *res_arr = malloc(sizeof *res_arr * (uint32_t)(size * size));

    // populate ro_arr and copy it's contents to wr_arr/res_arr
    ro_arr = debug_populate_array(ro_arr, size, mode);
    wr_arr = copy_array(ro_arr, wr_arr, size);
    res_arr = copy_array(ro_arr, res_arr, size);
    
    /* =========================================================================
    relax the data */
    avg(rank, wr_arr, ro_arr, res_arr, start_row, end_row, thread_lim, size, \
            precision);
    // =========================================================================

    /* If a file path has been specified, write the contents of res_arr to it.
    Only do this sequentially on the root thread.*/
    if (rank==ROOT) {
        if (!(file_path==NULL)) {
            if (verbose) printf("outputting to %s\n", file_path);
            fpt = fopen(file_path, "w");
            if (fpt == NULL) {
                perror("fopen");
            } else {
                write_csv(res_arr, size, fpt);
                fclose(fpt);
            }
        }

        if (print) debug_display_array(res_arr, size);
    }

    // free the allocated memory for ro_arr/wr_arr/res_arr
    // free2ddouble(&ro_arr);
    // free2ddouble(&wr_arr);
    // free2ddouble(&res_arr);

    free(wr_arr);
    free(ro_arr);
    free(res_arr);

    MPI_Finalize();
    return 0;
}