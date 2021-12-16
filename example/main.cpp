
#include <iostream>
#include <limits>
#include <chrono>

#include "../utils/server.h"
#include "../headers/waksman.h"
#include "../headers/bitonic.h"
#include "../headers/melbshuffle.h"
#include "../headers/bucket.h"

int main()
{
    unsigned int size = 160000;

    // parameters
    uint32_t block_size = 800;

    // parameters for melbshuffle
    uint32_t p1 = 5, p2 = 5;

    // paramter for Bucket ORP
    uint32_t Z = 512;

    auto *cloud = new server(block_size);

    // create vector
    name_t input_name = 0;
    cloud->create_array(input_name, size);
    for (int i = 0; i < size; ++i) {
        cloud->put(input_name, i, new element(i, 0, nullptr));
    }
    cloud->reset_IO();

    auto t1 = std::chrono::high_resolution_clock::now();
    waksman wak(cloud, size);
    name_t output_name = wak.permute(input_name);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();

    printf("waksman:\nruntime for = %lu\n", duration);
    printf("number of I/0s: %d\n\n", cloud->get_IO());
    cloud->reset_IO();

    // check correctness

    for (int j = 0; j < size; j+=1000) {
        element *e = cloud->get(output_name,j);
        printf("---\nT[%d] = %d\n", j, e->key);
        printf("I[%d] = %d\n", j, wak.get_inv_pi(j));
    }


    t1 = std::chrono::high_resolution_clock::now();
    melbshuffle melb(cloud, size, p1, p2);
    output_name = melb.permute(output_name);
    t2 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();

    printf("melbshuffle:\nruntime for = %lu\n", duration);
    printf("number of I/0s: %d\n\n", cloud->get_IO());
    cloud->reset_IO();

    // check correctness

    for (int j = 0; j < size; j+=1000) {
        element *e = cloud->get(output_name,j);
        printf("---\nT[%d] = %d\n", j, e->key);
        printf("I[%d] = %d\n", j, melb.get_inv_pi(j));
    }


    t1 = std::chrono::high_resolution_clock::now();
    bucket buck(cloud, size, Z);
    output_name = buck.permute(output_name);
    t2 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();

    printf("bucket:\nruntime for = %lu\n", duration);
    printf("number of I/0s: %d\n\n", cloud->get_IO());
    cloud->reset_IO();

    // check correctness

    for (int j = 0; j < size; j+=1000) {
        element *e = cloud->get(output_name,j);
        printf("---\nT[%d] = %d\n", j, e->key);
        printf("I[%d] = %d\n", j, buck.get_inv_pi(j));
    }

    return 0;
}
