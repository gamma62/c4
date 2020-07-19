/* c4.c
 * 
 * Calculator with functions from the number theory.
 * based on libcalc4.so
*/

#include <stdio.h>
#include <unistd.h>	/* getopt() */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>

#include <calc4.h>
#include <config.h>

#ifdef _HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

void usage(char hex, char dec, char oct, char bin, char s60);
void bin_output(long in_number);
void s60_output(long in_number);
void list_some_primes(long long from);
void list_prim_factors(unsigned long long number);
void run_base_fermat_test(unsigned long long number);
void types(unsigned long long number);

unsigned *prim = NULL;
int dim = 0;

int main(int argc, char *argv[])
{
	int opt = 0;
	char hex = 0, dec = 0, oct = 0, bin = 0, s60 = 0;
	long long number = -1;
	unsigned long long unumber = 1;
	unsigned long long x = 1;
	int r = 0;
	long long d = 1;
#ifdef _HAVE_READLINE
	char *linebuff = NULL;
#else
	char linebuff[1024];
#endif
	char prompt[] = "> \0\0\0";

	/* argument processing */
	while ((opt = getopt(argc, argv, "xdobs")) != -1) {
		switch ((char) opt) {
		case 'x':       /* hexadecimal output */
			hex = 1;
			break;
		case 'd':       /* decimal output */
			dec = 1;
			break;
		case 'o':       /* octal output */
			oct = 1;
			break;
		case 'b':       /* binary output */
			bin = 1;
			break;
		case 's':       /* sexagesimal output */
			s60 = 1;
			break;
		case 'h':
			usage(hex, dec, oct, bin, s60);
			exit(0);
			break;
		default:
			break;
		}
	}
	while (optind < argc) {
		number = atoll(argv[optind]);
		optind++;
	}
	if (!hex && !dec && !oct) {
		dec = 1;
	}

#ifdef _HAVE_READLINE
	using_history();
#endif
	usage(hex, dec, oct, bin, s60);

	/* main loop: input processing */
	while (TRUE) {
#ifdef _HAVE_READLINE
		if (linebuff != NULL)
			free(linebuff);
		linebuff = readline(prompt);
		add_history(linebuff);
#else
		write(1, prompt, 3);
		r = read(0, linebuff, LINESIZE-1);
		if (r == 0) { /* EOF */
			break;
		}
#endif
		/* readline returns NULL if ^D pressed */
		if (linebuff == NULL)
			break;

		/* at least not NULL now, this is important */
		strip_blanks(linebuff);
		r = strlen(linebuff);
		if (linebuff[r-1] == '\n')
			linebuff[--r] = '\0';

		/* check loop-breaking */
		if (r < 1)
			continue;
		if (linebuff[0] == 'q')
			break;

		/* here is something todo, not NULL and not empty */
		if (linebuff[0] == 'h') {
			usage(hex, dec, oct, bin, s60);

		/* eval dot commands */
		} else if (linebuff[0] == '.') {
			if (linebuff[1] == 'q') {
				break;
			} else if (linebuff[1] == 'h') {
				usage(hex, dec, oct, bin, s60);
				continue;
			}

			/* change the output */
			switch (linebuff[1]) {
			case 'x':
				hex = (r>2 && linebuff[2] == '-') ? 0 : 1;
				break;
			case 'd':
				dec = (r>2 && linebuff[2] == '-') ? 0 : 1;
				break;
			case 'o':
				oct = (r>2 && linebuff[2] == '-') ? 0 : 1;
				break;
			case 'b':
				bin = (r>2 && linebuff[2] == '-') ? 0 : 1;
				break;
			case 's':
				s60 = (r>2 && linebuff[2] == '-') ? 0 : 1;
				break;

			/* load/unload prim[] table */
			case 'u':
				/* unload prime table */
				if (prim != NULL) {
					free(prim);
					prim = NULL;
					dim = 0;
					printf("unloaded\n");
				}
				break;
			case 'l':
				/* load from prepared prim32.dat, ZERO to load all */
				number = 1000*1000LL;
				if (r>2) {
					number = strtoll(linebuff+2, (char **)NULL, 0);
					if (number < 0) number = 0;
				}
				d = c4_load_pt(number);
				if (d != 0) {
					printf("load failed, %lld\n", d);
				}
				break;

			/* regenerate prim[] table */
			case 'f':
				number = 0;
				if (r>2) {
					number = strtoll(linebuff+2, (char **)NULL, 0);
				}
				d = c4_fill_pt(number);
				if (d != 0) {
					printf("fill failed, %lld\n", d);
				}
				break;

			/* prime test with table */
			case 'p':
				if (r>2) {
					unumber = strtoull(linebuff+2, (char **)NULL, 0);
					if (prim != NULL) {
						d = c4_is_prim(unumber);
						if (d == 1) {
							printf("%llu is prime\n", unumber);
						} else if (d == 0) {
							printf("%llu is composite\n", unumber);
						} else {
							printf("cannot decide (too large)\n");
						}
					} else {
						printf("prim table not loaded\n");
					}
				}
				break;

			/* prime factor with table */
			case 'P':
				if (r>2) {
					if (prim != NULL) {
						unumber = strtoull(linebuff+2, (char **)NULL, 0);
						list_prim_factors(unumber);
					} else {
						printf("prim table not loaded\n");
					}
				}
				break;

			/* list primes in table by value or index */
			case 'I':
			case 'i':
				if (r>2) {
					number = strtoll(linebuff+2, (char **)NULL, 0);
				} else {
					number = 0;
				}
				if (prim != NULL) {
					if (linebuff[1] == 'I') {
						int i = c4_pi((unsigned long long)number);
						printf("PI(%llu)=%d   ", number, i);
						if (i >= dim) {
							printf("index above dim %d\n", dim);
							i = dim-1;
							//break;
						}
						if (prim[i] < number) {
							while(prim[i] < number && i < dim) {
								i++;
							}
							i--;
						} else {
							while(prim[i] > number && i > 1) {
								i--;
							}
						}
						printf("-> index: %d\n", i);
						number = i;
					}
					list_some_primes(number);
				} else {
					printf("prim table not loaded\n");
				}
				break;

			/* base Fermat prime test */
			case 't':
				if (r>2) {
					unumber = strtoull(linebuff+2, (char **)NULL, 0);
					run_base_fermat_test(unumber);
				}
				break;

			/* Euler's Totient function */
			case 'T':
				if (r>2) {
					unumber = strtoull(linebuff+2, (char **)NULL, 0);
					x = c4_phi(unumber);
					printf("%llu -> %llu\n", unumber, x);
				}
				break;

			/* the types */
			case 'y':
				if (r>2) {
					unumber = strtoull(linebuff+2, (char **)NULL, 0);
				} else {
					unumber = 0;
				}
				types(unumber);
				break;

			default:
				break;
			}

		/* now, calc for me something, r >= 1 */
		} else {
			number = eval4(linebuff);
			if (hex)
				printf("%#llx ", number);
			if (oct)
				printf("%#llo ", number);
			if (dec)
				printf("%lld ", number);
			if (bin)
				bin_output(number);
			if (s60)
				s60_output(number);
			printf("\n");
		}
	}/*while*/
	printf("\n");

	if (prim != NULL)
		free(prim);
#ifdef _HAVE_READLINE
	if (linebuff != NULL)
		free(linebuff);
#endif

	return 0;
}

