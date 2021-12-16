/********************************************************************
 Implementation of the Melbourne Shuffle
 
 Ohrimenko, O., Goodrich, M.T., Tamassia, R. and Upfal, E., 2014, July.
 The Melbourne shuffle: Improving oblivious storage in the cloud.
 
 Created by William Holland on 3/02/21.
 
 MIT License
 Copyright (c) 2021 William Holland
 *********************************************************************/

#include <map>
#include <vector>
#include <assert.h>
#include "../headers/melbshuffle.h"

static bool compare(element *x, element *y) {
    return x->aux < y->aux;
}

name_t melbshuffle::permute(name_t input)
{
    name_t output = input+1;

    // create the temporary arrays and the output array
    cloud->create_array(Ta, (num_chunks)*chunk_width*p1);
    cloud->create_array(Tb, (num_chunks)*chunk_width*p2);
    cloud->create_array(output, size);

    // shuffle the input
    shuffle_pass(input, Ta, Tb, output);

    // delete temporary storage and the input array
    cloud->delete_array(Ta);
    cloud->delete_array(Tb);
    cloud->delete_array(input);

    pi->new_seed();
    // create temporary and output storage for the next shuffle
    cloud->create_array(Tc, (num_chunks)*chunk_width*p1);
    cloud->create_array(Td, (num_chunks)*chunk_width*p2);
    output++;
    cloud->create_array(output, size);

    shuffle_pass(output-1, Tc, Td, output);
    cloud->delete_array(Tc);
    cloud->delete_array(Td);
    cloud->delete_array(output-1);

    return output;
}

void melbshuffle::shuffle_pass(name_t I, name_t T1, name_t T2, name_t O)
{
    distribution_phase_1(I, T1);
    distribution_phase_2(T1, T2);
    cleanup_phase(T2, O);
}

void melbshuffle::distribution_phase_1(name_t I, name_t T)
{
    // a bin refers to the elements of an input bucket that belong to the same output chunk

    // array to store buckets from the input
    element **bucket;
    uint32_t cid, idx = 0;
    // maximum load of a bin
    uint32_t max_load = p1*num_chunks;

    // initialise empty bins (one for each output chunk)
    std::map<uint32_t, std::vector<element*>*> rev_bin;
    for (int id = 0; id < num_chunks; ++id) {
        rev_bin[id] = new std::vector<element*>();
    }

    // iterate through the input buckets, placing elements in the correct output chunks
    for (int id = 0; id < num_buckets; ++id) {
        // determine the length of the input bucket. Only the last bucket can have a different length
        uint32_t range = ((idx + bucket_width) < size) ? bucket_width : (size - idx);
        // retrieve the bucket
        bucket = get_range(I, idx, range);
        element *e;
        // place elements that belong to the same output chunk in the same bin
        for (int i = 0; i < range; ++i) {
            e = bucket[i];
            cid = pi->eval_perm(e->key)/chunk_width; // chunk id
            rev_bin[cid]->push_back(e);
        }

        // push bins to the temporary storage
        std::vector<element*> *vec;
        // calculate the offset for each bin. Each output chunk contains a bin from each input bucket
        uint32_t offset = id*max_load, block_size = num_buckets*max_load;
        for (int i = 0; i < num_chunks; ++i) {
            vec = rev_bin[i];
            assert(vec->size() < max_load);
            put_bin(T, offset, vec, max_load);
            offset += block_size;
        }
        for (int id = 0; id < num_chunks; ++id) {
            rev_bin[id]->clear();
        }
        idx += bucket_width;
        free(bucket);
    }
}

