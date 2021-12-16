/********************************************************************
 Implementation of the a low memory oblivious permutation based on
 the Waksman network.
 
 Holland, W.L., Ohrimenko, O., Wirth, A., 2022, June. ASIACCS
 Efficient Oblivious Permutation via the Waksman Network.
 
 Created by William Holland on 3/02/21.
 
 MIT License
 Copyright (c) 2021 William Holland
 *********************************************************************/

#include <tgmath.h>
#include "../headers/waksman.h"



name_t waksman::permute(name_t name)
{
    // allocate temporary storage
    temp1 = name;
    temp2 = name+1;
    cloud->create_array(temp2, length);
    temp3 = name+2;
    cloud->create_array(temp3, length);
    skip_array = name+3;
    cloud->create_array(skip_array, length);

    // determine the size of a leaf
    uint32_t msb = 32 - clz(length | 1u);
    uint32_t mask = 1u << (msb-1);
    mask |= (1u << (msb-2));
    if(length > mask) {
        leaf_size = 4;
    } else {
        leaf_size = 3;
    }

    // determine the number of levels in the network
    uint32_t num_levels = 2*(sizeof(uint32_t) * CHAR_BIT - clz(length/2) - 1);
    skip_indices = (uint32_t*) calloc(num_levels/2, sizeof(uint32_t));

    // create root node of the permutation tree
    auto *root = new perm_node(nullptr, 1, true, 0, length);

    configuration_phase(root, temp1);

    delete skip_indices;

    name_t output = empty_road_phase();

    // delete unused arrays
    cloud->delete_array(skip_array);
    cloud->delete_array(temp2);
    if(output == temp1) {
        cloud->delete_array(temp3);
    } else {
        cloud->delete_array(temp1);
    }

    return output;
}

void waksman::configuration_phase(perm_node *node, name_t source_array)
{
    uint32_t size = node->size;
    // determine the output array.
    // at each level the procedure alternates between temporary arrays
    name_t target_array = (source_array == temp1) ? (temp2) : (temp1);

    if (size <= leaf_size) {
        // leaf node reached
        route_leaf(node, source_array);
    } else {
        // non-leaf node

        // set the exterior switches
        set_exterior(node);
        // route elements in the node
        route_internal_node_cp(node, source_array, target_array);

        // recurse according to a preorder traversal
        auto left = new perm_node(node, node->depth+1, true, node->offset, size/2);
        configuration_phase(left, target_array);

        auto right = new perm_node(node, node->depth+1, false, node->offset+size/2, size/2 + (size & 1u));
        configuration_phase(right, target_array);
    }
    delete node;
}

name_t waksman::empty_road_phase()
{
    // initialise the root node for traversal
    auto *root = new perm_node(nullptr, 1, true, 0, length);

    name_t source = temp3, dest = temp1;
    uint32_t skip_index = length;

    // calculate the tree height
    uint32_t tree_height = 0, size = length;
    while(size > leaf_size) {
        tree_height++;
        size /= 2;
    }

    // perform a reverse level-order traversal
    for (int i = tree_height; i > 0; i--) {
        // for each height i perform a pre-order traversal of depth i
        preorder_trav(root, i, source, dest, skip_index);
        dest = source;
        // alternate the temporary arrays
        source = (source == temp1) ? temp3 : temp1;
        // get offset for the skip elements of the next level
        skip_index /= 2;
    }
    return source;
}

