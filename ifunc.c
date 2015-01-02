/* trent: 27-dec-14: downloaded from http://pasky.or.cz/dev/glibc/ifunc.c */
/* 
 * ifunc (gnu_indirect_function): "Programmable Symbol Alias"
 * 
 * Highly-optimized function provided by an .so. Are we running
 * on SSE-capable processor where we can use this?
 *
 * We could branch on every call, or have two versions of the whole
 * library. Or, we can let the dynamic linker alias the function
 * symbol with the correct implementation on the first call!
 * (Originally for libc functions like strstr(), strcpy(), ...
 * but can be used elsewhere too!)
 *
 * ifunc is precisely that. The ifunc symbol itself is
 * void *func(void), invoked on the first call of func(), and
 * returns pointer to the _actual_ function used subsequently.
 *
 * You need: In short - 11.2+. glibc-2.10+, binutils-2.19.51.0.10.
 * Supported archs: x86*, ppc* (needs newer binutils). */

/* Compile as: gcc -std=gnu99 -O3 -Wall -fpic -shared -fvisibility=default ifunc.c -o libifunc.so */
/* Test as: echo -e '#include<stdio.h>\nextern int nifty(int *x, int n); int main(void) { int x[] = {1, 2, 3, 4}, n = sizeof(x)/sizeof(x[0]); printf("%d %d\\n", nifty(x, n), nifty(x, n)); }' | gcc -x c - -lifunc -L. -Wl,-rpath,.; ./a.out */

#include <stdbool.h>
#include <stdio.h>


/* gcc-provided header: /usr/lib/gcc/.../.../include/cpuid.h */
#include <cpuid.h>

static inline bool
can_sse41(void)
{
	/* For a much more fun example, see:
	 * http://repo.or.cz/w/glibc.git/blob/HEAD:/sysdeps/x86_64/multiarch/init-arch.c
	 * http://repo.or.cz/w/glibc.git/blob/HEAD:/sysdeps/x86_64/multiarch/init-arch.h */

	int a, b, c, d;
	__cpuid (1, a, b, c, d);
	return c & bit_SSE4_1;
}




extern int nifty(int *x, int n);

static int
nifty_lame(int *x, int n)
{
	puts("lame");

	int y = 0;
	for (int i = 0; i < n; i++)
		y += x[i];
	return y;
}

/* In gcc-4.4+ you can adjust targets and optimizations
 * per-function. */
static int __attribute__((target("sse4.1")))
nifty_sse41(int *x, int n)
{
	puts("SSE 4.1");

	int y = 0;
	for (int i = 0; i < n; i++) // gets autovectorized!
		y += x[i];
	return y;
}

__asm__(".type nifty, \%gnu_indirect_function");
extern void *nifty_ifunc(void) __asm__("nifty");

void *nifty_ifunc(void)
{
	puts("deciding");

	/* Use intermediate variable to get a warning for non-matching
	 * prototype. */
	typeof(nifty) *func = can_sse41() ? nifty_sse41 : nifty_lame;
	return func;
}


/* Questions? */


/* Thank you! */


/* (c) 2010  Petr Baudis <pasky@suse.cz>
 * MIT licence */
