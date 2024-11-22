#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>

#define N 2
#define M 100
#include <time.h>


static volatile int __attribute__ ((aligned (128))) a = 42;
static volatile int __attribute__ ((aligned (128))) c1 = 1;
static volatile int __attribute__ ((aligned (128))) c2 = 0;

static volatile int* cc1 = &c1;
static volatile int* cc2 = &c2;

#pragma GCC push_options
#pragma GCC optimize ("O1")
int __attribute__ ((noinline)) withspecKind1(int* a,volatile int** c){
    int v;
    if (**c) {
        __asm__ volatile(
		"serialize\n"
//              "lfence\n"
//              "rdtsc\n"
//              "addq %%rax, %%rdi\n"
//		"subq %%rax, %%rdi\n"
                "movq (%%rdi), %%rax\n"
                : "=a" (v)
                :
                : "memory"
                );
        return v;
    }

    return 0;
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize ("O1")
int __attribute__ ((noinline)) withspecKind2(int* a,volatile int** c){
    int v;

    if (**c) {
        __asm__ volatile(
                "movq (%%rdi), %%rax\n"
                : "=a" (v)
                : 
                : "memory"
                );
        return v;
    }

    return 0;
}
#pragma GCC pop_options

void test_withspecKind1() {
    uint64_t start=0;
    uint64_t end=0;
    uint64_t total_elapsed=0;
        // Train the branch predictor
        for (int j = 0; j < M; j++) {
            withspecKind1(&a, &cc1);
        }

	// Remove everything of interest from the cache
        __asm__ volatile (
            "clflush (%0)\n"
            "clflush (%1)\n"
            "clflush (%2)\n"
            "clflush (%3)\n" 
            "clflush (%4)\n"
            "lfence\n"
            "sfence\n"
            "mfence\n"
	    "serialize\n" :: "r"(&a),"r"(&c1),"r"(&c2),"r"(&cc2),"r"(&cc1)
        );


        withspecKind1(&a, &cc2);


        // Start timer
        __asm__ volatile (
            "lfence\n"
            "sfence\n"
            "mfence\n"
            "rdtscp\n"
            : "=A" (start)
        );
        // Load a
        __asm__ volatile (
            "mov (%0), %%rdx"
            :
            : "r" (&a)
            : "rdx"
        );
        // Stop timer
        __asm__ volatile (
            "lfence\n"
            "rdtscp\n"
            : "=A" (end)
        );

	if (end - start < 1000) {
		// return 0 for total elapsed time if we got interrupted by
		// system in the middle of our measurement.
	        total_elapsed += end -start;
	}

    printf(" %llu ", total_elapsed);
   }

void test_withspecKind2() {
    uint64_t start=0;
    uint64_t end=0;
    uint64_t total_elapsed=0;
 
        for (int j = 0; j < M; j++) {
            withspecKind2(&a, &cc1);
        }
        // Remove a from the cache
        __asm__ volatile (
            "clflush (%0)\n"
            "clflush (%1)\n"
            "clflush (%2)\n"
            "clflush (%3)\n" 
            "clflush (%4)\n" :: "r"(&a),"r"(&c1),"r"(&c2),"r"(&cc2),"r"(&cc1)
        );
       __asm__ volatile (
            "lfence\n"
            "sfence\n"
            "mfence\n"
	    "serialize\n"
            :
        );



        // We expect a branch misprediction here.
        // if rdpkru is speculated, then a should be loaded in the caches
//        f(&cc1);
        withspecKind2(&a, &cc2);


        // Start timer
        __asm__ volatile (
            "lfence\n"
            "sfence\n"
            "mfence\n"
            "rdtscp\n"
            : "=A" (start)
        );
        // Load a
        __asm__ volatile (
            "mov (%0), %%rdx"
            :
            : "r" (&a)
            : "rdx"
        );
        // Stop timer
        __asm__ volatile (
            "lfence\n"
            "rdtscp\n"
            : "=A" (end)
        );

	if (end - start < 1000) {
		// return 0 for total elapsed time if we got interrupted by
		// system in the middle of our measurement.
	        total_elapsed += end -start;
	}

    printf(" %llu ", total_elapsed);
}

void ref(int incache){
	// Reference call, to see how long the access if it is/it is not in the
	// cache

	uint64_t start=0;
	uint64_t end=0;
	uint64_t total_elapsed=0;
        __asm__ volatile (
            "clflush (%0)\n"
            "clflush (%1)\n"
            "clflush (%2)\n"
            "clflush (%3)\n" 
            "clflush (%4)\n" :: "r"(&a),"r"(&c1),"r"(&c2),"r"(&cc2),"r"(&cc1)
        );
       __asm__ volatile (
            "lfence\n"
            "sfence\n"
            "mfence\n"
	    "serialize\n"
            :
        );


	if (incache) {
		__asm__ volatile (
            "mov (%0), %%rdx"
            :
            : "r" (&a)
            : "rdx"
        );
	}
 

        // Start timer
        __asm__ volatile (
            "lfence\n"
            "sfence\n"
            "mfence\n"
            "rdtscp\n"
            : "=A" (start)
        );
        // Load a
        __asm__ volatile (
            "mov (%0), %%rdx"
            :
            : "r" (&a)
            : "rdx"
        );
        // Stop timer
        __asm__ volatile (
            "lfence\n"
            "rdtscp\n"
            : "=A" (end)
        );

	if (end - start < 1000) {
	        total_elapsed += end -start;
	}
    printf(" %llu ", total_elapsed);
}



int main() {
	printf("{ \"kind1\": ");
	test_withspecKind1();
	printf(", \"kind2\": ");
	test_withspecKind2();
	printf(", \"refInCache\": ");
	ref(1);
	printf(", \"refOutCache\": ");
	ref(0);
	printf("}\n");
	return 0;
}
