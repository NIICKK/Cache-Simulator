#ifdef CCOMPILER
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cstdio>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#endif

#include <unistd.h>
#include "cachesim.hpp"

void print_help_and_exit(void) {
    printf("cachesim [OPTIONS] < traces/file.trace\n");
    printf("-h\t\tThis helpful output\n");
    printf("L1 parameters:\n");
    printf("  -c C1\t\tTotal size in bytes is 2^C1\n");
    printf("  -b B1\t\tSize of each block in bytes is 2^B1\n");
    printf("  -s S1\t\tNumber of blocks per set is 2^S1\n");
    exit(0);
}

void print_statistics(cache_stats_t* p_stats);

int main(int argc, char* argv[]) {
    int opt;
    uint64_t c1 = DEFAULT_C1;
    uint64_t b1 = DEFAULT_B1;
    uint64_t s1 = DEFAULT_S1;

    /* Read arguments */
    while(-1 != (opt = getopt(argc, argv, "c:b:s:v:C:B:S:h"))) {
        switch(opt) {
            case 'c':
                c1 = atoi(optarg);
                break;
            case 'b':
                b1 = atoi(optarg);
                break;
            case 's':
                s1 = atoi(optarg);
                break;
            case 'h':
                /* Fall through */
            default:
                print_help_and_exit();
                break;
        }
    }

    printf("Cache Settings\n");
    printf("c: %" PRIu64 "\n", c1);
    printf("b: %" PRIu64 "\n", b1);
    printf("s: %" PRIu64 "\n", s1);
    printf("\n");

    /* Setup the cache */
    setup_cache(c1, b1, s1);

    /* Begin reading the file */
    char rw;
    uint64_t address;
    while (!feof(stdin)) {
        int ret = fscanf(stdin, "%c %" PRIx64 "\n", &rw, &address);
        if(ret == 2) {
            cache_access(rw, address);
        }
    }

    set_final_stats();
    print_statistics();
    complete_cache();

    return 0;
}

