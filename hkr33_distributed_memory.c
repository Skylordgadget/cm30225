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

struct thread_rows {
    int start_row;
    int end_row;
};

struct thread_data {
    double precision;
    int thread_lim;
    int size;
};

void remove_spaces(char *str) {
    int count = 0;
    /* traverse str If the current character is not a space,
    move it to index count++ */
    for (int i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i];
    str[count] = '\0';
}

double** malloc_double_array(int size) {
    // malloc for the pointers to each row
    double** arr = (double**)malloc(size*sizeof(double*));
    // malloc for each element in each row  
    for (int i=0; i<size; i++) 
        arr[i] = (double*)malloc(size*sizeof(double));

    return arr;
}

void free_double_array(double** arr, int size) {
    // free memory for each element in each row
    for (int i=0; i<size; i++) free(arr[i]);
    // free memory for the pointers to each row
    free(arr);
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

MPI_Datatype create_trows_type(){
    struct thread_rows trows;
    MPI_Datatype trows_type;
    MPI_Datatype type[2] = {MPI_INT, MPI_INT};
    int blocklen[2] = {1, 1};
    MPI_Aint disp[2];
    MPI_Aint base;

    MPI_Get_address(&trows, &base);
    MPI_Get_address(&trows.start_row, &disp[0]);
    MPI_Get_address(&trows.end_row, &disp[1]);
    disp[0] = MPI_Aint_diff(disp[0], base);
    disp[1] = MPI_Aint_diff(disp[1], base);

    MPI_Type_create_struct(2, blocklen, disp, type, &trows_type);
    MPI_Type_commit(&trows_type);
    return trows_type;
}

MPI_Datatype create_tdata_type(){
    struct thread_data tdata;
    MPI_Datatype tdata_type;
    MPI_Datatype type[3] = {MPI_DOUBLE, MPI_INT, MPI_INT};
    int blocklen[3] = {1, 1, 1};
    MPI_Aint disp[3];
    MPI_Aint base;

    MPI_Get_address(&tdata, &base);
    MPI_Get_address(&tdata.precision, &disp[0]);
    MPI_Get_address(&tdata.thread_lim, &disp[1]);
    MPI_Get_address(&tdata.size, &disp[2]);
    disp[0] = MPI_Aint_diff(disp[0], base);
    disp[1] = MPI_Aint_diff(disp[1], base);
    disp[2] = MPI_Aint_diff(disp[2], base);

    MPI_Type_create_struct(3, blocklen, disp, type, &tdata_type);
    MPI_Type_commit(&tdata_type);
    return tdata_type;
}

int main(int argc, char** argv){
    int process_Rank, size_Of_Cluster;
    struct thread_rows trows;
    struct thread_data tdata;

    MPI_Init(&argc, &argv);

    MPI_Datatype trows_type = create_trows_type();
    MPI_Datatype tdata_type = create_tdata_type();


    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);
    #ifdef DEBUG
        printf("%d\n", process_Rank);
    #endif


    if (process_Rank == ROOT) {
        // FILE*       fpt;
        // command line arguments
        int         num_threads = -1;
        int         thread_lim;

        int         size = -1;   
        int         size_mutable;

        // double      precision_arg = -1.0;
        double      precision = -1.0;

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
                    size = atoi(optarg);
                    break; 
                case 't': 
                    num_threads = atoi(optarg); 
                    break; 
                case 'p': 
                    precision = atof(optarg);
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
        
        // snprintf(out_path, sizeof(out_path), \
        //             ".%sres_%dt_%ds_%fp.txt", \
        //                 SLASH, num_threads, size, precision);

        size_mutable = size - 2; // array borders are not mutable
        /* if there are more threads than rows in the array,
        set thread_lim = size_mutable, otherwise thread_lim = num_threads */  
        thread_lim = (size_mutable < num_threads) ? size_mutable : num_threads-1;
        
        tdata.precision = precision;
        tdata.thread_lim = thread_lim;
        tdata.size = size;

        /* if thread_lim is not divisible by the mutable array size,
        threads_l = number of threads that need to do more work */
        int threads_l = size_mutable % thread_lim;

        int rows_per_thread_s = size_mutable / thread_lim;
        int rows_per_thread_l = rows_per_thread_s + 1;
        
        double** old_arr = malloc_double_array(size);
        double** new_arr = malloc_double_array(size);
        
        old_arr = debug_populate_array(old_arr, size, mode);
        new_arr = copy_array(old_arr, new_arr, size);
        
        if (print) debug_display_array(old_arr, size);

        trows.start_row = 0;
        trows.end_row = 0;
        
        //clock_gettime(CLOCK_REALTIME, &timer_start); // start timing code here
        for (int i=0; i<thread_lim; i++) {
            // evenly distribute work among available threads
            if (threads_l > 0) {
                trows.start_row = (i>0) ? trows.end_row+1 : 1;
                trows.end_row = trows.start_row + rows_per_thread_l - 1;
                threads_l--;
            } else {
                trows.start_row = (i>0) ? trows.end_row+1 : 1;
                trows.end_row = trows.start_row + rows_per_thread_s - 1;
            }

            if (verbose) {
                printf("thread %d, start row: %d | end row %d\n", i+1, trows.start_row, trows.end_row);
            }
            MPI_Send(&trows, 1, trows_type, i+1, 99, MPI_COMM_WORLD);
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
    //}
    } else {
        MPI_Status stat;
        MPI_Recv(&trows, 1, trows_type, ROOT, 99, MPI_COMM_WORLD, &stat);
        printf("I am thread: %d, my start row is %d, my end row is %d\n", process_Rank, trows.start_row, trows.end_row);
    }
    MPI_Bcast(&tdata, 1, tdata_type, ROOT, MPI_COMM_WORLD);
    printf("I am thread: %d | p: %f | t: %d | s: %d\n", process_Rank, tdata.precision, tdata.thread_lim, tdata.size);
    MPI_Finalize();
    return 0;
}