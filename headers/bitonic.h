//
// Created by wholland on 2/02/21.
//

#ifndef MY_PROJECT_BITONIC_H
#define MY_PROJECT_BITONIC_H

#include "../utils/server.h"
#include "../utils/permutation.h"
#include "ORP.h"

class bitonic : public ORP
{
private:
    uint32_t size;
public:
    explicit bitonic(server *cloud, uint32_t size):
        ORP(cloud, size),
        size(size)
    {}

    name_t permute(name_t arr) override;
};

#endif //MY_PROJECT_BITONIC_H