void waksman::set_exterior(perm_node *node)
{
    uint32_t num_switch = ceil(node->size/(double)2);
    auto switch_set_entry = new bitvector(num_switch, false);
    auto switch_set_exit = new bitvector(num_switch, false);
    // initialise bitvectors for switch settings
    auto entry = new bitvector(num_switch, false);
    auto exit = new bitvector(num_switch, false);


    uint32_t count = 0;
    // reserve node is the starting point of the next cycle
    uint32_t res_entry = 0, res_exit = 0;
    bool inv = true;


    ext_data *data;
    if(node->size & 1u) {
        // network has odd size
        data = new ext_data(node->size-1, SWAP);
        // the bottom input and output wires are already "set"
        exit->at(num_switch-1) = SWAP;
        entry->at(num_switch-1) = SWAP;
        switch_set_entry->at(num_switch-1) = true;
        count++;
    } else {
        // network has even size
        // arbitrarily set a switch in the exterior
        data = new ext_data(node->size-1, PERSIST);
        exit->at(num_switch-1) = PERSIST;
    }

    switch_set_exit->at(num_switch-1) = true;
    count++;

    // begin traversal of the bipartite graph
    while(count < (2*num_switch)) {
        // identify the next entry node in the traversal
        // the edge connects the current output array position to a target input array position
        if(inv) {
            // moving from exit switch to entry switch
            data->tar = eval_inv_pi(node, data->cur);
            set_switch(data, &res_entry, entry, switch_set_entry);
        } else {
            // moving from entry switch to exit switch
            data->tar = eval_pi(node, data->cur);
            set_switch(data, &res_exit, exit, switch_set_exit);
        }
        inv = !inv;
        count++;
    }
    delete data;
    delete switch_set_entry;
    delete switch_set_exit;
    node->entry = entry;
    node->exit = exit;
}

void waksman::set_switch(ext_data *data, uint32_t *res, bitvector *settings, bitvector *is_set)
{
    // is the target node set
    if(!is_set->at(data->tar/2)) {
        // entry node is not set
        // configure entry node to the corresponding exit node
        data->configure();
        settings->at(data->tar/2) = data->cur_setting;
        // node is now set
        is_set->at(data->tar/2) = true;

        // move to the neighbour index of the current entry node
        data->update_index();

        // check if the reserve entry node has been set
        if(*res == data->tar/2) {
            // find next unset node as the reserve
            *res = next_null(is_set, *res);
        }
    } else {
        // entry node is set; use reserve node
        data->cur = 2*(*res);
        data->cur_setting = PERSIST;
        settings->at(data->cur/2) = data->cur_setting;
        is_set->at(data->cur/2) = true;
        // find next reserve
        *res = next_null(is_set, *res);
    }
}

void waksman::route_leaf(perm_node *node, name_t source)
{
    // the orientation of the leaf (left or right child) determines the offset in the output array
    uint32_t offset = node->parent->offset;
    if(!node->is_left_child) {
        offset++;
    }
    element *e;
    // route each element in the leaf
    for (int i = 0; i < node->size; ++i) {
        e = cloud->get(source, node->offset + i);
        route_element(node, e, offset, eval_pi(node, i));
    }
}

void waksman::route_element(perm_node *node, element *elem, uint32_t offset, uint32_t value)
{
    // does the element skip a level?
    bool skip = false;
    bool even_parent = !(node->parent->size & 1u);
    if(value == node->size-1) {
        if(even_parent || (!node->is_left_child)) {
            // if node is a right child and the parent is even
            skip = true;
        }
    }
    if(skip) {
        // element skips a level
        skip_fn(node, elem, length / 2, 0);
    } else {
        // otherwise element goes to the next level
        offset += value *2;
        cloud->put(temp3, offset, elem);
    }
}

void waksman::skip_fn(perm_node *node, element *element, uint32_t offset, uint32_t index)
{
    // The key objective is to find the destination level
    // All elements of the same destination level are placed together in the skip array

    // skip case depends on the parity of the siblings
    perm_node *parent = node->parent;

    if(parent->parent != nullptr) {
        // grandparent is a node

        // check cardinality of grandparent
        switch(parent->parent->size & 1u) {
            case EVEN :
                if(parent->size & 1u) {
                    // this is the OO case (parent and parent's sibling are odd)
                    // skip to the next level
                    skip_fn(parent, element, offset/2, index+1);
                } else {
                    // this is the EE case
                    if(node->is_left_child) {
                        // destination level is reached

                        // remove auxiliary information related to the levels skipped
                        element->aux >>= (index+1);
                        cloud->put(skip_array, offset + skip_indices[index], element);
                        skip_indices[index]++;
                    } else {
                        skip_fn(parent, element, offset/2, index+1);
                    }
                }
                break;
            case ODD :
                if(parent->is_left_child || node->is_left_child) {
                    // destination level is reached if parent or current node are left children

                    // remove auxiliary information related to the levels skipped
                    element->aux >>= (index+1);
                    cloud->put(skip_array, offset + skip_indices[index], element);
                    skip_indices[index]++;
                } else  {
                    skip_fn(parent, element, offset/2, index+1);
                }
                break;
        }
    } else {
        // parent is the root
        element->aux >>= (index+1);
        cloud->put(skip_array, offset + skip_indices[index], element);
        skip_indices[index]++;
    }
}

