//
// Created by wholland on 21/07/21.
//

#ifndef MY_PROJECT_BUCKET_H
#define MY_PROJECT_BUCKET_H

#include "../utils/server.h"
#include "../utils/permutation.h"
#include "ORP.h"

class bucket : public ORP
{
private:
    uint32_t size;
    // security parameter
    uint32_t Z;
    uint32_t B;
    uint32_t seed;
public:
    explicit bucket(server *cloud, uint32_t power, uint32_t Z):
            ORP(cloud, power),
            size(power),
            Z(Z),
            seed(rand()),
            B(0)
    {}

    name_t permute(name_t arr) override;

    /**
    Elements are assigned to buckets with a hash function.
    Elements are routed into output buckets through a butterfly network.
    @param arr The identifier for the input array.
    @return The length of the output array.
    */
    name_t butterfly(name_t arr);

    /**
    Non-oblivious permutation function.
    @param arr The identifier for the input array.
    @return The length of the output array.
    */
    name_t rearrange(name_t arr);

    /**
    Retrieves a bucket from the server.
    @param arr The identifier for the input array.
    @param width The width of the bucket.
    @param offset The index in the source array.
    @param buck Container to place the real elements of the bucket.
    */
    void get_bucket(name_t arr, uint32_t offset, uint32_t index, std::vector<element *> *buck);

    /**
    Places a bucket of real and dummy elements at the server
    @param arr The identifier for the array
    @param offset The index in the destination array
    @param buck Container to place the real elements of the bucket.
    */
    void put_bucket(name_t arr, uint32_t offset, std::vector<element *> *buck);

    /**
    Splits an input bucket into two buckets based on permutation tags. The larger tags go in the
     right bucket.
    @param input The input bucket.
    @param out_right The right output bucket.
    @param out_left The left output bucket.
    @param level The current level in the network.
    */
    void split_input_bucket(std::vector<element *> *input,
            std::vector<element *> *out_right,
            std::vector<element *> *out_left,
            uint32_t level);

    /**
    Completes the final round of the butterfly network.
    Dummy elements are removed from buckets and the buckets are shuffled.
    @param left The left input bucket.
    @param right The right input bucket.
    @param arr The identifier for the input array.
    @param count The number of real elements placed in the output.
    */
    uint32_t final_round(std::vector<element *> *left, std::vector<element *> *right,
            name_t arr, uint32_t count);
};

#endif //MY_PROJECT_BUCKET_H
