#include "cachesim.hpp"

cache* cacheAddr;
cache_stats_t* statsAddr;
uint64_t c, b, s;

uint64_t findIndex(uint64_t addr, int c, int b, int s){/*checked*/
    return (addr>>b)&(uint64_t(pow(2,c-b-s)-1));;
}

uint64_t findTag(uint64_t addr, int c, int b, int s){/*checked*/
    uint64_t tag;
    tag = addr >> (c - s);
    return tag;
}

/*update block info only when there is a hit*/
bool look_up(uint64_t addr, char mode,const cache* mycache, int c, int b, int s){
    bool hit = false;
    uint64_t index = findIndex(addr,c,b,s);
    uint64_t tag = findTag(addr,c,b,s);

    /*
    std::cout << std::endl <<"addr:" << std::hex << addr <<std::endl;
    std::cout << "index: " << std::hex << index << std::endl;
    std::cout << "tag: " << std::hex << tag << std::endl;
    */

    /*iterate all the sets in the cache and find the corresponding set to the given index*/
    for(auto it = mycache->my_sets->begin(); it!= mycache->my_sets->end(); ++it ){

        //std::cout <<"(*it)->index: " << std::hex << (*it)->index << std::endl;

        /*find the corresponding set or there is only one set (fully associative cache)*/
        if((*it)->index == index || c == b+s){ //if c == b+s, fully associative(no index)

            /*debug code starts
            if(c!= b+s){
                //std::cout << "index found!" << std::endl;
            } else{
                //std::cout << "no index!" << std::endl;
            }
            debug code ends*/

            /*iterate all the blocks in the set and find if there is a tag match*/
            for(auto sth = (*it)->my_blocks->begin(); sth !=  (*it)->my_blocks->end(); ++sth){

                /*determine if the block is filled, an empty block does not have a tag*/
                //std::cout << "valid: "<<(*sth)->valid << std::endl;
                if((*sth)->valid){

                    //if there is a tag match
                    if((*sth)->tag == tag){
                        hit =true;
                        //std::cout << "tag found!" << std::endl;

                        /*update the age for all the blocks in the set*/
                        for(auto chg = (*it)->my_blocks->begin(); chg !=  (*it)->my_blocks->end(); ++chg){

                            //if the block age < the age of accessed block, increment its age
                            if((*chg)->age < (*sth)->age && (*chg)->valid){
                                ++(*chg)->age;
                            }
                        }

                        /*update the accessed block to 1 (most recently used)*/
                        (*sth)->age = 1;

                        /*when mode is write, update the dirty bit to true*/
                        if(mode == 'w'){
                            (*sth)->dirty = true;
                        }

                        return hit;
                    } else{/*haven't got a tag match, keep looping*/}
                }
            }
            break;
        }

    }
    return hit;
}

/*invoke this function when there is a tag miss*/
void LRU_replace(uint64_t addr, char mode){
    uint64_t index = findIndex(addr,c,b,s);
    uint64_t tag = findTag(addr,c,b,s);

    /*iterate all sets in the cache*/
    for(auto it = cacheAddr->my_sets->begin(); it!= cacheAddr->my_sets->end(); ++it) {
        if ((*it)->index == index) {//found the corresponding set

            int oldest = 0;
            auto oldest_tag_iterator = (*it)->my_blocks->begin();
            auto fill_empty_block_iterator = (*it)->my_blocks->begin();
            bool empty = false;

            /*iterate all the blocks in the set*/
            for (auto sth = (*it)->my_blocks->begin(); sth != (*it)->my_blocks->end(); ++sth) {

                    //if there is empty block
                if( !((*sth)->valid) ){
                    empty = true;
                    //record the location for empty block
                    fill_empty_block_iterator = sth;

                    //since we already found an empty block, just exit the my_blocks loop
                    break;
                }
                    //otherwise, update the age of the oldest block and its location
                else if(((*sth)->age) > oldest) {
                    empty = false;
                    oldest = (*sth)->age;
                    oldest_tag_iterator = sth;
                }
            }

            /*modify the corresponding block*/

                // there is at least one empty block
            if(empty){
                //std::cout << "empty!" <<std::endl;
                for (auto sth = (*it)->my_blocks->begin(); sth != (*it)->my_blocks->end(); ++sth) {

                    //increment the age of filled blocks
                    if((*sth)->valid){
                        ++(*sth)->age;
                    }
                }

                /* just accessed empty block*/

                //update the tag
                (*fill_empty_block_iterator)->tag =tag;

                //update the age
                (*fill_empty_block_iterator)->age = 1;

                //update the valid bit, turn valid to true since it is filled now
                (*fill_empty_block_iterator)->valid =true;

                /*if we are writing to a cache*/
                if(mode== 'w'){
                    (*fill_empty_block_iterator)->dirty = true;
                    //std::cout << "dirty!" <<std::endl;
                } else if(mode == 'r'){
                    (*fill_empty_block_iterator)->dirty = false;
                }
            }
                // there is no empty block, we need to replace the LRU
            else{

                //std::cout << "replacement happen!" << std::endl;
                /*
                //print the tag of the replaced block
                std::cout << "tag: "<<std::hex<< (*oldest_tag_iterator)->tag << std::endl;
                */

                //increment the age for all blocks
                for (auto sth = (*it)->my_blocks->begin(); sth != (*it)->my_blocks->end(); ++sth){
                    ++(*sth)->age;
                }

                //assign the new tag replace by the LRU tag
                (*oldest_tag_iterator)->tag = tag;
                //update the age of newly filled block to 1 (most recently used)
                (*oldest_tag_iterator)->age = 1;

                /*if the dirty bit(is true) of the replaced, then ++write_back*/
                if((*oldest_tag_iterator)->dirty){
                    ++statsAddr->write_back_l1;
                }

                if(mode == 'w'){
                    (*oldest_tag_iterator)->dirty = true;
                } else if(mode == 'r'){
                    (*oldest_tag_iterator)->dirty = false;
                }
            }

            /*exit the my_sets loop*/
            break;
        }
    }
}
void set_final_stats(){
    statsAddr->total_hit_ratio = (double)(statsAddr->total_hits_l1)/(statsAddr->accesses);
    statsAddr->total_miss_ratio = (double)(statsAddr->total_misses_l1)/(statsAddr->accesses);

    statsAddr->read_hit_ratio = (double)(statsAddr->read_hits_l1)/(statsAddr->reads);
    statsAddr->read_miss_ratio = (double)(statsAddr->read_misses_l1)/(statsAddr->reads);

    statsAddr->write_hit_ratio = (double)(statsAddr->write_hits_l1)/(statsAddr->writes);
    statsAddr->write_miss_ratio = (double)(statsAddr->write_misses_l1)/(statsAddr->writes);

    double HT = 2+0.2*s;
    double MP = 20;
    statsAddr->avg_access_time_l1 = (
                                            (statsAddr->reads)*HT + (statsAddr->read_misses_l1)*MP
            + (statsAddr->writes)*HT + (statsAddr->write_misses_l1)*MP
                                    )/
            (statsAddr->reads +statsAddr->writes);
}

