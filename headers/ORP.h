/********************************************************************
 Virtual class for Oblivious Random Permutation.

 Class is designed to measure algorithm performance. The permute function
 can be modified to take a specific permutation as input.

 Created by William Holland on 1/02/21.
 *********************************************************************/

#ifndef MY_PROJECT_ORP_H
#define MY_PROJECT_ORP_H

#include "../utils/permutation.h"
#include "../utils/server.h"

class ORP
{
protected:
    // permutation function
    permutation *pi;
    // the server that stores the input array
    server *cloud;

public:

    explicit ORP(server *cloud, uint32_t size):
        pi(new permutation(size)),
        cloud(cloud)
    {}

    /**
    Permute array according to pi
    @param input_name The identifier for the array
    */
    virtual name_t permute(name_t input_name) {return 0;}

    /**
    Return the value of the local permutation function

    @param key item identifier
    @return pi(key)
    */
    int get_pi(int key)
    {
        return this->pi->eval_perm(key);
    }

    /**
    Return the value of the local inverse permutation function

    @param key item identifier
    @return pi^{-1}(key)
    */
    int get_inv_pi(int i) {
        return this->pi->eval_inv_perm(i);
    }
};

#endif //MY_PROJECT_ORP_H
