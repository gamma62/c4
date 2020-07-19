/* mill.c on FreeBSD and Linux */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#include <pthread.h>

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

#ifdef LINUX
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#else
#include <sys/sysctl.h>
#endif

int get_system_info(void)
{
#ifdef LINUX
//	unsigned int eax=11, ebx=0, ecx=1, edx=0;
//	unsigned int ncores=0, nthreads=0;
	struct utsname buffer;
	char getbuff[4096];
	int ncpu = 1;
	char version[200];
	char machine[200];
	char model[200];
	int byteorder = 0x1234;
	version[0] = '\0';
	machine[0] = '\0';
	model[0] = '\0';

	if (uname(&buffer) == 0) {
		sprintf(version, "%s %s\n", buffer.sysname, buffer.release);
		strncpy(machine, buffer.machine, sizeof(machine));
	}

	FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
	while (fgets (getbuff, 100, cpuinfo)) {
		if (strncmp(getbuff, "model name	: ", 12) == 0) {
			strncpy(model, getbuff+13, sizeof(model));
			break;
		}
	}
	fclose(cpuinfo);

	ncpu = get_nprocs();

	/* little endian or big endian? */
	if ( ((*(char *)&byteorder) & 0x0f) == (byteorder & 0x0f) )
		byteorder = 1234; //little endian (Intel)
	else
		byteorder = 4321; // big endian (Motorola)

//Linux 5.4.0-37-generic
	printf("%s", version);
//Intel(R) Core(TM) i3-7130U CPU @ 2.70GHz
	printf("%s", model);
//4 CPUs, arch x86_64, byteorder 1234
	printf("%d CPUs, arch %s, byteorder %d\n", ncpu, machine, byteorder);


//	printf("This system has %d processors configured and %d processors available.\n",
//		get_nprocs_conf(), get_nprocs());
//
//	asm volatile("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "0" (eax), "2" (ecx) : );
//	printf("Cores: %d Threads: %d Actual thread: %d\n",
//		eax,
//		ebx,
//		edx);
//
//	asm volatile("cpuid": "=a" (ncores), "=b" (nthreads) : "a" (0xb), "c" (0x1) : );
//	printf("Cores: %d Threads: %d with HyperThreading: %s\n",
//		ncores,
//		nthreads,
//		(ncores!=nthreads) ? "Yes" : "No");

	return ncpu;
#else
	int ncpu = 1;
	int mib[2];
	char version[200];
	char machine[200];
	char model[200];
	int byteorder;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ncpu);
	if (sysctl(mib, 2, &ncpu, &len, NULL, 0))
		ncpu = 1;
	mib[1] = HW_MODEL;
	len = sizeof(model);
	if (sysctl(mib, 2, model, &len, NULL, 0))
		model[0] = '\0';
	mib[1] = HW_MACHINE;
	len = sizeof(machine);
	if (sysctl(mib, 2, machine, &len, NULL, 0))
		machine[0] = '\0';
	mib[1] = HW_BYTEORDER;
	len = sizeof(byteorder);
	if (sysctl(mib, 2, &byteorder, &len, NULL, 0))
		byteorder = 0;

	mib[0] = CTL_KERN;
	mib[1] = KERN_VERSION;
	len = sizeof(version);
	if (sysctl(mib, 2, version, &len, NULL, 0)) {
		version[0] = '\n';
		version[1] = '\0';
	}

//FreeBSD 12.0-RELEASE r341666 GENERIC
	printf("%s", version);
//Intel(R) Core(TM) i3-7130U CPU @ 2.70GHz
	printf("%s\n", model);
//4 CPUs, arch amd64, byteorder 1234
	printf("%d CPUs, arch %s, byteorder %d\n", ncpu, machine, byteorder);

	return ncpu;
#endif
}

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

	printf("saving %d items to %s ...\n", dim, fname);
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

	printf("preparation...\n");
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

	// init -- one testnum in each loop
	while(testnum < 1000 || dim < 2*mult*npthread) {
		if (is_prime_thread(testnum, dim)) {
			prim[dim++] = testnum;
		}
		testnum += 2;
	}
	//printf("init done: dim %d, last prime %u, next testnum %u\n", dim, prim[dim-1], testnum);

	printf("processing (mult=%d, npthread=%d) ...\n", mult, npthread);
	if (npthread > 1) {
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
		// fallback
		fill_prim_table_thread (NULL);
	}
	printf("processing finished: dim %d, last prime %u\n", dim, prim[dim-1]);

	if (saveit())
		return 1;

	return 0;
}

int main(int argc, const char *argv[])
{
	long double x = 1.0;
	int ret = 0;

	npthread = get_system_info(); // print system info
	mult = 256;

	if (argc == 1) {
		printf("usage: %s <mult> [<npthread>]\n", argv[0]);
		printf("    defaults: mult=%d npthread=%d\n", mult, npthread);
		return 0;
	}

	last_testnum = UINT_MAX;
	x = last_testnum;
	//malloc_dim = (int) (1.07 * x / logl(x));
	malloc_dim = (int) (x / (logl(x) - 1.5));
	malloc_dim = ((malloc_dim/100)+1)*100;
	//printf("last testnum %u, malloc dim %d\n", last_testnum, malloc_dim);

	if (argc > 1) {
		mult = atoi(argv[1]);
		if (argc > 2) {
			npthread = atoi(argv[2]);
		}
	}
	if (mult < 1) mult = 1;
	if (npthread < 1) npthread = 1;
	// begin_task has 64bits
	if (npthread > 62) npthread = 62;

	ret = fill_prim_table_main();
	if (ret == 0)
		printf("success\n");
	else
		printf("failure\n");

	free(testnumbers);
	free(pthreads);
	free(prim);

	return ret;
}