void waksman::route_internal_node_cp(perm_node *node, name_t source, name_t dest)
{
    uint32_t num_switches = ceil(node->size/(double)2);
    uint32_t size = node->size;

    // in all cases routing for the first (num_switches-2) switches is identical
    for (unsigned int i = 0; i < num_switches-2; ++i) {
        route_switch_cp(node, source, dest, i);
    }
    element *e1, *e2;
    // routing of remaining switches depends on the parities of current and children node sizes
    switch (size & 1u) {
        case EVEN :
            // node is even
            route_switch_cp(node, source, dest, num_switches-2);
            if((size/2) & 1u) {
                // children are odd

                // elements skip levels along a wire
                e1 = get_update_elem(node, source, node->size-2);
                e2 = get_update_elem(node, source, node->size-1);
                if(node->entry->at(num_switches-1) == PERSIST) {
                    route_wire(e1, size/2, eval_pi(node, size-2)/2, node->offset + num_switches - 1, dest);
                    route_wire(e2, size/2, eval_pi(node, size-1)/2, node->offset + size - 1, dest);
                } else {
                    route_wire(e2, size/2, eval_pi(node, size-1)/2, node->offset + num_switches - 1, dest);
                    route_wire(e1, size/2, eval_pi(node, size-2)/2, node->offset + node->size - 1, dest);
                }

            } else {
                // children are even
                route_switch_cp(node, source, dest, num_switches-1);
            }
            break;
        case ODD:
            // node is odd
            if((node->size/2) & 1u) {
                // left child is odd
                e1 = get_update_elem(node, source, node->size-3);
                e2 = get_update_elem(node, source, node->size-2);

                if(node->entry->at(num_switches-2) == PERSIST) {
                    route_wire(e1, size/2, eval_pi(node, size-3)/2, node->offset + num_switches - 2, dest);
                    cloud->put(dest, node->offset + size-2, e2);
                } else {
                    cloud->put(dest, node->offset +size-2, e1);
                    route_wire(e2, size/2, eval_pi(node, size-2)/2, node->offset + num_switches - 2, dest);
                }

            } else {
                // right child is odd

                // element on bottom wire has been previously routed.
                route_switch_cp(node, source, dest, num_switches-2);
            }

            // we have to retrieve the wire if the node is the parent
            if(node->parent == nullptr) {
                e1 = get_update_elem(node, source, node->size-1);
                route_wire(e1, ceil(node->size/(double)2), eval_pi(node, size-1)/2, node->offset + node->size - 1, dest);
            }
            break;
    }
}

void waksman::route_switch_cp(perm_node *node,name_t source, name_t dest, uint32_t index)
{
    element *u_even, *u_odd;

    // retrieve elements from the server
    u_even = get_update_elem(node, source, 2*index);
    u_odd = get_update_elem(node,source, 2*index + 1);

    // apply switch and route along wires
    if (node->entry->at(index) == PERSIST) {
        // switch = PERSIST
        cloud->put(dest, node->offset + index, u_even);
        cloud->put(dest, node->offset + node->size / 2 + index, u_odd);
    } else {
        // switch = SWAP
        cloud->put(dest, node->offset + index, u_odd);
        cloud->put(dest, node->offset + node->size / 2 + index, u_even);
    }
}

