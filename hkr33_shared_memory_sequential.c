#include <stdio.h>
#include <stdlib.h>
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


double** copy_array(double** arr_a, double** arr_b, uint16_t size) {
    for (uint16_t i=0; i<size; i++)  
        for (uint16_t j=0; j<size; j++) arr_b[i][j] = arr_a[i][j];
    return arr_b;
}

void avg(double** old_arr, double** new_arr, uint16_t size_mutable, \
            double precision) {
    #ifdef DEBUG
        int counter = 0;
    #endif

    bool precision_met = false;

    double** ro_arr = old_arr;
    double** wr_arr = new_arr;
    double** tmp_arr;

    while (true) {
        #ifdef DEBUG
            counter++;
        #endif

        precision_met = true;
        // loop over rows
        for (uint16_t i=1; i<=size_mutable; i++) {
            // loop over columns
            for (uint16_t j=1; j<=size_mutable; j++) {
                // add up surrounding four value and divide by four
                wr_arr[i][j] = (ro_arr[i][j-1] + ro_arr[i][j+1] + \
                                ro_arr[i-1][j] + ro_arr[i+1][j])/4.0;
                precision_met &= \
                    fabs(ro_arr[i][j] - wr_arr[i][j]) < precision;
            }
        }
        

        // flip pointers
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        G_utd_arr = ro_arr; 

        if (precision_met) break;
    }

    #ifdef DEBUG
        printf("counter: %d\n", counter);
    #endif
}

/* /////////////////////////////////////////////////////////////////////////////
   //                                                                         //
   // main                                                                    //
   //                                                                         //
*/ /////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv) {
    FILE*       fpt;
    // command line arguments
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
        printf("extra arguments: %s\n", argv[optind]); 
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

    snprintf(out_path, sizeof(out_path), \
                "..%sout%sres_seq_%ds_%fp.txt", \
                    SLASH, SLASH, size, precision);

    printf("mode %c\n", mode);

    size_mutable = size - 2; // array borders are not mutable

    double** old_arr = malloc_double_array(size);
    double** new_arr = malloc_double_array(size);
    
    old_arr = debug_populate_array(old_arr, size, mode);
    new_arr = copy_array(old_arr, new_arr, size);
    
    if (print) debug_display_array(old_arr, size);


    clock_gettime(CLOCK_REALTIME, &timer_start); // start timing code here
    avg(old_arr, new_arr, size_mutable, precision);
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
            fprintf(fpt, "Sequential, %d, %f, %f", size, time, precision);
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