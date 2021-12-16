#include <random>

/********************************************************************
 Implementation of Bucket Oblivious Permutation

 Asharov, G., Chan, T.H., Nayak, K., Pass, R., Ren, L. and Shi, E., 2020.
 Bucket oblivious sort: An extremely simple oblivious sort.

 Created by William Holland on 21/07/21.
 
 MIT License
 Copyright (c) 2021 William Holland
 *********************************************************************/

#include <tgmath.h>
#include <vector>
#include <algorithm>
#include "../headers/bucket.h"

name_t bucket::permute(name_t arr)
{
    seed+=2;
    arr = butterfly(arr);
    // rearrange (non-obliviously) according to the input permutation.
    arr = rearrange(arr);
    return arr;
}

name_t bucket::butterfly(name_t arr)
{
    // largest power of two larger than 2n/Z
    B = ceil(2*size/(double)Z);
    uint32_t zeros = __builtin_clz(B);
    uint32_t msb = 32 - zeros;
    // check if B is a power of two
    if((B & (B - 1)) != 0) {
        B = 1u << msb;
    }

    // for the first split, the input array has no dummies
    auto in_left = new std::vector<element *>(),
            in_right = new std::vector<element *>(),
            out_left = new std::vector<element *>(),
            out_right = new std::vector<element *>();

    uint32_t width, jprime, count = 0;
    for (uint32_t i = 0; i < msb-1; ++i) {
        cloud->create_array(arr+1, B*Z);
        for (int j = 0; j < B/2; ++j) {

            if (i == 0) {
                // first round the input array has no dummies
                width = Z/2;
            } else {
                width = Z;
            }
            jprime = j / (1u << i) * (1u << (i));
            // get buckets from the server
            get_bucket(arr, width, (j + jprime) * width, in_left);
            get_bucket(arr, width, (j + jprime + (1u << i)) * width, in_right);

            // split two buckets according to random tags
            split_input_bucket(in_left, out_right, out_left, i);
            split_input_bucket(in_right, out_right, out_left, i);

            if(i == (msb -2))
            {
                // dummies need to be removed and buckets shuffled
                count = final_round(out_left, out_right, arr+1, count);
            } else {
                // place buckets on the server
                put_bucket(arr+1, 2*j*Z, out_left);
                put_bucket(arr+1, (2*j+1)*Z, out_right);
            }
        }
        // increment array
        cloud->delete_array(arr);
        arr++;
    }
    return arr;
}

uint32_t bucket::final_round(std::vector<element *> *left, std::vector<element *> *right,
        name_t arr, uint32_t count) {
    element *e;
    // after dummies are removed, randomly shuffle the buckets before placing at the server
    std::shuffle(left->begin(), left->end(), std::mt19937(std::random_device()()));
    std::shuffle(right->begin(), right->end(), std::mt19937(std::random_device()()));

    uint32_t card = left->size();
    // upload real elements
    for (int i = 0; i < card; ++i) {
        e = left->at(i);
        cloud->put(arr, count+i, e);
    }
    count += card;
    card = right->size();
    // upload real elements
    for (int i = 0; i < card; ++i) {
        e = right->at(i);
        cloud->put(arr, count+i, e);
    }
    count+=card;
    left->clear();
    right->clear();
    return count;
}

name_t bucket::rearrange(name_t arr)
{
    // non-oblivious algorithm for applying a permutation to an array
    cloud->create_array(arr+1, size);
    element *e;
    uint32_t index;
    for (int i = 0; i < size; ++i) {
        e = cloud->get(arr, i);
        index = pi->eval_perm(e->key);
        cloud->put(arr+1, index, e);
    }
    // increment array
    cloud->delete_array(arr);
    arr++;
    return arr;
}

void bucket::get_bucket(name_t arr, uint32_t width, uint32_t offset, std::vector<element *> *buck)
{
    buck->clear();

    uint32_t length = B*Z;
    element* e;
    // get bucket from the server
    for (int i = offset; i < width + offset && i < length; ++i) {
        if(cloud->check(arr, i)) {
            e = cloud->get(arr, i);
            if(e->key != INT32_MAX)
            {
                buck->push_back(e);
            } else {
                // remove dummies
                delete e;
            }
        }
    }
}

void bucket::put_bucket(name_t arr, uint32_t offset, std::vector<element *> *buck)
{
    element *e;
    uint32_t card = buck->size();

    // check if bucket overflows
    if (card > Z) {
        printf("overflow ABORT\n");
        exit(1);
    }
    // upload real elements
    for (int i = 0; i <  card; ++i) {
        e = buck->at(i);
        cloud->put(arr, offset+i, e);
    }
    // upload dummy elements
    for (int i =  card; i < Z; ++i) {
        cloud->put(arr, offset+i, new element(INT32_MAX, 0, nullptr));
    }
    buck->clear();
}

void bucket::split_input_bucket(std::vector<element *> *input, std::vector<element *> *out_right,
                                std::vector<element *> *out_left, uint32_t i) {
    uint32_t tag;
    // split the input into two buckets based on permutation tags
    for( element *e : *input) {
        // check if dummy
        if(e->key != UINT32_MAX) {
            // get random tag
            MurmurHash3_x86_32((char *) &e->key, sizeof(int), seed, &tag);
            tag %= B;
            if (tag & (1u << i)) {
                out_right->push_back(e);
            } else {
                out_left->push_back(e);
            }
        }
    }
}