void waksman::route_wire(element *element, uint32_t size, uint32_t perm_value, uint32_t index, name_t dest) {

    // if node is even or a leaf, place element in the current level
    if( (size & 1u) == 0 || (size == 3)) {
        cloud->put(dest, index, element);
    } else {
        // skip to the next level

        // compute the value of the exit switch that is mapped to the input wire
        bool exit_switch = (perm_value & 1u) ? PERSIST : SWAP;
        dest = (dest == temp1) ? (temp2) : (temp1);
        // add exit switch to auxiliary information
        element->aux <<= 1u;
        element->aux |= (uint32_t) (exit_switch & 1u);

        // recurse
        route_wire(element, ceil(size/(double)2), perm_value/2, index, dest);
    }
}

element *waksman::get_update_elem(perm_node *node,name_t source, uint32_t index)
{
    element *elem = cloud->get(source, node->offset + index);

    // add exit settings to auxiliary information
    bool setting = node->exit->at(eval_pi(node, index)/2);
    elem->aux <<= 1u;
    elem->aux |= (uint32_t) (setting & 1u);

    return elem;
}

uint32_t waksman::preorder_trav(perm_node *node, int depth, name_t source, name_t  dest, uint32_t skip_index)
{
    if(node->depth == depth) {
        // We have hit the leaf node.
        // Route the elements through the exit switches of the subnetwork.
         return route_internal_node_erp(node, source, dest, skip_index);
    } else {
        // internal node
        uint32_t size = node->size;

        // initialise left and right children and continue the traversal
        auto left = new perm_node(node, node->depth+1, true, node->offset, size/2);
        skip_index = preorder_trav(left, depth, source, dest, skip_index);

        delete left;

        auto right = new perm_node(node, node->depth+1, false, node->offset+size/2, size/2 + (size & 1u));
        uint32_t skip = preorder_trav(right, depth, source, dest, skip_index);
        delete right;

        return skip;
    }
}

unsigned int waksman::route_internal_node_erp(perm_node *node, name_t source, name_t dest, uint32_t skip_index) {

    uint32_t num_switches = ceil(node->size/(double)2);
    // for determining the parity of the left child of the node
    uint32_t size_left = node->size/2;

    // if we are at the root node, retrieve required elements from the skip array
    if(node->parent == nullptr) {
        complete_bottom_wires(dest, skip_index/2);
    }

    uint32_t source_index = node->offset;
    if(node->size <= leaf_size*2) {
        // parents of leaf nodes contain no skip elements
        for (int i = 0; i < num_switches-1; ++i) {
            route_switch_erp(source, dest, source_index, node, i);
            source_index+=2;
        }
    } else {
        // else, the bottom switches contain elements that skipped from the configuration phase

        // route the non-skip elements first
        for (int i = 0; i < num_switches-3; ++i) {
            route_switch_erp(source, dest, source_index, node, i);
            source_index+=2;
        }

        // The indices of the skip elements depend on the parity of the node
        switch (node->size & 1u) {
            case EVEN:
                route_switch_erp(source, dest, source_index, node, num_switches-3);
                source_index+=2;

                if(size_left & 1u) {
                    // both children are odd

                    // no skip elements to retrieve
                    route_switch_erp(source, dest, source_index, node, num_switches-2);
                } else {
                    // both children are even
                    route_switch_erp(skip_array, dest, skip_index, node, num_switches-2);
                    skip_index+=2;
                }
                break;
            case ODD:
                if(size_left & 1u) {
                    // left child odd
                    route_switch_erp(source, dest, source_index, node, num_switches-3);
                    route_switch_erp(skip_array, dest, skip_index, node, num_switches-2);
                    skip_index+=2;
                } else {
                    // left child odd

                    // no skip elements to retrieve
                    route_switch_erp(source, dest, source_index+1, skip_index, node, num_switches-3);
                    route_switch_erp(source, dest, source_index+3, skip_index+1, node, num_switches-2);
                    skip_index+=2;
                }
                break;
        }
    }
    return skip_index;
}

void waksman::route_switch_erp(name_t source, name_t dest, uint32_t index, perm_node *node, int switch_num)
{
    element *v_top, *v_bottom;

    // retrieve elements
    v_top = cloud->get(source, index);
    v_bottom = cloud->get(source, index + 1);

    // apply switch and place in the next level
    apply_switch(v_top, v_bottom, dest, node, switch_num);
}

