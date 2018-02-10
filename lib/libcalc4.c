/* fuctions for libcalc4.so
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>

#include <calc4.h>

void strip_blanks(char *input_line)
{
	char *in=input_line, *out=input_line;

	while (*in != '\0') {
		if (*in == ' ' || *in == '\t' || *in == '\n' || *in == '\r') {
			in++;
		} else {
			*out++ = *in++;
		}
	}
	*out = '\0';

	return;
}

int input_parser(char *input_line, long long *numbers, char *operators)
{
	int k = 0;	/* count of operators */
	char *buffp = input_line;
	char next_is_number = TRUE;
	char in_number = FALSE;
	char in_paren = FALSE;
	int in_depth = 0;
	char number_buff[LINESIZE];
	char *np = number_buff;

	while (TRUE) {

		if ((*buffp != '\0') && ( (in_depth > 0) ||
		(*buffp == 0x28 && np == number_buff) ))
		{
			if (!next_is_number) {
				return -1; //parenthesis, but operator expected
			}
			in_paren = TRUE;
			if (*buffp == 0x28) {
				in_depth++;
			} else if (*buffp == 0x29) {
				in_depth--;
			}
			/* collect characters for embedded call */
			*np++ = *buffp++;

		} else if ((*buffp != '\0') && ( (strchr(NUMBERS, *buffp) != NULL) ||
		((*buffp == '-' || *buffp == '+') && np == number_buff) ))
		{
			if (!next_is_number) {
				return -2; //number, but operator expected
			}
			in_number = TRUE;
			/* collect number characters */
			*np++ = *buffp++;

		} else {
			if (in_number) {
				/* number post-processing */
				in_number = FALSE;
				next_is_number = FALSE;
				*np = '\0';
				if (strchr(number_buff, ':') == NULL) {
					numbers[k] = strtol(number_buff, (char **)NULL, 0);
				} else {
					numbers[k] = s60_input(number_buff);
				}
				/* reset np */
				np = number_buff;
			}
			else if (in_paren) {
				/* processing section in parenthesis, recursive call */
				in_paren = FALSE;
				next_is_number = FALSE;
				if (np - number_buff > 2) {
					/* strip close paren and skip open paren */
					*(np-1) = '\0';
					numbers[k] = eval4(number_buff+1);
				} else {
					numbers[k] = 0; // empty paren range
				}
				/* reset np */
				np = number_buff;
			}

			/* ----- break the loop ---------------------- */
			if (*buffp == '\0' || k == KMAX-1) {
				break;
			}
			/* ------------------------------------------- */

			if (strchr(OPERATORS, *buffp) != NULL) {
				if (next_is_number) {
					return -4; // operator, after operator
				}
				operators[k++] = *buffp++;
				next_is_number = TRUE;
			} else {
				return -5; // should be an operator, but is not
			}
		}

	} /* while */

	if (k == KMAX-1) {
		return -6; // too many items
	} else if (next_is_number) {
		return -7; // still waiting for number
	} else {
		return (k);
	}
}

long long eval4(char *input_line)
{
	/* numbers[0], operators[0], numbers[1], ..., numbers[k-1], operators[k-1], numbers[k] */
	int k;	/* count of operators */
	long long numbers[KMAX+1];
	char operators[KMAX];

	if (*input_line == '\0')
		return 0;

	/* processing input_line into arrays */
	k = input_parser(input_line, numbers, operators);
	if (k < 0) {
		fprintf(stderr, "parse error (%d)\n", k);
		return 0;
	}

	/* highest precedence: parenthesis */

	/* high prec.: mult.operators */
	proc_mult(&k, numbers, operators);

	/* low prec.: add.operators */
	proc_add(&k, numbers, operators);

	if (k != 0) {
		fprintf(stderr, "processing failed (%d)\n", k);
		return 0;
	}

	return (numbers[k]);
}