void usage(char hex, char dec, char oct, char bin, char s60)
{
	printf("usage:\n");
	printf("operators: %s\n", OPERATORS);
	printf("number outputs (xdobs): %s%s%s%s%s\n",
		(hex ? "hex " : ""),
		(dec ? "dec " : ""),
		(oct ? "oct " : ""),
		(bin ? "bin " : ""),
		(s60 ? "s60 " : ""));
	printf("special commands: \n");
	printf("is prim test (Euclid): .p <N>\n");
	printf("prim fact (Euclid): .P <N>\n");
	printf("list primes by value (Euclid): .I [<N>]\n");
	printf("list primes by index (Euclid): .i [<N>]\n");
	printf("run prim test (Fermat): .t <N>\n");
	printf("fill prime table: .f [<N>]\n");
	printf("load prime table: .l [<N>]\n");
	printf("unload prime table: .u\n");
	printf("Totient function (phi): .T <N>\n");
	printf("types: .y\n");
	printf("quit: .q/q\n");
}

void bin_output(long in_number)
{
	unsigned long mask=0, number;
	char zeroprefix = 1;
	mask = ~(~mask>>1);	/* highest bit on */

	if (in_number == 0) {
		printf("0 ");
		return;
	}

	if (in_number < 0) {
		printf("-");
		number = (unsigned long) -in_number;
	} else {
		number = (unsigned long) in_number;
	}

	while (mask) {
		if (!zeroprefix || (number & mask)) {
			zeroprefix = 0;
			printf("%d", ((number & mask) ? 1 : 0));
		}
		mask >>= 1;
	}
	printf(" ");

	return;
}

