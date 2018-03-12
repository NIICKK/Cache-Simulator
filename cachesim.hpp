#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#ifdef CCOMPILER
#include <stdint.h>
#else
#include <cstdint>
#endif

#include <vector>
#include <iostream>
#include <math.h>
#include <inttypes.h>


struct cache_stats_t {
    uint64_t accesses;
    uint64_t reads;
    uint64_t read_hits_l1;
    uint64_t read_misses_l1;
    uint64_t writes;
    uint64_t write_hits_l1;
    uint64_t write_misses_l1;
    uint64_t write_back_l1;
    uint64_t total_hits_l1;
    uint64_t total_misses_l1;
    double total_hit_ratio;
    double total_miss_ratio;
    double read_hit_ratio;
    double read_miss_ratio;
    double write_hit_ratio;
    double write_miss_ratio;
    double avg_access_time_l1;
};

struct cacheblock{
    uint64_t tag;
    bool valid;
    bool dirty;
    int age;

    //constructor
    cacheblock()
            :valid(false),
             dirty(false),
             age(0) {}
};

struct cacheset{
    uint64_t index;
    std::vector<cacheblock*>* my_blocks;

    //constructor
    cacheset(uint64_t index)
            :index(index)
    {
        my_blocks = new std::vector<cacheblock*>;
    }

    ~cacheset(){
        //delete[] my_blocks;
    }
};

struct cache{
    int numSets;
    int numBlocks;
    std::vector<cacheset*>* my_sets;

    //constructor
    cache(int c, int b, int s)
            :numSets(pow(2,c-b-s)),
             numBlocks(pow(2,c-b))
    {
        my_sets = new std::vector<cacheset*>;
    }


    ~cache(){
        //delete[] my_sets;
    }

};

uint64_t findIndex(uint64_t addr, int c, int b, int s);
uint64_t findTag(uint64_t addr, int c, int b, int s);
bool look_up(uint64_t addr,char mode, const cache* mycache, int c, int b, int s);
void LRU_replace(uint64_t addr, char mode);
void set_final_stats();
void print_statistics();

void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1);
void cache_access(char type, uint64_t arg);
void complete_cache();

static const uint64_t DEFAULT_C1 = 12;   /* 4KB Cache */
static const uint64_t DEFAULT_B1 = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S1 = 3;    /* 8 blocks per set */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_HPP */
