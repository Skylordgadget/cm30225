mpicc hkr33_distributed_memory.c -O3 -Wall -Wextra -Wconversion -o hkr33_dist_mem

NOTE THAT, -O3 works for me--if something breaks I suggest trying -O2

Required Arguments:
	-t <number of threads>
	-s <width of matrix>
	-p <precision>
Optional Arguments:
	-v verbose - prints additional information while the program runs
	-a prints the start and end matrices
	-o outputs a text document containing the number of threads used, matrix size, completion time and precision into the same directory as the code
	-f <filename> outputs the resultant relaxed matrix to the file path specified
	-m <1, r, q, m, a, b u or g> dataset mode: 
		1: 1s along the top and left, 0s everywhere else
		r: random numbers between 0 and 9 on the borders
		q: random numbers between 0 and 9 for every element
        m: on the borders, each element = x_idx*y_idx else 0s
        a: on the borders, each element = x_idx+y_idx else 0s
        b: 1s on the borders, 0s in the middle
		g: 1s on borders, else random number between 0 and 9