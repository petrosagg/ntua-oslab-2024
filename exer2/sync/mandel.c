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
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

#if defined(SYNC_CONDVAR) ^ defined(SYNC_SEMAPHORE) == 0
# error You must #define exactly one of SYNC_CONDVAR or SYNC_SEMAPHORE.
#endif

#if defined(SYNC_CONDVAR)
# define USE_CONDVARS 1
#else
# define USE_CONDVARS 0
#endif

#define perror_pthread(ret, msg) \
	do { errno = ret; perror(msg); } while (0)

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
	fprintf(stderr, "Usage: %s thread_count\n\n"
		"Exactly one argument required:\n"
		"    thread_count: The number of threads to create.\n",
		argv0);
	exit(1);
}

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

// Whether we should termimnate early due to a ctrl+c
int terminate_early = 0;

void signal_handler(int signum) {
	// Use an atomic operation to set the flag since we're in a multi-threaded
	// context.
	__sync_fetch_and_or(&terminate_early, 1);
}

// An array of semaphores where semaphores[i] contains the semaphore
// corresponding to the i-th thread
sem_t *semaphores;
pthread_mutex_t lock;
pthread_cond_t cond;
int cur_line = 0;

// A (distinct) instance of this structure is passed to each thread
struct thread_info_struct {
	// The POSIX thread id, as returned by the library
	pthread_t tid;
	// The thread id
	int thrid;
	// The total number of threads
	int thrcnt;
};

void *thread_start_fn(void *arg) {
	struct thread_info_struct *thr = arg;

	// A temporary array, used to hold color values for the line being drawn
	int color_val[x_chars];

	for (int line = thr->thrid; line < y_chars; line += thr->thrcnt) {
		// If the user asked for termination exit early
		if (terminate_early) {
			break;
		}

		// Computing the line doesn't need to synchronize with other threads since
		// it operates on a local array.
		compute_mandel_line(line, color_val);

		if (USE_CONDVARS) {
			pthread_mutex_lock(&lock);
			// Wait until the current line being drawn is the line we are attempting to draw
			while (cur_line < line) {
				pthread_cond_wait(&cond, &lock);
			}
		} else {
			// Wait for this thread's semaphore
			sem_wait(&semaphores[thr->thrid]);
		}

		// We are in the critical section so it is our turn to draw a line.
		output_mandel_line(1, color_val);

		if (USE_CONDVARS) {
			// Advance the current line and broadcast the condition variable so that
			// all other threads re-check it.
			cur_line += 1;
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&lock);
		} else {
			// Post next thread's semaphore
			sem_post(&semaphores[(thr->thrid + 1) % thr->thrcnt]);
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	int thrcnt;
	struct thread_info_struct *thr;

	if (argc != 2)
		usage(argv[0]);
	if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0) {
		fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
		exit(1);
	}

	// Install signal handler to gracefully handle CTRL+C
	struct sigaction sa;
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}

	thr = calloc(thrcnt, sizeof(*thr));

	// If we're using semaphores we need to allocate as many as there are threads
	if (!USE_CONDVARS) {
		semaphores = calloc(thrcnt, sizeof(*semaphores));
		for (int i = 0; i < thrcnt; i++) {
			if (sem_init(&semaphores[i], 0, 0) < 0) {
				perror("sem_init");
				exit(1);
			}
		}
		// Post the first thread so that the process starts from line 0
		sem_post(&semaphores[0]);
	}

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	// Create threads
	for (int i = 0; i < thrcnt; i++) {
		thr[i].thrid = i;
		thr[i].thrcnt = thrcnt;

		int ret = pthread_create(&thr[i].tid, NULL, thread_start_fn, &thr[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}
	}

	// Wait for all threads to terminate
	for (int i = 0; i < thrcnt; i++) {
		int ret = pthread_join(thr[i].tid, NULL);
		if (ret) {
			perror_pthread(ret, "pthread_join");
			exit(1);
		}
	}

	reset_xterm_color(1);

	// If this is not a normal shutdown we return a non-zero code
	return terminate_early;
}
