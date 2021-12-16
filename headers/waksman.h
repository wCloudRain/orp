
//
// Created by wholland on 1/02/21.
//

#ifndef MY_PROJECT_WAKSMAN_H
#define MY_PROJECT_WAKSMAN_H

#include <stdint-gcc.h>
#include <climits>
#include <bitset>
#include <vector>
#include "../utils/permutation.h"
#include "../utils/server.h"
#include "ORP.h"

#define PERSIST true
#define SWAP false

#define EVEN 0
#define ODD 1

typedef uint32_t name_t;
using namespace std;
#define clz(x) __builtin_clz(x)

typedef std::vector<bool> bitvector;

/**
    Structure for handling data during the execution of set exterior
*/
struct ext_data
{
    uint32_t cur;
    uint32_t tar = 0;
    uint32_t cur_setting;

    explicit ext_data(uint32_t i, bool setting):
            cur(i),
            cur_setting(setting)
    {}

    // configure target wire to the current wire and its setting
    void configure() {
        // the target setting is based on the parity of the wires
        cur_setting = ((cur & 1u) == (tar & 1u)) == cur_setting;
    }

    // switch to the sibling wire of the switch
    void update_index() {
        cur = ((tar & 1u) == 0) ? (tar + 1) : (tar - 1);
    }

    ~ext_data() = default;
};

/**
    Structure of a node in the permutation tree.
    Each node is performs a subpermutation of the global permutation.
*/
class perm_node
{
public:
    perm_node *parent;
    name_t depth;
    bool is_left_child;
    uint32_t offset;
    uint32_t size;
    // entry and exit switches of the subpermutation
    bitvector *entry;
    bitvector *exit;

    perm_node(perm_node *parent, uint32_t depth, char flag, uint32_t offset, uint32_t size):
            parent(parent),
            depth(depth),
            is_left_child(flag),
            offset(offset),
            size(size),
            entry(nullptr),
            exit(nullptr)
    {}

    ~perm_node()
    {
        delete entry;
        delete exit;
    }
};

class waksman : public ORP
{
private:
    uint32_t length;
    // we permit a leaf to have a size in {2,3,4}
    // this allows all leaf nodes to have the same depth and simplifies the routing algorithm
    uint32_t leaf_size;
    name_t temp1;
    name_t temp2;
    name_t temp3;
    // skip arrays contain elements that skip levels at the end of the configuration phase.
    // the skip array reduces the number of temporary arrays at the server
    name_t skip_array;
    uint32_t *skip_indices;

    /**
    Performs network configuration and routing simultaneously.
    The exterior of the network node is set and elements are routed to the next level.
    The procedure recurses into the two subnetworks of the node.
    @param node The input node that corresponds to a subnetwork
    @param array The identifier for the input array
    */
    void configuration_phase(perm_node *node, name_t array);

    /**
    Elements are stored at the server with the values of their upcoming switches.
    The empty road phase routes elements through the second half of the network by
     iterating through the remaining switches.
    @return the identifier of the output array
    */
    name_t empty_road_phase();

    /**
    Configures the exterior switches of the subnetwork that corresponds to node.
    The subpermutation function and the switches form a 2-regular bipartite graph. Set_exterior
     traverse the bipartite graph and enforces a 2-colouring. The colouring determines the
     switch settings.
    @param node The input node that corresponds to a subnetwork
    */
    void set_exterior(perm_node *node);

    /**
    Set the next switch (colour) in the traversal. The permutation function determines the edge and the next
     switch setting (colour) is determined by the setting (colour) of the current switch in the traversal
    @param data Data struct that contains information about the current component of the traversal
    @param res The reserve switch. If the traversal hits the end of a cycle, move to the next cycle.
    @param settings The switch settings of the target switches
    @param is_set Boolean vector that says which switches are set.
    */
    static void set_switch(ext_data *data, uint32_t *res, bitvector *settings, bitvector *is_set);

    /**
    Route the elements of a leaf node. All elements from the node are retrieved and routed according
     to the subpermutation values through the subroutine route_element.
    @param node The input node that corresponds to the subnetwork of the leaf
    @param source The identifier for the source array
    */
    void route_leaf(perm_node *node, name_t source);

    /**
    Subroutine of route_leaf that places an element in its correct position according to the network wires.
    @param node The input node that corresponds to the subnetwork of the leaf.
    @param elem The element to be routed.
    @param poff The offset in the destination array.
    @param dest The identifier for the destination array.
    */
    void route_element(perm_node *node, element *elem, uint32_t poff, uint32_t dest);

    /**
    Follows network wires for elements that skip levels.
    Determines the destination level and places element in the skip array (in a segment with elements in the
     same destination level).
    Elements in the skip array are retrieved during the empty road phase.
    @param node The input node that corresponds to the subnetwork of the leaf.
    @param elem The element to be routed.
    @param poff The offset in the skip array.
    @param dest The index to determine the number of skip elements in the destination level.
    */
    void skip_fn(perm_node *node, element *element, uint32_t off, uint32_t index);

