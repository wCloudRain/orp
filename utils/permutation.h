/********************************************************************
 Implementation of a random permutation.
 A large memory implementation that stores a random permutation of
 the array {0,..,n-1}

 Created by William Holland on 1/02/21.
 *********************************************************************/

#ifndef MY_PROJECT_PERMUTATION_H
#define MY_PROJECT_PERMUTATION_H

#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <time.h>
#include <random>
#include <chrono>
#include "../include/murmurhash3.h"

class permutation
{
private:
    uint32_t size;
    uint32_t *perm;
    uint32_t *inv_perm;
    //uint32_t mask_left;
    //uint32_t mask_right;
    //uint32_t seed;
    //uint32_t power;

public:
    explicit permutation(uint32_t size) :
            size(size)
    {
        this->size = size;
        perm = (uint32_t*) calloc(sizeof(uint32_t), size) ;
        inv_perm = (uint32_t*) calloc(sizeof(uint32_t), size);

        // create the array {0,1,...,size-1}
        for (int i = 0; i < size; ++i) {
            perm[i] = i;
        }

        // shuffle the array
        unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
        std::shuffle(&perm[0], &perm[size],  std::default_random_engine(seed));

        // create an array with the inverse permutation
        uint32_t count=0;
        for (int j = 0; j < size; ++j) {
            while(perm[count] != j) {
                count++;
            }
            inv_perm[j] = count;
            count = 0;
        }
        /*
        power = 32-__builtin_clz(size | 1);
        uint32_t mask = (1u << power) - 1;
        mask_right = mask >> (power/2);
        mask_left = mask ^ mask_right;
        seed = rand();
         */
    }

    /**
    Returns the value of the permutation function.
    @param item The key of an item.
    @return The permuted location of the item.
    */
    uint32_t eval_perm(uint32_t item)
    {
        /*
        uint32_t left, right;
        right = mask_right & item;
        left = mask_left & item;

        uint32_t hash;

        MurmurHash3_x86_32( (char *) & right, sizeof(int), seed, &hash);
        left ^= hash & mask_left;
        MurmurHash3_x86_32( (char *) & left, sizeof(int), seed, &hash);
        right ^= hash & mask_right;

        uint32_t value = left | right;
        value %= size;

        return value;
        */
        return perm[item];
    }

    /**
    Returns the value of the inverse permutation function.
    @param item The key of an item.
    @return The inverse permuted location of the item.
    */
    uint32_t eval_inv_perm(uint32_t item)
    {
        /*
        uint32_t left, right;
        right = mask_right & item;
        left = mask_left & item;

        uint32_t hash;
        MurmurHash3_x86_32( (char *) & left, sizeof(int), seed, &hash);
        right ^= hash & mask_right;
        MurmurHash3_x86_32( (char *) & right, sizeof(int), seed, &hash);
        left ^= hash & mask_left;

        return left | right;
        */
        return inv_perm[item];

    }

    uint32_t perm_size() {
        return size;
    }

    /**
    Assigns a new random permutation by randomly shuffling the stored array.
    */
    void new_seed() {
        unsigned seed = rand();
        std::shuffle(&perm[0], &perm[size],  std::default_random_engine(seed));

        uint32_t count=0;
        for (int j = 0; j < size; ++j) {
            while(perm[count] != j) {
                count++;
            }
            inv_perm[j] = count;
            count = 0;
        }
    }
};

#endif //MY_PROJECT_PERMUTATION_H
