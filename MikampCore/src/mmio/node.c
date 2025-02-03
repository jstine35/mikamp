/*
 
 Mikamp Portable System Management Facilities (the MMIO)

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 ------------------------------------------------------
 node.c

 Air's own node system.
 I should rename MMIO to 'MMSTL' .. muahaha!

*/

#include "mmio.h"

// _____________________________________________________________________________________
// Removes the node from the list is currently bound to.
//
void _mm_node_delete(MM_NODE *node)
{
    if(node)
    {
        if(!node->processing)
        {
            if(node->prev)
                node->prev->next = node->next;
            if(node->next)
                node->next->prev = node->prev;
            _mm_free(node->allochandle, node);

        } else
            node->killswitch = TRUE;
    }
}

// _____________________________________________________________________________________
//
MM_NODE *_mm_node_create(MM_ALLOC *allochandle, MM_NODE *insafter, void *data)
{
    MM_NODE     *newnode;

    newnode              = _mmobj_allocblock(allochandle, MM_NODE);
    newnode->allochandle = allochandle;

    if(insafter)
    {
        if(insafter->next)
        {   insafter->next->prev = newnode;
            newnode->next        = insafter->next;
        }
        newnode->prev    = insafter;
        insafter->next   = newnode;
    }

    return newnode;
}

// _____________________________________________________________________________________
// This should be called prior to FOR loops and WHILE loops, when the node or other
// nodes in the list are subject to removal.
//
static void _mm_node_process(MM_NODE *node)
{
    if(node)
    {
        node->processing++;
        _mm_node_process(node->next);
    }
}

// _____________________________________________________________________________________
// Processes the entire list starting from the specified node, deleting any nodes flagged
// for removal.  This can be called at any time, but is generally called at the end of
// a FOR or WHILE loop that processed nodes using _mm_node_process and _mm_node_post
// commands.
//
void _mm_node_cleanup(MM_NODE *node)
{
    if(node)
    {
        assert(node->processing);
        node->processing--;
        if(!node->processing && node->killswitch)
            _mm_node_delete(node);
        _mm_node_cleanup(node->next);
    }
}