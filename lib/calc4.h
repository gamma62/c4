/* header for calc4 library
 * base calculator functions, without output tools,
 * some functions from number theory
 * loadable table of 32bit prime numbers, upto 2^32-1
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

void strip_blanks(char *input_line);

int input_parser(char *input_line, long long *numbers, char *operators);

long long eval4(char *input_line);

void proc_mult(int *kp, long long *numbers, char *operators);

void proc_add(int *kp, long long *numbers, char *operators);

long s60_input(char *input_line);

int c4_load_pt(int limit);

int c4_fill_pt(int limit);

int c4_is_prim(unsigned long long n);

int *c4_prim_fact(unsigned long long n);

unsigned long long c4_pi(unsigned long long n);

long long c4_lnko(long long a, long long b);

long long c4_lnko3(long long a, long long b, long long *x, long long *y);

unsigned long long c4_lkkt(unsigned long long n, unsigned long long m);

unsigned long long c4_cnt_div(unsigned long long n);

unsigned long long c4_sum_div(unsigned long long n);

long long c4_rapid_exp(long long base, long long power, unsigned long long modulus);

int c4_fermat_test(unsigned long long n, long w);

unsigned long long c4_phi(unsigned long long n);

int c4_is_rel_prim(unsigned long long n, unsigned long long m);

#endif  /* _LIBCALC4_H_ */
