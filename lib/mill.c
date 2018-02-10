/* experimental new mill.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
/////////////////////////////
#include <pthread.h>
#ifndef get_nprocs
int get_nprocs(void) {
	return 4;
}
#endif
// #include <sys/sysinfo.h>
/////////////////////////////

/* prime table */
unsigned *prim = NULL;
int dim = 0;
/* highest number to test */
unsigned testnum = 0;
unsigned last_testnum = 1000;
int malloc_dim = 1000;

/* multithread */
int npthread = 1;
pthread_t *pthreads = NULL;
int mult = 1;
unsigned *testnumbers = NULL;
unsigned long long begin_task = 0;
int task_ready = 0;
pthread_mutex_t in_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t out_mutex = PTHREAD_MUTEX_INITIALIZER;

/* save and load */
const char *fname = "prim32.dat";

int is_prime_thread(unsigned long long n, int currentdim)
{
	int i=0, answer_is_prime=1;
	unsigned long long q;

	q = (unsigned long long) sqrtl( (long double) n );

	for (i = 1; i < currentdim && (unsigned long long) prim[i] <= q; i++) {
		if (n % ((unsigned long long) prim[i]) == 0) {
			answer_is_prime = 0; /* composite */
			break;
		}
	}

	return answer_is_prime;
}

void * fill_prim_table_thread (void * dummy)
{
	int i, j;
	int myindex = 0;
	unsigned long long mymask = 1;
	int end = 0;

	for(i=0; i < npthread; i++) {
		mymask <<= 1;
		if (pthreads[i] == pthread_self()) {
			myindex = i;
			break;
		}
	}

	while ( !end ) {
		// wait
		pthread_mutex_lock(&in_mutex);
		while((begin_task & mymask) != mymask)
			pthread_cond_wait(&start_cond, &in_mutex);
		begin_task &= ~mymask;
		pthread_mutex_unlock(&in_mutex);

		// tasks
		for(i=0; i < mult; i++) {
			if (testnumbers[mult*myindex+i] == 0) {
				end = 1;
			}
			if (!is_prime_thread(testnumbers[mult*myindex+i], dim)) {
				testnumbers[mult*myindex+i] = 0; /* composite */
			}
		}

		pthread_mutex_lock(&out_mutex);
		task_ready++;
		if (task_ready == npthread) {
			// store
			for (i = 0; i < npthread; i++) {
				for (j = 0; j < mult; j++) {
					if (testnumbers[mult*i+j] > 0) {
						if (dim == malloc_dim) {
							printf("dim overflow\n");
							exit(EXIT_FAILURE);
						}
						prim[dim++] = testnumbers[mult*i+j];
					}
				}
			}

			// reinit
			for (i = 0; i < npthread; i++) {
				for (j = 0; j < mult; j++) {
					if (testnum <= last_testnum -2) {
						testnumbers[mult*i+j] = testnum;
						testnum += 2;
					} else {
						testnumbers[mult*i+j] = 0;
					}
				}
			}
			task_ready = 0;

			// allow next start
			pthread_mutex_lock(&in_mutex);
			begin_task = ~0;
			pthread_cond_broadcast(&start_cond);
			pthread_mutex_unlock(&in_mutex);
		}
		pthread_mutex_unlock(&out_mutex);
	}

	return dummy;
}

int saveit(void)
{
	size_t wr_size=0, wr_ctrl=0;
	int fd=0;

	fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	wr_size = (size_t) dim * sizeof(unsigned);
	wr_ctrl = write(fd, (void *)&prim[0], wr_size);
	if (wr_ctrl != wr_size) {
		perror("write");
		close(fd);
		return 2;
	}

	if (close(fd)) {
		perror("close");
		return 3;
	}

	return 0;
}

int fill_prim_table_main(void)
{
	int i, j;

	prim = (unsigned *) malloc(malloc_dim*sizeof(unsigned));
	if (prim == NULL) {
		perror("malloc prim");
		return (-1);
	}
	pthreads = (pthread_t *) malloc(npthread*sizeof(pthread_t));
	if (pthreads == NULL) {
		perror("malloc pthreads");
		return (-1);
	}
	testnumbers = (unsigned *) malloc(mult*npthread*sizeof(unsigned));
	if (testnumbers == NULL) {
		perror("malloc testnumbers");
		return (-1);
	}

	// init
	for (i = 0; i < malloc_dim; i++)
		prim[i] = 0;
	prim[0] = 2;
	prim[1] = 3;
	prim[2] = 5;
	prim[3] = 7;
	dim = 4;
	testnum = prim[dim-1] +2;

	// init, one testnum in each loop
	while(testnum < 1000 || dim < 2*mult*npthread) {
		if (is_prime_thread(testnum, dim)) {
			prim[dim++] = testnum;
		}
		testnum += 2;
	}
	printf("init: dim %d, last prime %u, testnum %u\n", dim, prim[dim-1], testnum);

	if (npthread > 1) {
		printf("config: mult=%d npthread=%d\n", mult, npthread);

		// init testnumbers
		for (i = 0; i < npthread; i++) {
			for (j = 0; j < mult; j++) {
				testnumbers[mult*i+j] = testnum;
				testnum += 2;
			}
		}

		// threads, mult*npthread testnumbers in each loop
		for(i=0; i < npthread; i++) {
			if (pthread_create(&(pthreads[i]), NULL, fill_prim_table_thread, NULL)) {
				perror("pthread_create");
				exit(EXIT_FAILURE);
			}
		}
		begin_task = ~0;
		pthread_cond_broadcast(&start_cond);

		// join threads
		for(i=0; i < npthread; i++) {
			pthread_join(pthreads[i], NULL);
		}

	} else {
		printf("config: fallback\n");
		fill_prim_table_thread (NULL);
	}

	// save the result
	printf("saving %d items... ", dim);
	if (saveit()) {
		printf("failed\n");
		return 1;
	}
	printf("ok\n");

	return 0;
}

int main(int argc, const char *argv[])
{
	long double x = 1.0;
	int ret = 0;

	npthread = get_nprocs();
	mult = 256;
	last_testnum = UINT_MAX;
	x = last_testnum;
	//malloc_dim = (int) (1.07 * x / logl(x));
	malloc_dim = (int) (x / (logl(x) - 1.5));
	malloc_dim = ((malloc_dim/100)+1)*100;

	if (argc == 1) {
		printf("usage: %s <mult> [<npthread>]\n", argv[0]);
		printf("  mult=%d npthread=%d\n", mult, npthread);
		return 0;
	}

	if (argc > 1) {
		mult = atoi(argv[1]);
		if (mult < 1) mult = 1;
		if (argc > 2) {
			npthread = atoi(argv[2]);
			if (npthread < 1) npthread = 1;
		}
	}
	// begin_task has 64bits
	if (npthread > 62) npthread = 62;

	printf("last %u, malloc %d, mult %d, npthread %d, run ...\n",
		last_testnum, malloc_dim, mult, npthread);

	if (argc > 3) return 0;

	ret = fill_prim_table_main();
	if (ret == 0)
		printf("primes from 2 to %u, dim %d\n", prim[dim-1], dim);
	else
		printf("failed, last prime %u, dim %d\n", prim[dim-1], dim);

	free(testnumbers);
	free(pthreads);
	free(prim);

	return ret;
}