void proc_mult(int *kp, long long *numbers, char *operators)
{
	int i, j;
	long long number = 0;
	char change = FALSE;

	for (i=0; i<*kp; i += (change ? 0 : 1)) {
		change = FALSE;
		if (operators[i] != '*' && operators[i] != '/' && operators[i] != '%')
			continue;

		if ((operators[i] == '/' || operators[i] == '%') && numbers[i+1] == 0L) {
			fprintf(stderr, "divide by zero\n");
			*kp = 0;
			numbers[0] = 0L;
			return;
		}

		switch (operators[i]) {
		case '*':
			number = numbers[i] * numbers[i+1];
			break;
		case '/':
			number = numbers[i] / numbers[i+1];
			break;
		case '%':
			number = numbers[i] % numbers[i+1];
			break;
		}

		/* numbers[i], operators[i], numbers[i+1], ..., numbers[k-1], operators[k-1], numbers[k] */
		for (j=i; j<*kp-1; j++) {
			operators[j] = operators[j+1];
			numbers[j+1] = numbers[j+2];
		}
		numbers[i] = number;
		(*kp)--;
		change = TRUE;
	}

	return;
}

void proc_add(int *kp, long long *numbers, char *operators)
{
	long long number = 0;
	int i, j;
	char change = FALSE;

	for (i=0; i<*kp; i += (change ? 0 : 1)) {
		change = FALSE;
		if (operators[i] != '+' && operators[i] != '-')
			continue;

		switch (operators[i]) {
		case '+':
			number = numbers[i] + numbers[i+1];
			break;
		case '-':
			number = numbers[i] - numbers[i+1];
			break;
		}

		/* numbers[i], operators[i], numbers[i+1], ..., numbers[k-1], operators[k-1], numbers[k] */
		for (j=i; j<*kp-1; j++) {
			operators[j] = operators[j+1];
			numbers[j+1] = numbers[j+2];
		}
		numbers[i] = number;
		(*kp)--;
		change = TRUE;
	}

	return;
}

long s60_input(char *input_line)
{
	long number = 0L;
	char *ptr;
	long h=0, m=0, s=0, day=0, mon=0;
	int len = strlen(input_line);

	ptr = input_line+len;
	if (ptr > input_line) {
		do { ptr--; } while (ptr >= input_line && *ptr >= '0' && *ptr <= '9');
		s = strtol(ptr+1, (char **)NULL, 10);
		if (ptr > input_line) {
			*ptr = '\0';
			do { ptr--; } while (ptr >= input_line && *ptr >= '0' && *ptr <= '9');
			m = strtol(ptr+1, (char **)NULL, 10);
			if (ptr > input_line) {
				*ptr = '\0';
				do { ptr--; } while (ptr >= input_line && *ptr >= '0' && *ptr <= '9');
				h = strtol(ptr+1, (char **)NULL, 10);
				if (ptr > input_line) {
					*ptr = '\0';
					do { ptr--; } while (ptr >= input_line && *ptr >= '0' && *ptr <= '9');
					day = strtol(ptr+1, (char **)NULL, 10);
					if (ptr > input_line) {
						*ptr = '\0';
						do { ptr--; } while (ptr >= input_line && *ptr >= '0' && *ptr <= '9');
						mon = strtol(ptr+1, (char **)NULL, 10);
					}//mon
				}//day
			}//hour
		}//min
	}//sec

	number = s + 60*(m + 60*(h + 24*(day + 30*mon)));
	return number;
}

int c4_load_pt(int limit)
{
	char fname[LINESIZE];
	unsigned items, r_size=0, r_ctrl=0;
	int fd=0;
	struct stat statit;

	if (prim != NULL) {
		return 1;
	}

	strncpy(fname, SHAREDIR "/" PTFILE, LINESIZE-1);
	fname[LINESIZE-1] = '\0';

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open %s\n", fname);
		return 2;
	}
	if (fstat(fd, &statit) == -1) {
		perror("fstat");
		close(fd);
		return 3;
	}

	items = statit.st_size / sizeof(int);
	if (limit <= 0) limit = items;
	if (limit > (int)items) limit = items;

	prim = (unsigned *) malloc(limit*sizeof(unsigned));
	if (prim == NULL) {
		perror("malloc");
		return (-1);
	}

	fprintf(stderr, "loading table ...\n");
	r_size = (unsigned)limit * sizeof(unsigned);
	r_ctrl = read(fd, (void *)&prim[0], r_size);
	if (r_ctrl != r_size) {
		perror("read");
		close(fd);
		return 4;
	}
	close(fd);

	dim = limit;
	fprintf(stderr, "loaded %d primes to memory, last %u\n",
		dim, prim[dim-1]);

	return 0;
}

