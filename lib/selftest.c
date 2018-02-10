/* selftest */

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

/* the 32bit prime numbers -- 2,3,5,7, ... 4294967291U < 4294967295U */
unsigned *prim = NULL;
int dim = 0;

int main(void)
{
	int ret=0, limit=0, i=0, n=0, m=0, szumma=0, dn=0;
	long long small=0, tri=0;
	unsigned long long big=0;
	long long result=0;

	limit = 1000000;
	printf("load %d items ...\n", limit);
	ret = c4_load_pt(limit);
	if (ret) {
		printf("load failed (%d)\n", ret);
		return 1;
	}
	printf("\n");

	//1mill...
	printf("the %dth prime is %u (is 15485863)\n", limit, prim[limit-1]);
	printf("prime test...\n");
	printf("%u is prime? %d\n", prim[limit-1], c4_is_prim(prim[limit-1]));
	printf("\n");

	free(prim); prim=NULL;

	//105mill...
//loaded 105097564 items to memory, last 2147483629
//from .i 105097500
//2147482231 2147482237 2147482273 2147482291 2147482327 2147482343 2147482349 2147482361 2147482367
//2147482409 2147482417 2147482481 2147482501 2147482507 2147482577 2147482583 2147482591 2147482621
//2147482661 2147482663 2147482681 2147482693 2147482697 2147482739 2147482763 2147482801 2147482811
//2147482817 2147482819 2147482859 2147482867 2147482873 2147482877 2147482921 2147482937 2147482943
//2147482949 2147482951 2147483029 2147483033 2147483053 2147483059 2147483069 2147483077 2147483123
//2147483137 2147483171 2147483179 2147483237 2147483249 2147483269 2147483323 2147483353 2147483399
//2147483423 2147483477 2147483489 2147483497 2147483543 2147483549 2147483563 2147483579 2147483587
//2147483629

	limit = 0;
	printf("load all 32bit primes ...\n");
	ret = c4_load_pt(limit);
	if (ret) {
		printf("load failed (%d)\n", ret);
		return 1;
	} else {
		// reloaded: 2 to 4294967291, dim 203280221
		printf("loaded %d, last %u (is dim=203280221 and last=4294967291)\n", dim, prim[dim-1]);
		for(i=0; i < 100; i++) {
			printf("%u ", prim[i]);
		}
		printf("\n");
		printf("control\n\
2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97 101 103 107 109 113 127 \
131 137 139 149 151 157 163 167 173 179 181 191 193 197 199 211 223 227 229 233 239 241 251 257 \
263 269 271 277 281 283 293 307 311 313 317 331 337 347 349 353 359 367 373 379 383 389 397 401 \
409 419 421 431 433 439 443 449 457 461 463 467 479 487 491 499 503 509 521 523 541 \
\n");
	}
	printf("\n");

	printf("small composite test...\n");
	small = 46279LL * 46337LL;
	ret = c4_is_prim(small);
	printf("(46279 * 46337) %lld %d\n", small, ret);
	small = 46301LL * 46327LL;
	ret = c4_is_prim(small);
	printf("(46301 * 46327) %lld %d\n", small, ret);
	small = 46307LL * 46309LL;
	ret = c4_is_prim(small);
	printf("(46307 * 46309) %lld %d\n", small, ret);

	printf("big multiple test (test takes long time)...\n");
	big = 2147482231LLU * 2147483423LLU;
	ret = c4_is_prim(big);
	printf("(2147482231 * 2147483423) %llu %d\n", big, ret);
	big = 2147482409LLU * 2147483137LLU;
	ret = c4_is_prim(big);
	printf("(2147482409 * 2147483137) %llu %d\n", big, ret);
	big = 2147482661LLU * 2147482949LLU;
	ret = c4_is_prim(big);
	printf("(2147482661 * 2147482949) %llu %d\n", big, ret);
	big = 2147482817LLU * 2147482817LLU;
	ret = c4_is_prim(big);
	printf("(2147482817 * 2147482817) %llu %d\n", big, ret);
	printf("\n");

	//rpow 7^560 mod 561 = 1
	result = c4_rapid_exp(7, 560, 561);
	printf("rpow 7^560 mod 561 = %lld\n", result);
	if (result != 1) return 1;

	printf("Fermat test on small composite numbers...\n");
	small = 46279LL*46337LL;
	ret = c4_fermat_test(small,2);
	printf("%lld %d\n", small, ret);
	small = 46301LL*46327LL;
	ret = c4_fermat_test(small,2);
	printf("%lld %d\n", small, ret);
	small = 46307LL*46309LL;
	ret = c4_fermat_test(small,2);
	printf("%lld %d\n", small, ret);
	printf("\n");

	printf("easy big primes test below 2^31 ...\n");
	big = 2147483647LLU; // 2^31-1
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483629LLU; // 2^31-19
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483587LLU; // 2^31-61
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483579LLU; // 2^31-69
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483563LLU; // 2^31-85
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483549LLU; // 2^31-99
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483543LLU; // 2^31-105
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483497LLU; // 2^31-151
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483489LLU; // 2^31-159
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 2147483477LLU; // 2^31-171
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));

	printf("easy big primes test below 2^32 ...\n");
	big = 4294967291LLU; // 2^32-5
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967279LLU; // 2^32-17
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967231LLU; // 2^32-65
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967197LLU; // 2^32-99
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967189LLU; // 2^32-107
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967161LLU; // 2^32-135
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967143LLU; // 2^32-153
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967111LLU; // 2^32-185
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967087LLU; // 2^32-209
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	big = 4294967029LLU; // 2^32-267
	printf("%llu is a prime? %d\n", big, c4_is_prim(big));
	printf("\n");

	printf("count of divisors...\n");
	//p21:
	printf("sum of divisors of %d is %llu (is 284)\n", 220, c4_sum_div(220));
	printf("sum of divisors of %d is %llu (is 220)\n", 284, c4_sum_div(284));
	printf("sum of divisors of %d is %llu (same)\n", 6, c4_sum_div(6));
	printf("sum of divisors of %d is %llu (same)\n", 28, c4_sum_div(28));
	printf("sum of divisors of %d is %llu (same)\n", 496, c4_sum_div(496));
	printf("sum of divisors of %d is %llu (same)\n", 8128, c4_sum_div(8128));
	printf("amicable numbers...\n");
	szumma=0;
	for(n=2; n<10000; n++) {
		m = c4_sum_div(n);
		if (n < m && m < 10000 && (int)c4_sum_div(m) == n) {
			szumma += n+m;
			if (m > 1000) printf("...dn(%d)=%d and dn(%d)=%d\n", n, m, m, n);
		}
	}
	printf("sum of amicable numbers below 10000 is %d (31626)\n\n", szumma);

	printf("sum of (real) divisors...\n");
	//p12:
	printf("count of divisors of %lld is %llu (is 576)\n", 76576500LL, c4_cnt_div(76576500LL));
	for(i=2; i < INT_MAX && (unsigned)i < prim[dim-1]; i++) {
		tri = i*(i+1)/2;
		if ((i & 1) == 0) {
			dn = c4_cnt_div(i/2);
			dn *= c4_cnt_div(i+1);
		} else {
			dn = c4_cnt_div((i+1)/2);
			dn *= c4_cnt_div(i);
		}
		if (dn > 400) printf("...triangular number %lld (%d*%d/2) has %d divisors\n", tri, i, i+1, dn);
		if (dn > 500) break;
	}
	printf("triangular number %lld (%d*%d/2) has %d divisors\n\n", tri, i, i+1, dn);

	printf("the phi function...\n");
	big = 6636841LLU;
	printf("%llu -> phi %llu (6631684)\n", big, c4_phi(big));
	big = 7026037LLU;
	printf("%llu -> phi %llu (7020736)\n", big, c4_phi(big));
	big = 7357291LLU;
	printf("%llu -> phi %llu (7351792)\n", big, c4_phi(big));
	big = 7507321LLU;
	printf("%llu -> phi %llu (7501732)\n", big, c4_phi(big));
	big = 8316907LLU;
	printf("%llu -> phi %llu (8310976)\n", big, c4_phi(big));
	big = 8319823LLU;
	printf("%llu -> phi %llu (8313928)\n", big, c4_phi(big));

	printf("end\n");
	if (prim != NULL) free(prim);
	return 0;
}