    /**
    Routes the elements of an internal node during the configuration phase.
    @param node The input node that corresponds to the subnetwork of the leaf.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    */
    void route_internal_node_cp(perm_node *node, name_t source, name_t dest);

    /**
    Routes the element along a wire that skips level (the bottom input wire of an odd subnetwork).
    @param element The element being routed
    @param size The of the current subnetwork.
    @param perm_value The subpermutation value of the element
    @param index The index in the destination array
    @param dest The identifier for the destination array.
    */
    void route_wire(element *element, uint32_t size, uint32_t perm_value, uint32_t index, name_t dest);

    /**
    Routes the elements of a switch during the configuration phase.
    @param node The input node that corresponds to the subnetwork of the leaf.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    @param index The index in the source array
    */
    void route_switch_cp(perm_node *node, name_t source, name_t dest, uint32_t index);

    /**
    During configuration phase. Retrieves an element to place on a wire that skips networks.
    The output is given to route_wire.
    @param node The input node that corresponds to the subnetwork of the leaf.
    @param source The identifier for the source array.
    @param index The index in the source array
    */
    element *get_update_elem(perm_node *node, name_t source, uint32_t index);

    /**
    Subroutine that performs preorder traversals during the empty road phase.
    The empty road phase requires a reverse level-order traversal. This is achieved through
     a preorder traversal that only visits the leaf nodes. The leaf nodes are trimmed and
     the process is repeated. The 'depth' allows the tree to be trimmed.
    @param node The current node in the preorder traversal.
    @param depth The depth of the leaf nodes.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    @param s_index The index of the next item in the skip array.
    */
    uint32_t preorder_trav(perm_node *node, int depth, name_t source, name_t dest, uint32_t s_index);

    /**
    Routes the elements of an internal node during the empty road phase.
    @param node The input node that corresponds to the subnetwork of the internal node.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    @param s_index The index of the next item in the skip array.
    */
    unsigned int route_internal_node_erp(perm_node *parent, name_t source, name_t dest, uint32_t s_index);

    /**
    Routes the elements of a switch during the empty road phase.
    This is an overloaded function and applies in the (standard) case where neither element belongs
     to the skip array.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    @param index The index in the source array.
    @param node The input node that corresponds to the subnetwork of the exit switch.
    @param switch_num The ID of the exit switch
    */
    void route_switch_erp(name_t source, name_t dest, uint32_t index, perm_node *node, int switch_num);

    /**
    Routes the elements of a switch during the empty road phase.
    This is an overloaded function and applies in the (exceptional) case where one element belongs
     to the skip array.
    @param source The identifier for the source array.
    @param dest The identifier for the destination array.
    @param source_i The index in the source array.
    @param skip_i The index in the skip array.
    @param node The input node that corresponds to the subnetwork of the exit switch.
    @param switch_num The ID of the exit switch
    */
    void route_switch_erp(name_t source, name_t dest, uint32_t source_i, uint32_t skip_i,
            perm_node *node, int switch_num);

    /**
    Apply a switch and route elements during the empty road phase.
    The key logic involves determining the locations in the destination array (following the network wires)
    @param v_top The element on the top wire.
    @param v_bottom The element on the bottom wire.
    @param dest The identifier for the destination array.
    @param node The input node that corresponds to the subnetwork of the exit switch.
    @param switch_num The ID of the exit switch
    */
    void apply_switch(element *v_top, element *v_bottom, name_t dest, perm_node *node, int switch_num);

    /**
    A subroutine for the last level of the empty road phase.
    Takes elements from the skip array and places them in the output array.
    @param dest The identifier for the destination array.
    @param skip_i The index in the skip array.
    */
    void complete_bottom_wires(name_t dest, uint32_t skip_index);

    /**
    Evaluates the local subpermutation function.
    @param node The input node that corresponds to the subnetwork.
    @param key The key of the element
    @return pi_{node}(key)
    */
    uint32_t eval_pi(perm_node *node, uint32_t key);

    /**
    Evaluates the local subpermutation function.
    @param node The input node that corresponds to the subnetwork.
    @param key The key of the element
    @return pi^{-1}_{node}(key)
    */
    uint32_t eval_inv_pi(perm_node *node, uint32_t key);

public:
    explicit waksman(server *cloud, uint32_t size):
            ORP(cloud, size),
            length(size)
    {}

    name_t permute(name_t name) override;

    static void print_terminal(bool setting)
    {
        if(setting == PERSIST) {
            printf("PERSIST\n");
        } else if(setting == !PERSIST) {
            printf("EXCHANGE\n");
        } else {
            printf("not a recognizable terminal setting\n");
        }
    }

    /**
    Method is used during set_exterior to (efficiently) locate new cycles.
    @param bitvec A bitvector.
    @param index The previous lowest index of a false value in the bitvector.
    @return The lowest index of a false value.
    */
    static uint32_t next_null(bitvector *bitvec, uint32_t index);
};

#endif //MY_PROJECT_WAKSMAN_H