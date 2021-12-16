/********************************************************************
 Implementation of the Bitonic Sorting Network

 Nassimi, D. and Sahni, S., 1979.
 Bitonic sort on a mesh-connected parallel computer.

 Created by William Holland on 02/02/21.
 
 MIT License
 Copyright (c) 2021 William Holland
 *********************************************************************/

#include "../headers/bitonic.h"
#include "../include/murmurhash3.h"

name_t bitonic::permute(name_t arr)
{
    //pi->new_seed();
    uint32_t i,j,k,l;
    element *el, *ek;
    uint32_t randk, randl;
    for (i = 2; i <= size; i*=2) {
        for (j = i/2; j > 0 ; j/=2) {
            for (k = 0; k < size; ++k) {
                l = k^j;
                if(l > k) {
                    // retrieve elements
                    ek = cloud->get(arr, k);
                    el = cloud->get(arr, l);
                    // hash the keys
                    randk = pi->eval_perm(ek->key);
                    randl = pi->eval_perm(el->key);
                    // route according to the order of the hash values
                    if( (((i & k) == 0) && (randk > randl))
                        || (((i & k) != 0) && (randk < randl))) {
                        // swap the elements
                        cloud->put(arr, k, el);
                        cloud->put(arr, l, ek);
                    } else {
                        cloud->put(arr, k, ek);
                        cloud->put(arr, l, el);
                    }
                }
            }
        }
    }
    return arr;
}