int c4_fill_pt(int limit)
{
	int i, j, is_prime=0;
	unsigned p, q;

	if (prim != NULL) {
		return 1;
	}
	if (limit < 1) limit = 1000000;

	prim = (unsigned *) malloc(limit*sizeof(unsigned));
	if (prim == NULL) {
		perror("malloc");
		return (-1);
	}

	for (i = 0; i < limit; i++) { prim[i] = 0; }
	prim[0] = 2;
	prim[1] = 3;
	prim[2] = 5;
	prim[3] = 7;
	p = prim[i];

	fprintf(stderr, "filling table ...\n");
	for (i = 4; i < limit; p += 2) {
		is_prime = 1;
		q = (unsigned) sqrtl( (long double) p );
		for (j = 0; prim[j] <= q; j++) {
			if (p % prim[j] == 0) {
				is_prime = 0; /* composite */
				break;
			}
		}
		if (is_prime) {
			prim[i++] = p;
		}
	}
	dim = i;

	fprintf(stderr, "filled in %d primes to memory, last %u\n",
		dim, prim[dim-1]);

	return 0;
}

int c4_is_prim(unsigned long long n)
{
	int i=0, j=0, is_prime=1;
	long double x=0, C=1.051598463361330;
	unsigned long long p, q;

	if (prim == NULL) {
		return -1;
	}

	if (n == 2ULL) {
		return 1;
	} else if ((n & 1ULL) == 0) {
		return 0;
	}

	if (n < UINT_MAX && n < prim[dim-1]) {
		// hash-alike, using prim[]

		x = n;
		j = (int) ( C * x / logl(x) );
		if (j < 0) j = 0;
		if (j > dim-1) j = dim-1;

		if (prim[j] < n) {
			// index j is under-estimated
			while (j+10000 < dim-1 && prim[j+10000] < n)
				j += 10000;
			while (j+1000 < dim-1 && prim[j+1000] < n)
				j += 1000;
			while (j+100 < dim-1 && prim[j+100] < n)
				j += 100;
			while (j+10 < dim-1 && prim[j+10] < n)
				j += 10;
			while (j < dim-1 && prim[j] < n)
				j++;
		} else if (prim[j] > n) {
			// index j is over-estimated
			while (j > 10000 && prim[j-10000] > n)
				j -= 10000;
			while (j > 1000 && prim[j-1000] > n)
				j -= 1000;
			while (j > 100 && prim[j-100] > n)
				j -= 100;
			while (j > 10 && prim[j-10] > n)
				j -= 10;
			while (j > 0 && prim[j] > n)
				j--;
		}

		if (prim[j] == n) {
			is_prime = 1; /* found in prim[] */
		} else {
			is_prime = 0; /* not found in prim[] --> composite */
		}

	} else {
		// old school

		q = sqrtl((long double) n);

		for (i = 0; i < dim && prim[i] <= q; i++) {
			if (n % (prim[i]) == 0) {
				is_prime = 0; /* composite */
				break;
			}
		}

		if (is_prime != 0 && q > prim[dim-1]) {
			/* n is too large, prime table was not enough */
			p = prim[dim-1];
			while (p < q) {
				if (n % p == 0) {
					is_prime = 0; /* composite */
					break;
				}
				p += 2; /* only odd numbers */
			}
		}

		/* 64bit int : tested with all possible 32bit numbers -- if no divisor then this is a prime */
	}

	return is_prime;
}

