#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

//#define _POSIX_C_SOURCE 199309L

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif 

// global
int THREAD_LIMIT = 4;  
int MAT_SIZE=100;
double PRECISION=0.0000001;
int DEBUG=0;
int PRINT=0;

struct timespec begin_time, end_time;
double elapsed;

pthread_barrier_t my_barrier;
pthread_cond_t all_threads_finished;
int finished_threads=0;

typedef struct a{
    int thread_id;
    double** array;
    double** t_array;
    int start_row;
    int end_row;
    pthread_mutex_t* lock;
    double difference;
    int mat_size;
} threadArgs;

double** initArray(int row_size, int col_size) {
    double** array_name = (double **)malloc(row_size * sizeof(double*));
    if (array_name == NULL) {
        exit(0);
    }
    for (int i = 0; i < row_size; i++) {
        array_name[i] = (double *)malloc(col_size * sizeof(double));
        if (array_name[i] == NULL) {
            exit(0);
        }
    }
    return array_name;
}

void freeArray(double** array_name, int row_size){
    int i;
    for (i=0; i<row_size; i++){
        free(array_name[i]);
    }
    free(array_name);
}

void printMatrix(double** array, int size){
    printf("\n");
    int i,j;
	for (i=0; i<size; i++){
		for(j=0; j<size; j++){
            printf("%.12f ",array[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void* average(void* args){
    int precisionmet=0;
    int thread_done=0;
    threadArgs* tArgs = (threadArgs*) args;
    // averaging values
    while(1){
        tArgs->difference = 0.0;
        int i,j; 
        if(!thread_done){
            for (i=tArgs->start_row; i<tArgs->end_row+1; i++){
                for (j=1; j<tArgs->mat_size+1; j++){
                    if (DEBUG) { printf("thread: %d, row: %d, col %d\n", tArgs->thread_id, i, j); }
                    tArgs->t_array[i][j]=\
                                        (tArgs->array[i-1][j]+tArgs->array[i+1][j]
                                        +tArgs->array[i][j+1]+tArgs->array[i][j-1])/4;
                }
            }
        }

        pthread_barrier_wait(&my_barrier);

        // copying temp array values to shared array
        if(!thread_done){
            for (i=tArgs->start_row; i<tArgs->end_row+1; i++){
                for (j=1; j<tArgs->mat_size+1; j++){
                    tArgs->difference += fabs(tArgs->array[i][j] - tArgs->t_array[i][j]);   // t_array is the next iteration of the array. current - next = difference. 
                    tArgs->array[i][j]=tArgs->t_array[i][j]; // updating the array
                }
            }
        }


        // when done, set precisionmet = 1
        if(!thread_done){
            if (tArgs->difference <= PRECISION) {
                thread_done=1;
                pthread_mutex_lock(tArgs->lock);
                finished_threads++;
                pthread_mutex_unlock(tArgs->lock);
            }
        }
        // Wait for all threads to be finished before reiterating
        pthread_barrier_wait (&my_barrier);
        if (finished_threads==THREAD_LIMIT) break;
    }
    if (DEBUG) { printf("Thread %d has exited the while loop\n",tArgs->thread_id); }
    if (DEBUG) { printf("Finished Threads: %d\n",finished_threads); }
    pthread_exit(NULL);
}

int main(int argc, char** argv){

    // int option;
    // while ((option = getopt(argc, argv, "n:s:p:do")) != -1) {
    //     switch (option) {
    //         case 'n':
    //             THREAD_LIMIT = atoi(optarg); // Convert optarg to an integer
    //             break;
    //         case 's':
    //             MAT_SIZE = atoi(optarg); // Convert optarg to an integer
    //             break;
    //         case 'p':
    //             PRECISION = atof(optarg); // Convert optarg to a double
    //             break;
    //         case 'd':
    //             DEBUG=1;
    //             break;
    //         case 'o':
    //             PRINT=1;
    //             break;
    //         case '?':
    //             puts("Unknown option or missing argument");
    //     }
    // }

    int SIZE=MAT_SIZE-2;    // work on inner array   
    // ensures number of threads < array size
    //THREAD_LIMIT = 4;
    THREAD_LIMIT = ( THREAD_LIMIT < SIZE ? THREAD_LIMIT : SIZE );

    clock_gettime(CLOCK_MONOTONIC, &begin_time);

    // spawn threads to do work here
    pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&all_threads_finished, NULL);
    pthread_barrier_init(&my_barrier, NULL, THREAD_LIMIT);

    double** num_array = NULL;
    double** t_array = NULL;
    num_array = initArray(MAT_SIZE, MAT_SIZE);
    t_array = initArray(MAT_SIZE, MAT_SIZE);

    // fill boundary with random values
    int i,j;
    for (i=0; i<MAT_SIZE; i++){
        for (j=0; j<MAT_SIZE; j++){
            if (i == 0 || i == MAT_SIZE-1 || j == 0 || j == MAT_SIZE-1){ num_array[i][j] = rand()%10; t_array[i][j] = num_array[i][j]; }
        }
    }

    // initialise the structs and threads 
    threadArgs tArgs[THREAD_LIMIT];
    pthread_t threads[THREAD_LIMIT];

    if (PRINT) { printMatrix(num_array, MAT_SIZE); }

    // number of threads which can do the larger work size
    int max_work_thread_num = SIZE % THREAD_LIMIT;
    double rows_per_thread = (double) SIZE / THREAD_LIMIT;
    int max_rows_per_thread = ceil(rows_per_thread);
    int min_rows_per_thread = floor(rows_per_thread);
    
    int start=0;
    int end=0;
    for (i=0; i<THREAD_LIMIT; i++){
        // if the number of threads divides evenly into the array size OR there are no more threads we can give the max work size
        if (max_work_thread_num == 0){
            // if this is the very first thread, start at 0. Else, start after the last thread
            if (i == 0){ start = 1; } else { start = end+1; }
            end = start+min_rows_per_thread-1; 
        } else {
            if (i == 0){ start = 1; } else { start = end+1; }
            end = start+max_rows_per_thread-1; 
            max_work_thread_num--;  // will eventually hit zero, so we will not go into this else statement again.
        }
        tArgs[i].thread_id = i;
        tArgs[i].array=num_array;
        tArgs[i].t_array=t_array;
        tArgs[i].start_row=start;
        tArgs[i].end_row=end;
        tArgs[i].lock=&thread_lock;        
        tArgs[i].difference=0.0;
        tArgs[i].mat_size=SIZE;
        int rc = pthread_create(&threads[i], NULL, average, (void*) &tArgs[i]);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
}

    // wait for all threads to finish their work
    for (i=0; i<THREAD_LIMIT; i++){
        pthread_join(threads[i], NULL);
    }

    if (PRINT) { printMatrix(num_array, MAT_SIZE); }

    // all threads are finished, destroy stuff
    pthread_mutex_destroy(&thread_lock);
    pthread_cond_destroy(&all_threads_finished);
    freeArray(num_array, SIZE);
    freeArray(t_array, SIZE);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed = end_time.tv_sec - begin_time.tv_sec;
    elapsed += (end_time.tv_nsec - begin_time.tv_nsec) / 1000000000.0;
    printf("%f\n",elapsed);
    return 0;
}