void s60_output(long in_number)
{
	unsigned long number = 0L;
	unsigned long h, m, s, day, mon;

	if (in_number < 0) {
		printf("-");
		number = (unsigned long) -in_number;
	} else {
		number = (unsigned long) in_number;
	}

	s = number % 60;
	number = number / 60;
	if (number == 0) {
		printf("%ld ", s);
		return;
	}

	m = number % 60;
	number = number / 60;
	if (number == 0) {
		printf("%ld:%ld ", m, s);
		return;
	}

	h = number % 24;
	number = number / 24;
	if (number == 0) {
		printf("%ld:%ld:%ld ", h, m, s);
		return;
	}

	day = number % 30;
	mon = number / 30;
	if (mon == 0) {
		printf("%ld:%ld:%ld:%ld ", day, h, m, s);
	} else {
		printf("%ld:%ld:%ld:%ld:%ld ", mon, day, h, m, s);
	}

	return;
}

void list_some_primes(long long from)
{
	int j=0;

	if (prim != NULL && from >= 0 && from < dim) {
		for(j=0; from+j < dim && j < 100; j++) {
			printf("%u ", prim[from+j]);
		}
		printf("\n");
	}

	return;
}

void list_prim_factors(unsigned long long number)
{
	int *ans=NULL;
	int i=0, q=0, u=0;

	if (prim != NULL) {
		ans = c4_prim_fact(number);

		if (ans == NULL) {
			printf("cannot decide\n");

		} else if (ans[0] == -1) {
			printf("%lld is a prime number\n", number);
			free(ans);
			ans=NULL;

		} else {
			printf("%lld =", number);
			/* Carmichael number, the Korselt's criteria:
			*  N is a universal pseudo-prime <-->
			*  N = p*q*...*r (different prime numbers)
			*  for each p: (p-1) | (n-1)
			*/
			for(i=0; ans[i] >= 0; i+=2) {
				if (i > 0)
					printf(" *");
				if (ans[i+1] > 1) {
					printf(" %u^%d", prim[ans[i]], ans[i+1]);
					q++;
				} else {
					printf(" %u", prim[ans[i]]);
					u += (number-1) % (prim[ans[i]]-1);
				}
			}
			printf("\n");
			if (q > 0) {
				printf("not Q-free, not a Carmichael number\n");
			} else {
				printf("Carmichael number? %s\n", (u==0 ? "yes" : "no"));
			}
			free(ans);
			ans=NULL;
		}
	}

	return;
}

void run_base_fermat_test(unsigned long long number)
{
	long witness[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
	int ans=0;
	unsigned i=0;

	if (number < 2) {
		printf("number is too small\n");
		return;
	}

	while ( i < (sizeof(witness)/(sizeof(witness[0]))) ) {
		printf("w:%ld ", witness[i]);
		ans = c4_fermat_test(number, witness[i]);
		printf("%d; ", ans);
		if (ans != 1) break;
		i++;
	}
	printf("\n");

	if (ans < 0) {
		printf("overflow\n");
	} else if (ans == 0) {
		printf("%lld is composite\n", number);
	} else {
		printf("prime (probably)\n");
	}

	return;
}

void types(unsigned long long number)
{
	printf("int sizeof %u, max %u\n", (unsigned)sizeof(int), INT_MAX);
	printf("long sizeof %u, max %lu\n", (unsigned)sizeof(long), LONG_MAX);
	printf("long long sizeof %u, max %llu\n", (unsigned)sizeof(long long), LLONG_MAX);
	if (prim != NULL && number > 0 && number <= prim[dim-1]) {
		printf("estimated PI(%llu) is %llu\n", number, c4_pi(number));
	}
	return;
}

/* eof */