int *c4_prim_fact(unsigned long long n)
{
	unsigned long long q;
	int *pf=NULL;
	int i=0, e=0;
	unsigned als=0;

	if (prim == NULL) {
		return NULL;
	}

	q = sqrtl((long double) n);
	if (q > prim[dim-1]) {
		printf("warning: number %llu too large (index:%d prim:%u)\n", n, dim-1, prim[dim-1]);
		return NULL;
	}

	als=0;
	pf = (int *) malloc((als+1)*sizeof(int));
	if (pf == NULL) {
		return NULL;
	}
	pf[als] = -1;

	while (n > 1 && i < dim && prim[i] <= n) {
		if (als == 0 && q < prim[i]) {
			//cannot have divisor
			break;
		}
		e = 0;
		while ((n > 1) && (n % prim[i] == 0)) {
			n /= prim[i];
			e++;
		}
		if (e > 0) {
			/* save (i,e) */
			als += 2;
			pf = (int *) realloc((void *)pf, (als+1)*sizeof(int));
			if (pf == NULL) {
				return NULL;
			}
			pf[als-2] = i; /* index in prim[] */
			pf[als-1] = e; /* exponent */
			pf[als] = -1;
		}
		i++;
	}

	return pf;
}

unsigned long long c4_pi(unsigned long long n)
{
	long double x = n;
	long double C = 1.051598463361330;
	unsigned long long px = 0;

	px = C * x / logl(x);

	return px;
}

unsigned long long c4_lnko_historical(unsigned long long n, unsigned long long m)
{
	if (n <= 1 || m <= 1)
		return 1;

	while (n != m) {
		if (n > m) {
			n %= m;
			if (n == 0) n = m;
		} else if (n < m) {
			m %= n;
			if (m == 0) m = n;
		}
	}

	return n;
}

/* normal GCD, (a,b) -> d
 * loop based
*/
long long c4_lnko(long long a, long long b)
{
	long long r;

	// 1 compare, 3 reminder, 3 assignment
	while (b != 0) {
		r = a%b;
		a = b;
		b = r;
	}

	return a;
}

/* extended GCD, (a,b) -> d with x, y
 * where d = (a,b) and d = a*x + b*y
 * recursive
*/
long long c4_lnko3(long long a, long long b, long long *x, long long *y)
{
	long long ytmp, d;
	lldiv_t div;

	if (b != 0) {
		/* stack down, save div in stack frame */
		div = lldiv(a, b);
		// a <- (b), b <- (a-[a/b]*b)
		d = c4_lnko3(b, div.rem, x, y);

		/* stack up, calculate x and y from "d = a' * x + b' * y" formula
		*	d = (b) * x + (a-[a/b]*b) * y
		*	d = a * y + b * (x-[a/b]*y)
		*/
		ytmp = *y;
		// x <- (y), y <- (x-[a/b]*y)
		*y = *x - div.quot * ytmp;
		*x = ytmp;
	} else {
		/* finished, d is ready, turn back */
		d = a;
		*x = 1;
		*y = 0;
	}

	return d;
}

unsigned long long c4_lkkt(unsigned long long n, unsigned long long m)
{
	long long lkkt = 1;

	/* potenital overflow, check with ULLONG_MAX */

	lkkt = c4_lnko(n, m);
	if (lkkt != 0) {
		lkkt = n / lkkt * m;
	}

	return lkkt;
}

unsigned long long c4_cnt_div(unsigned long long n)
{
	int *ans=NULL;
	int i=0;
	unsigned long long dn = 1;

	if (prim == NULL) {
		return 0;
	}

	if (n < 1) {
		return 0; // invalid input
	} else if (n == 1) {
		return 1; // by definition
	} else if ((n == 2) || (n == 3)) {
		return 2;
	}

	ans = c4_prim_fact(n);
	if (ans == NULL) {
		return 0; // cannot decide -- limit reached
	}

	if (ans[0] == -1) {
		// prime number
		dn = 2;
	} else {
		// calculate number of divisors
		dn = 1;
		for(i=0; ans[i] >= 0; i+=2) {
			dn *= (1 + ans[i+1]);
		}
	}

	free(ans);
	ans=NULL;

	return dn;
}