void melbshuffle::distribution_phase_2(name_t T1, name_t T2)
{
    element **bucket;
    uint32_t bid;
    // max load of an input bin and max load of an output bucket
    uint32_t max_load1 = p1*num_chunks, max_load2 = p2*num_chunks;

    std::map<uint32_t, std::vector<element*>*> rev_bin;
    for (int id = 0; id < buckets_per_chunk; ++id) {
        rev_bin[id] = new std::vector<element*>();
    }

    // number of elements (both real and dummy) in a chunk
    uint32_t chunk_card = num_buckets*max_load1;
    // number of input bins retrieved on each iteration
    uint32_t num_bins = ceil((double)num_buckets/(double)buckets_per_chunk);

    // iterate through the chunks
    for (int cid = 0; cid < num_chunks; ++cid) {
        // offset for the next bin
        uint32_t offset_bins = 0, range;
        for (int j = 0; j < buckets_per_chunk; ++j) {

            range = (offset_bins + num_bins < num_buckets) ? num_bins : (num_buckets - offset_bins);
            range *= max_load1;
            // retrieve bucket (segment of bins)
            bucket = get_range(T1, cid*chunk_card + offset_bins*max_load1, range);

            element *e;
            // place elements in the correct bucket in the chunk
            for (int elem = 0; elem < range; ++elem) {
                e = bucket[elem];
                if(e->key != INT32_MAX) {
                    bid = (pi->eval_perm(e->key) / bucket_width);
                    bid %= buckets_per_chunk; // chunk id
                    rev_bin[bid]->push_back(e);
                } else {
                    // delete dummies
                    delete e;
                }
            }

            // push bins into the temporary array
            std::vector<element*> *vec;
            uint32_t offset = cid*max_load2*buckets_per_chunk*buckets_per_chunk + j*max_load2;
            for (bid = 0; bid < buckets_per_chunk; bid++) {
                vec = rev_bin[bid];
                assert(vec->size() < max_load2);
                put_bin(T2, offset, vec, max_load2);
                offset += max_load2*buckets_per_chunk;
            }
            for (int id = 0; id < buckets_per_chunk; ++id) {
                rev_bin[id]->clear();
            }
            free(bucket);
            offset_bins += num_bins;
        }

    }
}

void melbshuffle::cleanup_phase(name_t T, name_t O)
{
    element **block;
    auto *catchment = new std::vector<element*>();
    uint32_t max_load = p2*num_chunks;
    uint32_t offset = 0, t2_bucket_size = buckets_per_chunk*max_load;

    // iterate through the buckets
    for (int id = 0; id < num_buckets; ++id) {
        // retrieve the next bucket
        block = get_range(T, id*t2_bucket_size, t2_bucket_size);

        element *e;
        for (int i = 0; i < t2_bucket_size; ++i) {
            e = block[i];
            if(e->key != INT32_MAX)
            {
                // we will utilize the auxiliary storage for sorting
                e->aux = pi->eval_perm(e->key);
                catchment->push_back(e);
            } else {
                // remove the dummies
                delete e;
            }
        }
        // sort the bucket according to the permutation values
        std::sort(catchment->begin(), catchment->end(), compare);
        // place the bucket in the output array
        put_bucket(O, offset, catchment);

        offset += bucket_width;
        catchment->clear();
        free(block);
    }
}

void melbshuffle::put_bin(name_t T, uint32_t idx, std::vector<element*> *bin, uint32_t max_load)
{
    uint32_t count = 0;
    uint32_t bin_load = bin->size();
    assert(bin_load < max_load);
    // place bin elements in temporary storage
    while(count < bin_load)
    {
        cloud->put(T, idx+count, bin->at(count));
        count++;
    }
    // pad bin to max load with dummies
    for (int i = count; i < max_load; ++i) {
        cloud->put(T, idx+i, new element(INT32_MAX, 0, nullptr));
    }
}

void melbshuffle::put_bucket(name_t O, uint32_t offset, std::vector<element*> *bucket)
{
    // calculate the range of the bucket
    uint32_t range = (offset + bucket_width < size) ? bucket_width : (size - offset);
    element *e;
    for (int i = 0; i < range; ++i)
    {
        e = bucket->at(i);
        e->aux = 0;
        cloud->put(O, offset + i, e);
    }
}

element **melbshuffle::get_range(name_t name, int offset, uint32_t range)
{
    // allocate an array to store the elements
    auto bucket = (element**) calloc(range, sizeof(element*));

    //retrieve the elements
    for (int i = 0; i < range; ++i) {
        bucket[i] = cloud->get(name, offset + i);
    }
    return bucket;
}











