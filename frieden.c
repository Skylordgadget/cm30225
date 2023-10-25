#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

//#define _POSIX_C_SOURCE 199309L

#define THREAD_LIMIT 6
#define ROW 1000
#define COL 1000
#define PRECISION 1 
#define DEBUG 1
#define PRINT 0

struct timespec begin, end;
double elapsed;

//pthread_barrier_t my_barrier;
pthread_cond_t all_threads_finished;
int count = 0;
int finished_threads=0;

typedef struct a{
    int thread_id;
    double** array;
    double** t_array;
    int start_row;
    int end_row;
    int finished;
    pthread_mutex_t* lock;
    double difference;
    int loop;
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

void printMatrix(double** array){
    printf("\n");
    int i,j;
	for (i=0; i<ROW; i++){
		for(j=0; j<COL; j++){
            printf("%.12f ",array[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void* average(void* args){
    threadArgs* tArgs = (threadArgs*) args;

    while (1) {
        //printf("Start of the loop for thread %d\n",tArgs->thread_id);
        tArgs->difference = 0.0;

        int i,j; 
        for (i=tArgs->start_row; i<tArgs->end_row; i++){
            for (j=0; j<COL; j++){
                if (DEBUG) { printf("thread: %d, row: %d, col %d\n", tArgs->thread_id, i, j); }
                if (i != 0 && i < ROW-1 && j != 0 && j < COL-1){
                    tArgs->t_array[i][j]=(tArgs->array[i-1][j]+tArgs->array[i+1][j]+tArgs->array[i][j+1]+tArgs->array[i][j-1])/4;
                }
            }
        }

        // Wait for all threads to be finished before reiterating
        pthread_mutex_lock(tArgs->lock);
        count++;
        if (DEBUG) { printf("Count: %d\n",count); }
        //printf("difference: %f. Thread %d\n", tArgs->difference, tArgs->thread_id);
        if (count < THREAD_LIMIT - finished_threads) {
            if (DEBUG) { printf("Thread %d is waiting for the first condition.\n",tArgs->thread_id); }
            pthread_cond_wait(&all_threads_finished, tArgs->lock);
        } else {
            if (DEBUG) { printf("Thread %d is broadcasting to the other threads on the first condition.\n",tArgs->thread_id); }
            pthread_cond_broadcast(&all_threads_finished);
            count = 0;  // resetting count, so that the other threads can wait again.
        }
        pthread_mutex_unlock(tArgs->lock);

        for (i=tArgs->start_row; i<tArgs->end_row; i++){
            for (j=0; j<COL; j++){
                if (i != 0 && i < ROW-1 && j != 0 && j < COL-1){
                    // t_array is the next iteration of the array. current - next = difference. 
                    tArgs->difference += fabs(tArgs->array[i][j] - tArgs->t_array[i][j]);
                    tArgs->array[i][j]=tArgs->t_array[i][j]; // updating the array
                }
            }
        }

        // change difference???? 

        // Wait for all threads to be finished before reiterating
        //if (DEBUG) { printf("Thread %d has taken the lock.\n",tArgs->thread_id); }
        pthread_mutex_lock(tArgs->lock);
        count++;
        if (DEBUG) { printf("Count: %d\n",count); }
        //printf("difference: %f. Thread %d\n", tArgs->difference, tArgs->thread_id);
        if (count < THREAD_LIMIT - finished_threads) {
            if (DEBUG) { printf("Thread %d is waiting for the second condition.\n",tArgs->thread_id); }
            pthread_cond_wait(&all_threads_finished, tArgs->lock);
        } else {
            count = 0;  // resetting count, so that the other threads can wait again.
            // if the difference between all items (?) is less than the precision, we re-iterate again
            if (tArgs->difference <= PRECISION) {
                if (DEBUG) { printf("Thread %d is broadcasting and exiting.\n",tArgs->thread_id); }
                finished_threads++;
                pthread_cond_broadcast(&all_threads_finished);
                // i think i've spent like 3 weeks trying to figure out why my code was hanging. i wasnt releasing the damn lock this whole time.
                // or maybe something else in addition to this...
                pthread_mutex_unlock(tArgs->lock);  
                break;
            } else {
                if (DEBUG) { printf("Thread %d is broadcasting to the other threads on the second condition.\n",tArgs->thread_id); }
                int rc = pthread_cond_broadcast(&all_threads_finished);
                if (rc) { printf("error"); }
            }
        }
        pthread_mutex_unlock(tArgs->lock);
        if (DEBUG) { tArgs->loop++; }
    }

    if (DEBUG) { printf("Thread %d has exited the while loop\n",tArgs->thread_id); }
    if (DEBUG) { printf("Finished Threads: %d\n",finished_threads); }


    pthread_exit(NULL);
}

int main(int argc, char** argv){

    clock_gettime(CLOCK_MONOTONIC, &begin);

    // spawn threads to do work here
    pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&all_threads_finished, NULL);
    //pthread_barrier_init(&my_barrier, NULL, THREAD_LIMIT);

    double** num_array = NULL;
    double** t_array = NULL;
    num_array = initArray(ROW, COL);
    t_array = initArray(ROW, COL);

    // fill boundary with random values
    int i,j;
    for (i=0; i<ROW; i++){
        for (j=0; j<COL; j++){
            if (i == 0 || i == ROW-1 || j == 0 || j == COL-1){ num_array[i][j] = rand()%10; t_array[i][j] = num_array[i][j]; }
        }
    }

    // initialise the structs and threads 
    threadArgs tArgs[THREAD_LIMIT];
    pthread_t threads[THREAD_LIMIT];

    if (PRINT) { printMatrix(num_array); }

    // creating the threads
    int chunk = ceil((double) ROW / THREAD_LIMIT);
    for (i=0; i<THREAD_LIMIT; i++){
        tArgs[i].thread_id = i;
        tArgs[i].array=num_array;
        tArgs[i].t_array=t_array;
        tArgs[i].lock=&thread_lock;        
        tArgs[i].start_row=i*chunk;
        if (i == THREAD_LIMIT-1) {
            tArgs[i].end_row = ROW;
        }else{
            tArgs[i].end_row=tArgs[i].start_row+chunk;
        }
        tArgs[i].finished=0;
        tArgs[i].loop=0;
        tArgs[i].difference=0.0;
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

    if (PRINT) { printMatrix(num_array); }

    printf("Finished :)\n");

    // all threads are finished, destroy stuff
    pthread_mutex_destroy(&thread_lock);
    pthread_cond_destroy(&all_threads_finished);

    freeArray(num_array, ROW);
    freeArray(t_array, ROW);
    

    clock_gettime(CLOCK_MONOTONIC, &end);
   

    elapsed = end.tv_sec - begin.tv_sec;
    elapsed += (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
    printf("Seconds %f\n", elapsed);
    return 0;
}