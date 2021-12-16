/********************************************************************
 Implementation of a simulated client server environment.
 The server stores arrays on disk.
 Each array has an identifier and the client can manipulate the array
 items using the interface of the server

 Simulation is designed to measure performance in a client-server protocol.

 Created by William Holland on 1/02/21.
 *********************************************************************/
#ifndef MY_PROJECT_SERVER_H
#define MY_PROJECT_SERVER_H

#include <cstdint>
#include <iostream>
#include <fstream>
#include <tr1/unordered_map>
#include <assert.h>

#define BYTESPERELEM 9

typedef uint32_t name_t;

/**
    Structure of elements stored at the server.
    Each element has a key and a value and can store auxiliary information.
    Server retrieves the value and passes the client a pointer to the object.
    The size of the value is a parameter (the block size)
*/
struct element
{
    uint32_t key;
    uint32_t aux;
    uint32_t *value;

    explicit element(uint32_t k, uint32_t a, uint32_t *value):
            key(k),
            aux(a),
            value(value)
    {}

    ~element()
    {
        free(value);
    }
};

/**
    Structure of an array stored on disk
*/
struct disk_array
{
    std::ofstream *out;
    std::ifstream *in;
    uint32_t length;

    explicit disk_array(std::string const& filename, uint32_t length)
    {
        this->length = length;
        out = new std::ofstream(filename, std::ios::binary);
        in = new std::ifstream(filename, std::ios::binary); // she/him
    }
};

/**
    Structure a simulated server. The size of blocks stored at the server is
    a parameter. As blocks are stored on disk, this allows the simulated to
    measure the amount of memory a client uses in a client-server protocol.
*/
class server
{
private:
    uint32_t num_IO;
    uint32_t block_size;
    // map array IDs to arrays on disk
    std::tr1::unordered_map<name_t, disk_array*> table;

public:
    explicit server(uint32_t block_size):
            num_IO(0),
            block_size(block_size),
            table()
    {}

    /**
    Creates a new array at the server
    @param name The identifier for the array
    @param length The length of the stored array
    */
    void create_array(uint32_t name, uint32_t length)
    {
        // create a new file
        std::string filename = "file" + std::to_string(name) + ".dat";
        auto *f = new disk_array(filename, length);
        // ad (ID, file) to the server map
        table[name] = f;
    }

    /**
    Retrieves an element from a specified array and index at the server
    @param name The identifier for the array
    @param index The index of the element in the array
    @return An element at name[index]
    */
    element *get(uint32_t name, uint32_t index)
    {
        // count the number of IOs between server and client
        num_IO++;
        uint64_t output;
        uint32_t key, aux, *value;

        // get file handler
        disk_array *array = table[name];
        std::ifstream *f = array->in;

        // locate element in file and retrieve
        uint32_t file_idx = index*BYTESPERELEM;
        f->seekg(file_idx, std::ios::beg);
        f->read(reinterpret_cast<char*>(&output), sizeof(output));

        key = output & (uint64_t) INT32_MAX;
        aux = (uint32_t) (output >> 32u);
        // Allocate a block of block_size bits in the clients memory
        // The value is blank and used for simulating client memory
        value = (uint32_t*) calloc(block_size/32, sizeof(int));

        return new element(key, aux, value);
    }

    /**
    Places an element at a specified array and index at the server
    @param name The identifier for the array
    @param length The index of the element in the array
    */
    void put(uint32_t name, uint32_t index, element *x)
    {
        num_IO++;
        disk_array *array = table[name];
        std::ostream *f = array->out;
        uint64_t packet = x->aux;
        packet <<= 32u;
        packet |= (uint64_t) x->key;

        // locate index at the sever
        uint32_t file_idx = index*BYTESPERELEM;
        f->seekp(file_idx, std::ios::beg);
        f->write((char*) &packet, sizeof(packet));
        (*f) << std::endl;
        f->flush();

        delete x;
    }

    /**
    Resets the count of IOs between server and client
    */
    void reset_IO() {num_IO = 0;}

    /**
    @return the count of IOs between server and client
    */
    uint32_t get_IO() { return num_IO;}

    /**
    Delete an array from the server
    @param i the name of the array to be deleted
    */
    void delete_array(name_t i)
    {
        disk_array *arr = table[i];
        arr->out->close();
        arr->in->close();
        delete arr;
        table.erase(i);
    }

    /**
    Check if the array exists at the server and that the index is
     within the range of the array.
    @param name The identifier for the array
    @param index The index of the element in the array
    */
    bool check(name_t name, int index) {
        // get file handler
        disk_array *array = table[name];
        return index < array->length;
    }
};

#endif //MY_PROJECT_SERVER_H