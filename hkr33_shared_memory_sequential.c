#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <hkr33_shared_memory.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

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

void avg(double** ro_arr, double** wr_arr, uint16_t size_mutable) {
    bool precision_met = false;
    bool thread_done = false;

    double** tmp_arr;

    while (true) {

        precision_met = true;
        for (uint16_t i=1; i<=size_mutable; i++) {
            for (uint16_t j=1; j<=size_mutable; j++) {
                wr_arr[i][j] = \
                    (double)((ro_arr[i][j-1] + ro_arr[i][j+1] + \
                                ro_arr[i-1][j] + ro_arr[i+1][j])/4);
                precision_met &= \
                    fabs(ro_arr[i][j] - wr_arr[i][j]) <= precision;
            }
        }
        
        tmp_arr = ro_arr;
        ro_arr = wr_arr;
        wr_arr = tmp_arr;
        G_utd_arr = ro_arr;

        if (precision_met) break;
    }
    pthread_exit(NULL);
    return 0;
}


int main (int argc, char **argv) {
    const char  mode = '1';
    FILE*       fpt;

    // command line arguments
    int         size_arg = -1;
    uint16_t    size;
    uint16_t    size_mutable;

    double      precision_arg = -1.0;
    double      precision;

    bool        verbose = false;
    bool        print = false;
    bool        csv = false;
    
    int opt;
    

    while((opt = getopt(argc, argv, ":cvap:s:")) != -1) 
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

    // size and precision are required
    // if -1 is not overwritten, the program will exit
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

 
    size_mutable = size - 2; // array borders are not mutable
    // if there are more threads than rows in the array,
    // set thread_lim = size_mutable, otherwise thread_lim = num_threads   

    double** old_arr = malloc_double_array(size);
    double** new_arr = malloc_double_array(size);
    
    old_arr = debug_populate_array(old_arr, size, mode);
    new_arr = copy_array(old_arr, new_arr, size);
    
    if (print) debug_display_array(old_arr, size);

    avg()

    if (print) debug_display_array(G_utd_arr, size);

    if (csv) {
        fpt = fopen(".\\bin\\hkr33_result.csv", "w");
        if (fpt == NULL) perror("fopen");
        write_csv(G_utd_arr, size, fpt);
        fclose(fpt);
    }

    
    free_double_array(old_arr, size);
    free_double_array(new_arr, size);
    return 0;
}

