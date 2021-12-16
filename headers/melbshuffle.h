
/********************************************************************
 Implementation of the Melbourne Shuffle

 Ohrimenko, O., Goodrich, M.T., Tamassia, R. and Upfal, E., 2014, July.
 The Melbourne shuffle: Improving oblivious storage in the cloud.

 Created by William Holland on 3/02/21.
 *********************************************************************/

#ifndef MY_PROJECT_MELBSHUFFLE_H
#define MY_PROJECT_MELBSHUFFLE_H

#include <cmath>
#include <algorithm>
#include "../utils/permutation.h"
#include "../utils/server.h"
#include "ORP.h"

// Identifiers for temporary arrays
#define Ta 0x10000000
#define Tb 0x10000001
#define Tc 0x10000002
#define Td 0x10000003

class melbshuffle : public ORP
{
private:
    uint32_t p1;
    uint32_t p2;
    uint32_t size;
    // array is broken into buckets and a segment of buckets is a chunk
    uint32_t num_chunks;
    uint32_t num_buckets;
    uint32_t buckets_per_chunk;
    uint32_t bucket_width;
    uint32_t chunk_width;

    /**
    Performs a single shuffle of the input array.
    As not all permutations are possible, the main function permute
    executes shuffle_pass twice
    @param I The identifier for the input array
    @param T1 The identifier for the first temporary array
    @param T2 The identifier for the second temporary array
    @param O The identifier for the output array
    */
    void shuffle_pass(name_t I, name_t T1, name_t T2, name_t O);

    /**
    The input array is divided in buckets and chunks of buckets.
    The first distribution phase places all elements in the correct chunks in the output.
    Elements are placed in a temporary array and the chunks are padded with dummies so that they
     have equal cardinality.
    @param I The identifier for the input array
    @param T1 The identifier for the temporary array
    */
    void distribution_phase_1(name_t I, name_t T1);

    /**
    The input temporary array contains elements in the correct chunk
    The second distribution phase places all elements in a chunk in the correct bucket.
    Elements are placed in a temporary array and the chunks are padded with dummies so that they
     have equal cardinality.
    @param T1 The identifier for the input temporary array
    @param T2 The identifier for the output temporary array
    */
    void distribution_phase_2(name_t T1, name_t T2);

    /**
    The input temporary array contains elements in the correct bucket (with dummies) but not
     in the correct order.
    The clean up phase places retrieves each bucket and places elements in the correct order
    @param T The identifier for the input temporary array
    @param O The identifier for the output array
   */
    void cleanup_phase(name_t T, name_t O);

    /**
    Places a bin in temporary storage. The bin is padded with dummies to have cardinality max_load.
    Padding ensures that the bin loads are not revealed to the server.
    @param name The identifier for the temporary output array
    @param idx The starting index of the bin in the temporary array
    @param bin The real elements in the bin
    @param max_load The cardinality of the bin.
   */
    void put_bin(name_t T, uint32_t idx, std::vector<element*> *bin, uint32_t max_load);

    /**
    Places a bucket (correctly ordered) in the output array
    @param O The identifier for the output array
    @param idx The starting index of the bucket in the output array
    @param bucket The real elements in the bucket
   */
    void put_bucket(name_t O, uint32_t offset, std::vector<element*> *bucket);

    /**
    Retrieves a contiguous segment of elements from an external array
    @param name The identifier for the array
    @param idx The starting index of the segment
    @param range the length of the segment
    @return the array name[idx...(range-1)]
    */
    element **get_range(name_t name, int idx, uint32_t range);

public:
    explicit melbshuffle(server *cloud, uint32_t size, uint32_t p1, uint32_t p2):
            ORP(cloud, size),
            size(size),
            p1(p1),
            p2(p2)
    {
        printf("size: %d\n", size);
        this->num_buckets = (uint32_t) ceil(sqrt(size));
        this->bucket_width = (uint32_t) ceil(sqrt(size));
        if(bucket_width*num_buckets - bucket_width >= size) {
            bucket_width--;
        }
        this->num_chunks = (uint32_t) ceil(pow(size, 0.25));
        this->buckets_per_chunk = ceil((double) num_buckets / (double) num_chunks);
        this->chunk_width = buckets_per_chunk * bucket_width;
    }

    name_t permute(name_t input) override;
};

#endif //MY_PROJECT_MELBSHUFFLE_H