unsigned long long c4_sum_div(unsigned long long n)
{
	int *ans=NULL;
	int i=0, j=0; 
	int dn=0, *dn_arr=NULL;
	int inlen=0, p=0, powers=0, ppow=0, pow=0;
	int spd=0;

	if (prim == NULL) {
		return 0;
	}

	if (n < 1) {
		return 0; // invalid input
	} else if (n == 1) {
		return 1; // by definition
	} else if ((n == 2) || (n == 3)) {
		return 1;
	}

	ans = c4_prim_fact(n);
	if (ans == NULL) {
		return 0; // cannot decide -- limit reached
	}

	if (ans[0] == -1) {
		// prime number
		spd = 1;
	} else {
		// calculate number of divisors
		dn = 1;
		for(i=0; ans[i] >= 0; i+=2) {
			dn *= (1 + ans[i+1]);
		}

		// array for the divisors
		dn_arr = (int *) malloc(dn*sizeof(int));
		if (dn_arr == NULL) {
			// out of memory
			free(ans);
			ans=NULL;
			return 0;
		}
		for (j=0; j < dn; j++) {
			dn_arr[j] = 0;
		}

		// enumerate divisors and calculate the whole sum
		dn_arr[0] = 1;
		inlen = 1; // input length
		for(i=0; ans[i] >= 0; i+=2) {
			p = prim[ans[i]]; // the prime, by index
			powers = ans[i+1]; // exponent, >= 1
			ppow = 1;
			for(pow=1; pow <= powers; pow++) {
				ppow *= p;
				for (j=0; j < inlen; j++) {
					dn_arr[pow*inlen+j] = dn_arr[j] * ppow;
				}
			}
			inlen += inlen*powers;
		}

		// sum, except the number itself, the last index in dn_arr[]
		spd = 0;
		for (j=0; j < dn-1; j++) {
			spd += dn_arr[j];
		}
	}

	free(dn_arr);
	dn_arr=NULL;
	free(ans);
	ans=NULL;

	return spd;
}

long long c4_rapid_exp(long long base, long long power, unsigned long long modulus)
{
	unsigned long long calc=0, xbase=0, bits=0;

	/* check invalid input */
	if ((base < 1) || (power < 0) || (modulus < 1))
		return -1;

	/* check potential overflow */
	if (modulus > ULLONG_MAX/modulus)
		return -1;

	/* calculate  calc = base^power  (modulo modulus) */
	calc = 1;
	xbase = base;
	bits = power;
	while (bits > 0) {
		if (bits & 1)
			calc = (calc * xbase) % modulus;
		bits >>= 1;
		if (bits > 0)
			xbase = (xbase * xbase) % modulus;
	}

	return ((calc > LLONG_MAX) ? -1 : (long long)calc);
}

int c4_fermat_test(unsigned long long n, long w)
{
	long long calc=0;

	if (n == 2) {
		return 1; // 2 is a prime
	} else if ((n & 1) == 0) {
		return 0; // multiple of 2, composite
	}

	if ((n % w) == 0) {
		return 0; // composite number
	}

	/* the Fermat test:  n | w^(n-1) - 1
	* prime numbers and pseudo primes pass, composite numbers fail
	*/
	calc = c4_rapid_exp(w, n-1, n);

	if (calc < 0) {
		return -1; // overflow
	} else if ((calc % n) != 1) {
		return 0; // composite number
	} else {
		return 1; // maybe a prime
	}
}

unsigned long long c4_phi(unsigned long long n)
{
	unsigned long long phi_of_n = n, p = 2;
	int i=0;

	if (prim == NULL)
		return 0;

	if (n <= 1)
		return 1;

	for (i = 0; i < dim && n > 1; i++) {
		p = prim[i];
		if (p > n)
			break;
		if ((n % p) == 0) {
			phi_of_n = phi_of_n / p * (p-1);
			do {
				n /= p;
			} while ((n > 1) && ((n % p) == 0));
		}
	}

	return phi_of_n;
}

int c4_is_rel_prim(unsigned long long n, unsigned long long m)
{
	int has_cdiv = 0;

	if (n < 1 || m < 1)
		return 0;

	return (has_cdiv == 0);
}

/* eof */
