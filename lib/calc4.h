/* header for calc4 library
 * base calculator functions, without output tools,
 * some functions from number theory
 * loadable table of 32bit prime numbers, 2^32 -1
*/

#ifndef _LIBCALC4_H_
#define _LIBCALC4_H_

#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

/* input line and parser constants */
#define LINESIZE 1024
#define KMAX 100

/* operators and numbers for the parser */
#define OPERATORS "+-*/%"
#define NUMBERS "0123456789abcdefABCDEFxX:"

/* the 32bit prime numbers -- 2,3,5,7, ... 4294967291, max.dim 203280221 */
extern unsigned *prim;
extern int dim;

#ifndef SHAREDIR
#define SHAREDIR "/usr/local/share/c4"
#endif
#define PTFILE "prim32.dat\0"

/* remove all white chars from string */
void strip_blanks(char *input_line);

/* parse input into arrays: number op number op ... number
*  return count of ops
*/
int input_parser(char *input_line, long long *numbers, char *operators);

/* do calculator evaluation on stripped string input
*/
long long eval4(char *input_line);

/* do multiplication ops, left to right, ... */
void proc_mult(int *kp, long long *numbers, char *operators);

/* do additive ops, left to right, ... */
void proc_add(int *kp, long long *numbers, char *operators);

/* parse human readable time structure, [months:][days:][hours:][minutes]:seconds
*/
long s60_input(char *input_line);

/* load prime table from file to prim[] and set dim
*  return 0 if succeeded
*  global variables used: unsigned prim[], int dim
*/
int c4_load_pt(int limit);

/* fill prime table with prime numbers, prim[] and dim
*  return 0 if succeeded
*  global variables used: unsigned prim[], int dim
*/
int c4_fill_pt(int limit);

/* prime test with prim[] and dim,
*  return 1 if the number is prime,
*         0 if it is composite,
*         -1 if test cannot decide due to prim[] limitation
*  global variables used: unsigned prim[], int dim
*/
int c4_is_prim(unsigned long long n);

/* return array of prime index and exponent pairs, 
*  array terminated with -1
*  global variables used: unsigned prim[], int dim
*/
int *c4_prim_fact(unsigned long long n);

/* PI(n) is the number of prime numbers until n
*  return an estimation
*/
unsigned long long c4_pi(unsigned long long n);

/* least common divisor, lnko(n,m)
* and the extended algoritm for d = a*x + b*y
*/
long long c4_lnko(long long a, long long b);
long long c4_lnko3(long long a, long long b, long long *x, long long *y);

/* least common multiple, lkkt(n,m)
*/
unsigned long long c4_lkkt(unsigned long long n, unsigned long long m);

/* count of divisors of number n
*  return 0 if cannot decide, limit reached
*         1 if number is 1
*         2 if number is prime
*         >2 if number is composite
*  uses c4_prim_fact()
*/
unsigned long long c4_cnt_div(unsigned long long n);

/* sum of divisors of number n
*  uses c4_prim_fact()
*/
unsigned long long c4_sum_div(unsigned long long n);

/* rapid calculation of base^power modulo modulus
*/
long long c4_rapid_exp(long long base, long long power, unsigned long long modulus);

/* run the Fermat test on number with witness
*  return 0 if composite,
*         1 if test passed, maybe a prime,
*         -1 overflow
*/
int c4_fermat_test(unsigned long long n, long w);

/* number of relative primes to n
*/
unsigned long long c4_phi(unsigned long long n);

/* is relative prime
*/
int c4_is_rel_prim(unsigned long long n, unsigned long long m);

#endif  /* _LIBCALC4_H_ */