void print_statistics() {
    printf("Cache Statistics\n");
    printf("Accesses: %" PRIu64 "\n", statsAddr->accesses); //
    printf("Total hits: %" PRIu64 "\n", statsAddr->total_hits_l1); //
    printf("Total misses: %" PRIu64 "\n", statsAddr->total_misses_l1); //

    printf("Hit ratio for L1: %.3f\n", statsAddr->total_hit_ratio);//
    printf("Miss ratio for L1: %.3f\n", statsAddr->total_miss_ratio);//

    printf("Reads: %" PRIu64 "\n", statsAddr->reads);//
    printf("Read hits to L1: %" PRIu64 "\n", statsAddr->read_hits_l1);//
    printf("Read misses to L1: %" PRIu64 "\n", statsAddr->read_misses_l1);//

    printf("Read hit ratio for L1: %.3f\n", statsAddr->read_hit_ratio);//
    printf("Read miss ratio for L1: %.3f\n", statsAddr->read_miss_ratio);//

    printf("Writes: %" PRIu64 "\n", statsAddr->writes);//
    printf("Write hits to L1: %" PRIu64 "\n", statsAddr->write_hits_l1);//
    printf("Write misses to L1: %" PRIu64 "\n", statsAddr->write_misses_l1);//

    printf("Write backs from L1: %" PRIu64 "\n", statsAddr->write_back_l1);//

    printf("Write hit ratio for L1: %.3f\n", statsAddr->write_hit_ratio);//
    printf("Write miss ratio for L1: %.3f\n", statsAddr->write_miss_ratio);//

    printf("Average access time (AAT) for L1: %.3f\n", statsAddr->avg_access_time_l1);
}

void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1){
    /*initialize global variables: c,b,s*/
    c =  c1;
    b =  b1;
    s =  s1;

    /*create cache object and assign the global variable cacheAddr*/
    cache *mycache = new cache(c1,b1,s1);
    cacheAddr = mycache;

    /*create cache_stats_t object and assign the global variable statsAddr*/
    cache_stats_t *mystats = new cache_stats_t();
    statsAddr = mystats;

    /*build the empty cache*/
    unsigned numofsets = pow(2, c1-b1-s1);
    unsigned blocksperset = pow(2, s1);
    for(size_t j = 0; j < numofsets; ++j){
        //initialize all cacheset objects for the cache
        cacheset* mysets = new cacheset(j);
        (cacheAddr->my_sets)->push_back(mysets);

        for(size_t k = 0; k < blocksperset; ++k){
            //initialize the cacheblock objects for each cacheset object
            cacheblock* myblock = new cacheblock();
            (mysets->my_blocks)->push_back(myblock);
        }
    }
}

void cache_access(char type, uint64_t arg) {
    ++(statsAddr->accesses);//increment accesses
    switch(type){
        case 'r'://cache reads
            ++statsAddr->reads;//increment reads
            if(look_up(arg,'r',cacheAddr,c,b,s)){//read hit
                ++statsAddr->read_hits_l1;
                ++statsAddr->total_hits_l1;
                std::cout << "H" << std::endl;

            } else{// read miss
                ++statsAddr->read_misses_l1;
                ++statsAddr->total_misses_l1;
                std::cout << "M" << std::endl;

                /*if miss: (invoke LRUreplace())
                 * 1. if is empty block (age == 0), place address into the empty block
                 * 2. else place the address into the LRU block
                 * */
                LRU_replace(arg,'r');
            }
            break;

        case 'w'://cache writes
            ++statsAddr->writes;//increment writes
            if(look_up(arg, 'w',cacheAddr,c,b,s)){//write hit
                ++statsAddr->write_hits_l1;
                ++statsAddr->total_hits_l1;
                std::cout << "H" << std::endl;
                //implement algorithm to let corresponding block's dirty = true;

            } else{//write miss
                ++statsAddr->write_misses_l1;
                ++statsAddr->total_misses_l1;
                std::cout << "M" << std::endl;

                //replace the LRU and implement algorithm to let corresponding block's dirty = true;
                LRU_replace(arg, 'w');
            }

            break;
    }
}

void complete_cache(){
    //deallocate heap for my_sets
    for(auto it = cacheAddr->my_sets->begin(); it!= cacheAddr->my_sets->end(); ++it ){
        (*it)->~cacheset();
    }

    //deallocate heap for mycache
    cacheAddr->~cache();
    delete statsAddr;
}
