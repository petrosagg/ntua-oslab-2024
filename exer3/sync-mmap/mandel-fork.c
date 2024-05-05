/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

/*TODO header file for m(un)map*/

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

#if defined(SYNC_SEMAPHORE) ^ defined(SYNC_SHAREDBUF) == 0
# error You must #define exactly one of SYNC_SHAREDBUF or SYNC_SEMAPHORE.
#endif

#if defined(SYNC_SEMAPHORE)
# define USE_SEMAPHORES 1
#else
# define USE_SEMAPHORES 0
#endif

// An array of semaphores where semaphores[i] contains the semaphore
// corresponding to the i-th process
sem_t *semaphores;
// An array of colors used when we don't use semaphores for synchronization
int* color_buf;

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void compute_and_output_mandel_line(int fd, int line)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];

	compute_mandel_line(line, color_val);
	output_mandel_line(fd, color_val);
}

/*
 * Create a shared memory area, usable by all descendants of the calling
 * process.
 */
void *create_shared_memory_area(unsigned int numbytes)
{
	int pages;
	void *addr;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	/* Create a shared, anonymous mapping for this number of pages */
	addr = mmap(NULL, pages * sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	return addr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
	int pages;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("destroy_shared_memory_area: munmap failed");
		exit(1);
	}
}

int safe_atoi(char *s, int *val)
{
	long l;
	char *endp;

	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		*val = l;
		return 0;
	} else
		return -1;
}

void usage(char *argv0)
{
	fprintf(stderr, "Usage: %s process_count\n\n"
		"Exactly one argument required:\n"
		"    process_count: The number of processes to create.\n",
		argv0);
	exit(1);
}

void worker(int proc_id, int nprocs) {
	// A temporary array, used to hold color values for the line being drawn
	int color_val[x_chars];

	for (int line = proc_id; line < y_chars; line += nprocs) {
		if (USE_SEMAPHORES) {
			compute_mandel_line(line, color_val);
			// Wait for this process' semaphore
			sem_wait(&semaphores[proc_id]);
			// We are in the critical section so it is our turn to draw a line.
			output_mandel_line(1, color_val);
			// Post next process' semaphore
			sem_post(&semaphores[(proc_id + 1) % nprocs]);
		} else {
			compute_mandel_line(line, &color_buf[line * x_chars]);
		}
	}
}

int main(int argc, char* argv[])
{
	int nprocs;

	if (argc != 2)
		usage(argv[0]);
	if (safe_atoi(argv[1], &nprocs) < 0 || nprocs <= 0) {
		fprintf(stderr, "`%s' is not valid for `process_count'\n", argv[1]);
		exit(1);
	}

	// If we're using semaphores we need to allocate as many as there are processes
	if (USE_SEMAPHORES) {
		semaphores = create_shared_memory_area(nprocs * sizeof(*semaphores));
		for (int i = 0; i < nprocs; i++) {
			if (sem_init(&semaphores[i], 1, 0) < 0) {
				perror("sem_init");
				exit(1);
			}
		}
		// Post the first process so that the process starts from line 0
		sem_post(&semaphores[0]);
	} else {
		color_buf = create_shared_memory_area(x_chars * y_chars * sizeof(int));
	}

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	// Create processes
	for (int i = 0; i < nprocs; i++) {
		int p = fork();
		if (p < 0) {
			perror("fork");
			exit(1);
		}
		if (p == 0) {
			worker(i, nprocs);
			return 0;
		}
	}

	// Wait for all processes to terminate
	for (int i = 0; i < nprocs; i++) {
		wait(NULL);
	}

	if (!USE_SEMAPHORES) {
		for (int line = 0; line < y_chars; line += 1) {
			output_mandel_line(1, &color_buf[line * x_chars]);
		}
	}

	reset_xterm_color(1);
	return 0;
}