void waksman::route_switch_erp(name_t source, name_t dest, uint32_t source_i, uint32_t skip_i, perm_node *node,
                                 int switch_num)
{
    element *v_top, *v_bottom;

    // retrieve elements

    // the top wire is retrieved from the skip array
    v_top = cloud->get(skip_array, skip_i);
    v_bottom = cloud->get(source, source_i);

    // apply switch and place in the next level
    apply_switch(v_top, v_bottom, dest, node, switch_num);
}

void waksman::apply_switch(element *v_top, element *v_bottom, name_t dest, perm_node *node, int switch_num)
{
    // get the switch setting from the element auxiliary information.
    bool persist = v_top->aux & 1u;

    // update auxiliary information to remove the value of the current switch
    v_top->aux >>= 1u;
    v_bottom->aux >>= 1u;

    // calculate the index in the destination array (follow the network wires!)
    uint32_t top_index, bottom_index;
    if(node->parent == nullptr) {
        // simple case if the node is the root
        top_index = 2*switch_num;
        bottom_index = top_index+1;
    } else {
        top_index = node->parent->offset;
        top_index += 4*switch_num;
        if(!node->is_left_child) {
            top_index++;
        }
        bottom_index = top_index+2;
    }

    // apply the switch
    if(persist)
    {
        cloud->put(dest, top_index, v_top);
        cloud->put(dest, bottom_index, v_bottom);
    } else {
        cloud->put(dest, top_index, v_bottom);
        cloud->put(dest, bottom_index, v_top);
    }
}

void waksman::complete_bottom_wires(name_t dest, uint32_t skip_index)
{
    // at the root node

    // retrieve elements from the skip array and place in the output array
    element *top_wire = cloud->get(skip_array, skip_index);
    if(length & 1u) {
        // odd case
        cloud->put(dest, length-1, top_wire);
    } else {
        // even case
        element *bottom_wire = cloud->get(skip_array, skip_index+1);
        cloud->put(dest, length-2, top_wire);
        cloud->put(dest, length-1, bottom_wire);
    }
}

uint32_t waksman::eval_pi(perm_node *node, uint32_t key)
{
    perm_node *parent = node->parent;
    if(parent == nullptr) {
        // at the root node, apply input permutation function
        return pi->eval_perm(key);
    }
    // get the switch value of the element
    bool term = parent->entry->at(key);

    // at non-root node, the value of the local subpermutation function depends on the switch settings in the
    // ancestor exteriors
    if(node->is_left_child) {
        // left child
        if(term == PERSIST) {
            return eval_pi(parent, 2*key)/2;
        } else {
            return eval_pi(parent, 2*key+1)/2;
        }
    } else {
        // right child
        if(term == SWAP) {
            return eval_pi(parent, 2*key)/2;
        } else {
            return eval_pi(parent, 2*key+1)/2;
        }
    }
}

uint32_t waksman::eval_inv_pi(perm_node *node, uint32_t key)
{
    perm_node *parent = node->parent;
    if(parent == nullptr) {
        // at the root node, apply input inverse permutation function
        return pi->eval_inv_perm(key);
    }
    // get the switch value of the element
    bool setting = parent->exit->at(key);

    // at non-root node, the value of the local subpermutation function depends on the switch settings in the
    // ancestor exteriors
    if(node->is_left_child) {
        // left child
        if(setting == PERSIST) {
            return eval_inv_pi(parent, 2*key)/2;
        } else {
            return eval_inv_pi(parent, 2*key+1)/2;
        }
    } else {
        // right child
        if(setting == SWAP) {
            return eval_inv_pi(parent, 2*key)/2;
        } else {
            return eval_inv_pi(parent, 2*key+1)/2;
        }
    }
}

uint32_t waksman::next_null(bitvector *bitvec, uint32_t index) {
    uint32_t length = bitvec->size();
    if(index == length) {
        return length;
    }
    index++;

    while(index < length) {
        if(bitvec->at(index)) {
            index++;
        } else {
            return index;
        }
    }
    return index;
}














