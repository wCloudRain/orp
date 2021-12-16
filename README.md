Implementations of Oblivious Permutation (OP) algorithms.

The code was used to evaluate the performance of OP algorithms in the following publication:

    William Holland, Olga Ohrimenko, and Anthony Wirth.
    Efficient Oblivious Permutation via the Waksman Network.
    2022 ACM Asia Conference on Computer and Communications Security (ASIACCS ’22)



## Oblivious Permutation Algorithms

Implemented algorithms include:

1. Bitonic Sorting network:

        Nassimi, David, and Sartaj Sahni.
        Bitonic sort on a mesh-connected parallel computer.
        IEEE Transactions on Computers 28, no. 01 (1979): 2-7.

2. Bucket Oblivious Permutation:

        Asharov, Gilad, TH Hubert Chan, Kartik Nayak, Rafael Pass, Ling Ren, and Elaine Shi.
        Bucket oblivious sort: An extremely simple oblivious sort.
        In Symposium on Simplicity in Algorithms, pp. 8-14. Society for Industrial and Applied Mathematics, 2020.

3. The Melbourne Shuffle:

        Ohrimenko, Olga, Michael T. Goodrich, Roberto Tamassia, and Eli Upfal.
        The Melbourne shuffle: Improving oblivious storage in the cloud.
        In International Colloquium on Automata, Languages, and Programming, pp. 556-567. Springer, Berlin, Heidelberg, 2014.

4. A low memory oblivious routing algorithm for the Waksman network [Holland et al. ASIACCS '22].

## Example 

The example/main.cpp file provides an example of how to set parameters and execute the algorithms. First a server needs to be initialised. Then an array (to be permuted) is created and filled with keys. The array can be used as input to the 'permute' for each class of OP algorithms.


## Disclaimer 

The code is used purely for research and proof of concept. We provide no security guarantees with the use of the code.


## Citation 
If you use any of the code found in this repo, please use the following citation:

    @article{holland22,
      title={Efficient Oblivious Permutation via the Waksman Network.},
      author={William Holland, Olga Ohrimenko, and Anthony Wirth},
      journal={2022 ACM Asia Conference on Computer and Communications Security (ASIACCS ’22)},
      year={2022}
    }


## License 

This code is released under MIT, as found in the LICENSE file.

