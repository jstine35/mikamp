/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 mmalloc.c  (that's mm - alloc!)

  The Mikamp Portable Memory Allocation Layer
  
  [TODO] : OBSOLETE!  Most common runtimes provided by compiler vendors include features
  equal and superior to this crap that I rolled myself back in 1999.  This sucker is a sure
  candidate for removal, especially if the codeback is adopted to C++. --air

  Generally speaking, Mikamp only allocates nice and simple blocks of
  memory.  None-the-less, I allow the user the option to assign their own
  memory manage routines, for whatever reason it may be that they want them
  used.
 
  Notes:
   - All default allocation procedures can now ensure a fixed byte alignment on
     any allocated memory block.  The alignment is adjustable via the _MM_ALIGN
     compiler define.

*/

#include "mmio.h"
#include <string.h>

#ifdef _DEBUG

//#define MM_CHECKALLBLOCKS       // uncomment to check only local blocks during malloc/free

#ifndef _MM_OVERWRITE_BUFFER
#define _MM_OVERWRITE_BUFFER    16
#endif

#ifndef _MM_OVERWRITE_FILL
#define _MM_OVERWRITE_FILL      0x99999999
#endif

#else

#ifndef _MM_OVERWRITE_BUFFER
#define _MM_OVERWRITE_BUFFER     0
#endif

#endif

#ifdef MM_NO_NAMES
static const CHAR *msg_fail = "%s > Failure allocating block of size: %d";
#else
static const CHAR *msg_fail = "%s > In allochandle '%s' > Failure allocating block of size: %d";
#endif
static const CHAR *msg_set  = "General memory allocation failure.  Close some other applications, increase swapfile size, or buy some more ram... and then try again.";
static const CHAR *msg_head = "Out of memory!";


// Add the buffer threshold value and the size of the 'pointer storage space'
// onto the buffer size. Then, align the buffer size to the set alignment.
//
// TODO  : Make this code more thread-safe.. ?

#define BLOCKHEAP_THRESHOLD     3072

static MM_ALLOC      *globalalloc = NULL;
static MM_BLOCKINFO  *blockheap   = NULL; 
static uint           heapsize;               // number of blocks in the heap
static uint           lastblock;              // last block free'd (or first free block)

#ifdef _DEBUG

// _____________________________________________________________________________________
//
static BOOL checkunder(MM_ALLOC *type, MM_BLOCKINFO *block, BOOL doassert)
{
    ULONG   *ptr = block->block;
    uint     i;

    if(!block->block) return FALSE;

    // Check for Underwrites
    // ---------------------

    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, ptr++)
    {   if(*ptr != _MM_OVERWRITE_FILL)
        {   if(doassert)
            {   _mmlog("** _mm_checkunder > Memory underwrite on block type '%s'", type->name);
                assert(FALSE);
            }
            return FALSE;
        }
    }
    return TRUE;
}

// _____________________________________________________________________________________
//
static BOOL checkover(MM_ALLOC *type, MM_BLOCKINFO *block, BOOL doassert)
{
    ULONG   *ptr = block->block;
    uint     i;

    if(!block->block) return FALSE;

    // Check for Overwrites
    // --------------------

    ptr = (ULONG *)block->block;
    ptr = &ptr[block->size + _MM_OVERWRITE_BUFFER];

    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, ptr++)
    {
        if(*ptr != _MM_OVERWRITE_FILL)
        {
            if(doassert)
            {
                _mmlog("** _mm_checkover > Memory overwrite on block type '%s'", type->name);
                assert(FALSE);
            }
            return FALSE;
        }
    }
    return TRUE;
}

// _____________________________________________________________________________________
//
static void __inline recurse_checkallblocks(MM_ALLOC *type)
{
    int            cruise = type->blocklist;
    MM_ALLOC      *typec  = type->children;

    while(typec)
    {
        recurse_checkallblocks(typec);
        typec = typec->next;
    }

    while(cruise != -1)
    {
        MM_BLOCKINFO   *block = &blockheap[cruise];
        checkover(type, block, TRUE);
        cruise = block->next;
    }

    cruise = type->blocklist;
    while(cruise != -1)
    {
        MM_BLOCKINFO   *block = &blockheap[cruise];
        checkunder(type, block, TRUE);
        cruise = block->next;
    }
}

// _____________________________________________________________________________________
// Does a routine check on all blocks in the specified allocation type for possible mem-
// ory corruption (via the overflow and underflow buffers or, as some people call them,
// 'No Mans Land Buffers').
//
void _mm_checkallblocks(MM_ALLOC *type)
{
    if(!type) type = globalalloc;
    if(type) recurse_checkallblocks(type);
}

// _____________________________________________________________________________________
// Dumps a memory report to logfile on all memory types.
//
void _mm_allocreportall(void)
{

}

#else
#define checkover(x,y,z)  TRUE
#define checkunder(x,y,z)  TRUE
#endif


// _____________________________________________________________________________________
//
    static void tallyblocks(MM_ALLOC *type, uint *runsum, uint *blkcnt)
{
    int cruise = type->blocklist;

    if(type->children)
    {   MM_ALLOC *child = type->children;
        while(child)
        {   tallyblocks(child, runsum, blkcnt);
            child = child->next;
        }
    }

    while(cruise != -1)
    {   MM_BLOCKINFO   *block = &blockheap[cruise];
        *runsum += block->size;
        (*blkcnt)++;
        cruise = block->next;
    }

    *blkcnt += type->size;
}

// _____________________________________________________________________________________
//
void _mmalloc_reportchild(MM_ALLOC *type, uint tabsize)
{
    CHAR   tab[60];
    uint   i;

    if(!type) return;

    for(i=0; i<tabsize; i++) tab[i] = 32;
    tab[i] = 0;

    if(type)
    {   int  cruise = type->blocklist;

        uint    runsum = 0;
        uint    blkcnt = 0;

        _mmlog("\n%sChild AllocReport for %s", tab, type->name);

        if(type->children)
        {   tallyblocks(type, &runsum, &blkcnt);
            _mmlog("%sTotal Blocks    : %d\n"
                   "%sTotal Memory    : %dK", tab, blkcnt, tab, runsum/256);
        }

        runsum = 0;
        blkcnt = 0;

        while(cruise != -1)
        {   MM_BLOCKINFO   *block = &blockheap[cruise];
            runsum += block->size;
            blkcnt++;
            cruise = block->next;
        }

        _mmlog("%sLocal Blocks    : %d\n"
               "%sLocal Memory    : %dK", tab, blkcnt, tab, runsum/256);

#ifdef _DEBUG
        blkcnt = 0;
        cruise = type->blocklist;
        while(cruise != -1)
        {
            MM_BLOCKINFO   *block = &blockheap[cruise];

            if(type->checkplease)
            {   if(!checkover(type, block, FALSE) || !checkunder(type, block, FALSE))
                    blkcnt++;
            }

            cruise = block->next;
        }
        if(blkcnt) _mmlog("%sBad Blocks      : %d", tab,blkcnt);
#endif

        if(type->children)
        {
            MM_ALLOC *child = type->children;
            _mmlog("%sChildren types:", tab);

            while(child)
            {   _mmlog("%s - %s", tab, child->name);
                child = child->next;
            }

            child = type->children;
            while(child)
            {   _mmalloc_reportchild(child, tabsize+4);
                child = child->next;
            }
        }
    }
}

// _____________________________________________________________________________________
// Dumps a memory report to logfile on the specified block type.
//
void _mmalloc_report(MM_ALLOC *type)
{
    if(!type)
        type = globalalloc;

    if(type)
    {   uint    runsum = 0;
        uint    blkcnt = 0;
        int     cruise = type->blocklist;

        _mmlog("\nAllocReport for %s\n--------------------------------------\n", type->name);

        if(type->children)
        {   tallyblocks(type, &runsum, &blkcnt);
            _mmlog("Total Blocks    : %d\n"
                   "Total Memory    : %dK", blkcnt, runsum/256);
        }

        runsum = 0;
        blkcnt = 0;

        while(cruise != -1)
        {   MM_BLOCKINFO   *block = &blockheap[cruise];
            runsum += block->size;
            blkcnt++;
            cruise = block->next;
        }

        _mmlog("Local Blocks    : %d\n"
               "Local Memory    : %dK", blkcnt, runsum/256);

#ifdef _DEBUG
        blkcnt = 0;
        cruise = type->blocklist;
        while(cruise != -1)
        {
            MM_BLOCKINFO   *block = &blockheap[cruise];

            if(type->checkplease)
            {   if(!checkover(type, block, FALSE) || !checkunder(type, block, FALSE))
                    blkcnt++;
            }

            cruise = block->next;
        }
        if(blkcnt) _mmlog("Bad Blocks      : %d", blkcnt);
#endif

        if(type->children)
        {
            MM_ALLOC *child = type->children;
            _mmlog("Children types:");

            while(child)
            {   _mmlog(" - %s", child->name);
                child = child->next;
            }

            child = type->children;
            while(child)
            {   _mmalloc_reportchild(child, 4);
                child = child->next;
            }
        }

        _mmlog("--------------------------------------\n", type->name);
    }
}

// _____________________________________________________________________________________
// Create an mmio allocation handle for allocating memory in groups/pages.
//
static MM_ALLOC *createglobal(void)
{
    MM_ALLOC   *type;

    if(!blockheap)
    {   blockheap = malloc(sizeof(MM_BLOCKINFO) * BLOCKHEAP_THRESHOLD);
        heapsize  = BLOCKHEAP_THRESHOLD;
        {   uint  i;
            MM_BLOCKINFO  *block = blockheap;
            for(i=0; i<heapsize; i++, block++) block->block = NULL;
        }
    }

    type = (MM_ALLOC *)calloc(1, sizeof(MM_ALLOC));
    strcpy(type->name, "globalalloc");

#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    type->blocklist = -1;

    return type;
}

#define myretval(b)    return &(b)->block[_MM_OVERWRITE_BUFFER]
#define myblockval(b)  &(b)->block[_MM_OVERWRITE_BUFFER]

// _____________________________________________________________________________________
//
static ULONG __inline *mymalloc(MM_ALLOC *type, uint size)
{
#ifdef _DEBUG
    if(type->checkplease)
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif
    return (ULONG *)malloc((size + (_MM_OVERWRITE_BUFFER*2)) * 4);
}

// _____________________________________________________________________________________
//
static ULONG __inline *myrealloc(MM_ALLOC *type, ULONG *old_blk, uint size)
{
#ifdef _DEBUG
    if(type->checkplease)
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif
    return (ULONG *)realloc(old_blk, (size + (_MM_OVERWRITE_BUFFER*2)) * 4);
}

// _____________________________________________________________________________________
//
#ifndef MM_NO_NAMES
    static MM_BLOCKINFO *addblock(const CHAR *name, MM_ALLOC *type, ULONG *ptr, uint size, void (*deallocate)(void *block, uint count))
#else
    static MM_BLOCKINFO *addblock(MM_ALLOC *type, ULONG *ptr, uint size, void (*deallocate)(void *block, uint count))
#endif
{
    MM_BLOCKINFO *block;
    uint          i;

    for(i=lastblock, block = &blockheap[lastblock]; i<heapsize; i++, block++)
        if(!block->block) break;

    if(!block)
    {   for(i=0, block = blockheap; i<lastblock; i++, block++)
            if(!block->block) break;
    }

    lastblock = i+1;

    if(!block || (lastblock >= heapsize))
    {   
        // Woops! No open blocks.
        // ----------------------
        // Lets just make some more room, shall we?

        blockheap = realloc(blockheap, (heapsize+BLOCKHEAP_THRESHOLD) * sizeof(MM_BLOCKINFO));
        block     = &blockheap[i = heapsize];
        lastblock = heapsize+1;

        heapsize += BLOCKHEAP_THRESHOLD;

        {   uint  t;
            MM_BLOCKINFO  *cruise = &blockheap[lastblock];
            for(t=lastblock; t<heapsize; t++, cruise++) cruise->block = NULL;
        }

    }
    
    //assert( i != 4976 );
    //assert( i != 4975 );

#ifndef MM_NO_NAMES
    _mm_strcpy(block->name, name, MM_NAME_LENGTH);
#endif
    block->size       = size;
    block->nitems     = 0;
    block->deallocate = deallocate;
    block->block      = ptr;
    block->next       = type->blocklist;
    block->prev       = -1;

    if(type->blocklist != -1)
        blockheap[type->blocklist].prev = i;

    type->blocklist = i;

    return block;
}

// _____________________________________________________________________________________
// Deallocates the given block and removes it from the allocation list.
//
static void removeblock(MM_ALLOC *type, int blkidx)
{
    MM_BLOCKINFO *block;

    //assert( blkidx != 4976 );
    //assert( blkidx != 4975 );

#ifdef _DEBUG
    if(type->checkplease) 
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif

    block = &blockheap[blkidx];
    if(block->deallocate) block->deallocate(myblockval(block), block->nitems);

    // NOTE: Re-fetch block pointer incase realloc dicked with it!

    block = &blockheap[blkidx];
    if(block->prev != -1)
        blockheap[block->prev].next = block->next;
    else
        type->blocklist = block->next;

    if(block->next != -1) blockheap[block->next].prev = block->prev;

    free(block->block);
    block->block = NULL;
    //free(block);
}

// _____________________________________________________________________________________
// Fills the underrun and overrun buffers with the magic databyte!
// if 'wipe' is set TRUE, then it works like calloc,a nd clears out the dataspace.
//
static void fillblock(MM_BLOCKINFO *block, BOOL wipe)
{
    ULONG *tptr = block->block;

#ifdef _DEBUG
    uint  i;
    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, tptr++) *tptr = _MM_OVERWRITE_FILL;
#endif

    if(wipe) memset(tptr, 0, block->size*4);
    tptr+=block->size;

#ifdef _DEBUG
    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, tptr++) *tptr = _MM_OVERWRITE_FILL;
#endif
}

// _____________________________________________________________________________________
// adds the block 'type' to the specified block 'parent'
//
static void __inline _addtolist(MM_ALLOC *parent, MM_ALLOC *type)
{
    if(!parent)
    {   if(!globalalloc) globalalloc = createglobal();
        parent = globalalloc;
    }

    type->next = parent->children;
    if(parent->children) parent->children->prev = type;
    parent->children = type;
    type->parent     = parent;
    type->blocklist = -1;
}

// _____________________________________________________________________________________
// Create an mmio allocation handle for allocating memory in groups/pages.
// TODO : Add overwrite checking for these blocks
//
#ifndef MM_NO_NAMES
    MMEXPORT MM_ALLOC *_mmalloc_create(const CHAR *name, MM_ALLOC *parent)
#else
    MMEXPORT MM_ALLOC *mmalloc_create(MM_ALLOC *parent)
#endif
{
    MM_ALLOC   *type;

    type        = (MM_ALLOC *)calloc(1, sizeof(MM_ALLOC));
    type->size  = (sizeof(MM_ALLOC) / 4) + 1;

#ifndef MM_NO_NAMES
    _mm_strcpy(type->name, name, MM_NAME_LENGTH);
#endif
#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    _addtolist(parent, type);

    return type;
}

// _____________________________________________________________________________________
// Create an mmio allocation handle for allocating memory in groups/pages.
// TODO : Add overwrite checking for these blocks
//
#ifndef MM_NO_NAMES
    MMEXPORT MM_ALLOC *_mmalloc_create_ex(const CHAR *name, MM_ALLOC *parent, uint size)
#else
    MMEXPORT MM_ALLOC *mmalloc_create_ex(MM_ALLOC *parent, uint size)
#endif
{
    MM_ALLOC   *type;

    assert(size >= sizeof(MM_ALLOC));            // this can't be good!
    if(!size) size = sizeof(MM_ALLOC);

    type        = (MM_ALLOC *)calloc(1, size);
    type->size  = (size / 4) + 1;

#ifndef MM_NO_NAMES
    _mm_strcpy(type->name, name, MM_NAME_LENGTH);
#endif

#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    _addtolist(parent, type);

    return type;
}

// _____________________________________________________________________________________
//
MMEXPORT void _mmalloc_setshutdown(MM_ALLOC *type, void (*shutdown)(void *block), void *block)
{
    if(type)
    {   type->shutdown = shutdown;
        type->shutdata = block;
    }
}

// _____________________________________________________________________________________
// Pretty self-explainitory, as usual: closes *ALL* allocated memory.  For use during a
// complete shutdown or restart of the application.
//
MMEXPORT void _mmalloc_closeall(void)
{
    _mmalloc_close(globalalloc);
    globalalloc = NULL;
}

// _____________________________________________________________________________________
//
static void __inline recurse_passive_close(MM_ALLOC *type)
{
    MM_ALLOC     *typec;

    type->killswitch = TRUE;
    if(type->shutdown) type->shutdown(type->shutdata);

    typec  = type->children;

    while(typec)
    {   if(!typec->killswitch) recurse_passive_close(typec);
        typec = typec->next;
    }
}

// _____________________________________________________________________________________
// Flags the given block for closure but does NOT free the block.  It is the responsibility
// of the caller to use _mmalloc_passive_cleanup to process all blocks and free ones
// which have been properly marked for termination.  This call processes the block 
// and all children, calling shutdown procedures-- but not freeing blocks.
//
MMEXPORT void __inline _mmalloc_passive_close(MM_ALLOC *type)
{
    if(type && !type->killswitch)
        recurse_passive_close(type);
}

// _____________________________________________________________________________________
// frees all the blocks of the given type block handle.  No recursive action. (WOW)
// Does not unload Type or remove it from the parental linked list.
//
static void __inline _forcefree(MM_ALLOC *type)
{
    int         cruise = type->blocklist;
    while(cruise != -1)
    {
        MM_BLOCKINFO *block = &blockheap[cruise];

    //assert( cruise != 4977 );
    //assert( cruise != 4976 );
    //assert( cruise != 4975 );

#if (_MM_OVERWRITE_BUFFER != 0)
        if(type->checkplease)
        {   checkover(type, block, TRUE);
            checkunder(type, block, TRUE);
        }
#endif
        if( block->deallocate )
        {
            //assert( FALSE );
            block->deallocate( myblockval(block), block->nitems );
        }

        // NOTE: Re-get block address, incase deallocate changed it.
        block = &blockheap[cruise];
        if(block->block) free(block->block);
        block->block = NULL;
        lastblock    = cruise;
        cruise       = block->next;
    }
}

// _____________________________________________________________________________________
// processes the given allocblock and all of it's children, freeing all blocks which
// have been marked for termination (type->killswitch).  This method of block removal
// is the safest method for avoiding linked list errors caused by blocks that kill them
// selves during callbacks.
//
MMEXPORT BOOL _mmalloc_cleanup(MM_ALLOC *type)
{
    MM_ALLOC   *typec  = type->children;

    while(typec)
    {
        MM_ALLOC *cnext = typec->next;
        if(!typec->processing)
        {
            typec->processing = TRUE;           // prevent recursive _mmalloc_close calls
            if(!_mmalloc_cleanup(typec)) typec->processing = FALSE;
        }
        typec = cnext;
    }

    if(type->killswitch)
    {
        // Unload Blocks & Remove From List!
        // ---------------------------------

        _forcefree(type);
        if(type->parent)
        {   if(type->prev)
                type->prev->next = type->next;
            else
                type->parent->children = type->next;

            if(type->next)
                type->next->prev = type->prev;
        }
        free(type);
        return TRUE;
    }
    return FALSE;
}

// _____________________________________________________________________________________
// deallocates all memory bound to the given allocation type, but does not close the
// handle - memory can be re-allocated to it at any time.
// TODO - Code me, if I'm ever needed! ;)
//
MMEXPORT void _mmalloc_freeall(MM_ALLOC *type)
{
    if(type)
    {
        MM_ALLOC     *typec = type->children;

        while(typec)
        {   if(!typec->killswitch) recurse_passive_close(typec);
            typec = typec->next;
        }

        if(!type->processing)
            if(!_mmalloc_cleanup(type)) type->processing = FALSE;
    }
}

// _____________________________________________________________________________________
// Shuts down the given alloc handle/type.  Frees all memory associated with the handle
// and optionally checks for buffer under/overruns (if they have been flagged).
// NULL is not a valid parameter-- use _mmalloc_closeall instead!
//
MMEXPORT void _mmalloc_close(MM_ALLOC *type)
{
    if( type )
    {
        BOOL    processing = type->processing;
        type->processing = TRUE;
        if(!type->killswitch) recurse_passive_close(type);
        if(!processing)
            if(!_mmalloc_cleanup(type)) type->processing = FALSE;
    }
}

// _____________________________________________________________________________________
//
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_mallocx(const CHAR *name, MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint nitems))
#else
    MMEXPORT void *mm_mallocx(MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint nitems))
#endif
{
    ULONG   *d;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_malloc)
        d = type->fptr_malloc(size);
    else
#endif
    {   size = (size/4)+1;
        d = mymalloc(type, size);

        if(!d)
        {   
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_malloc", size);
            #else
                _mmlog(msg_fail, "_mm_malloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        } else
        {
        #ifndef MM_NO_NAMES
            MM_BLOCKINFO *block = addblock(name, type, d, size, deinitialize);
        #else
            MM_BLOCKINFO *block = addblock(type, d, size, deinitialize);
        #endif
            fillblock(block, FALSE);
            myretval(block);
        }
    }

    return d;
}

// _____________________________________________________________________________________
// this is used internally only for recursive searching for a deallocated block in the
// event that the initial basic search for the block fails.  Removes the block in the event
// that it is found, and returns the alloctype it was found in (so we can report it to the
// logfile).  If not found, returns NULL!
//
static MM_ALLOC *slowfree(MM_ALLOC *type, ULONG *d)
{
    MM_ALLOC   *found = NULL;

    while(type && !found)
    {   int   cruise = type->blocklist;
        while((cruise != -1) && (blockheap[cruise].block != d)) cruise = blockheap[cruise].next;
        
        if(cruise != -1)
        {   removeblock(type, cruise); //&blockheap[]);
            lastblock = cruise;
            break;
        }

        found = slowfree(type->children, d);
        type = type->next;
    }

    return found ? found : type;
}

// _____________________________________________________________________________________
// Free the memory specified by the given pointer.  Checks the given type first and if
// not found there checks the entire hierarchy of memory allocations.  If the type is
// NULL, the all memory is checked reguardless.
//
MMEXPORT void _mm_a_free(MM_ALLOC *type, void **d)
{
    if(d && *d)
    {
        if(!type)
        {   if(!globalalloc) return;
            type = globalalloc;
        }

#ifndef MMALLOC_NO_FUNCPTR
        if(type->fptr_free)
            type->fptr_free(*d);
        else
#endif
        {
            int    cruise;
            ULONG *dd = *d;

            dd -= _MM_OVERWRITE_BUFFER;

            // Note to Self / TODO
            // -------------------
            // I may want to change it so that free checks all children of the specified
            // blocktype without warning or error before resorting to a full search.
            // [...]

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   removeblock(type, cruise);
                lastblock = cruise;
            } else
            {
                MM_ALLOC    *ctype;

                // Check All Memory Regions
                // ------------------------
                // This is a very simple process thanks to some skilled use of recursion!

                ctype = slowfree(globalalloc->children, dd);

                if(ctype)
                {   _mmlog("_mm_free > (Warning) Freed block was outside specified alloctype!\n"
                           "         > (Warning) Type specified: \'%s\'\n"
                           "         > (Warning) Found in      : \'%s\'", type->name, ctype->name);

                    assert(FALSE);
                } else
                {   _mmlog("** _mm_free > Specified block not found! (type %s, address = %d)", type->name, dd);
                    assert(FALSE);
                }
            }

            *d = NULL;
        }
    }
}

// _____________________________________________________________________________________
//
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_callocx(const CHAR *name, MM_ALLOC *type, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#else
    MMEXPORT void *mm_callocx(MM_ALLOC *type, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#endif
{
    ULONG *d;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_calloc)
    {   d = type->fptr_calloc(nitems, size);
    } else
#endif
    {   
        // The purpose of calloc : to save others the work of the multiply!

        size = nitems * size;
        size = (size/4)+1;

        d = mymalloc(type,size);
        if(d)
        {
        #ifndef MM_NO_NAMES
            MM_BLOCKINFO *block = addblock(name, type, d, size, deallocate);
        #else
            MM_BLOCKINFO *block = addblock(type, d, size, deallocate);
        #endif
            block->nitems      = nitems;
            fillblock(block, TRUE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_calloc", size);
            #else
                _mmlog(msg_fail, "_mm_calloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}

// _____________________________________________________________________________________
//
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_reallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t size, void (*deallocate)(void *block, uint nitems))
#else
    MMEXPORT void *mm_reallocx(MM_ALLOC *type, void *old_blk, size_t size, void (*deallocate)(void *block, uint nitems))
#endif
{
    ULONG *d = NULL;    

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_realloc)
        d = type->fptr_realloc(old_blk, size);
    else
#endif
    {
        MM_BLOCKINFO   *block;

        size = (size/4)+1;

        if(old_blk)
        {
            int             cruise;
            ULONG          *dd = old_blk;

            dd -= _MM_OVERWRITE_BUFFER;

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   d = myrealloc(type, dd, size);

                block = &blockheap[cruise];

                block->size       = size;
                block->block      = d;
                block->deallocate = deallocate;
            } else
            {   _mmlog("** _mm_realloc > Block not found...");
                assert(FALSE);
            }
        } else
        {   d = mymalloc(type,size);
        #ifndef MM_NO_NAMES
            block = addblock(name, type, d, size, deallocate);
        #else
            block = addblock(type, d, size, deallocate);
        #endif
        }

        if(d)
        {   fillblock(block, FALSE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_realloc", size);
            #else
                _mmlog(msg_fail, "_mm_realloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}

// _____________________________________________________________________________________
// My own special reallocator, which works like calloc by allocating memory by both block-
// size and numitems.
//
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_recallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#else
    MMEXPORT void *mm_recallocx(MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#endif
{
    ULONG   *d = NULL;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_recalloc)
    {   d = type->fptr_recalloc(old_blk, nitems, size);
    } else
#endif
    {
        MM_BLOCKINFO   *block;
        // The purpose of calloc : to save others the work of the multiply!

        size = nitems * size;
        size = (size/4)+1;

        if(old_blk)
        {
            int             cruise;
            ULONG          *dd = old_blk;

            dd -= _MM_OVERWRITE_BUFFER;

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   d = myrealloc(type, dd, size);

                block = &blockheap[cruise];

                if(block->size < size)
                {   ULONG   *dd = (ULONG *)d;
                    memset(&dd[block->size+_MM_OVERWRITE_BUFFER], 0, (size - block->size)*4);
                }

                block->size       = size;
                block->block      = d;
                block->deallocate = deallocate;
            } else
            {   _mmlog("** _mm_recalloc > Block not found...");
                assert(FALSE);
            }
        } else
        {   d = mymalloc(type,size);
        #ifndef MM_NO_NAMES
            block = addblock(name, type, d, size, deallocate);
        #else
            block = addblock(type, d, size, deallocate);
        #endif
            memset(myblockval(block), 0, size*4);
        }

        if(d)
        {   block->nitems  = nitems;
            fillblock(block, FALSE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_recalloc", size);
            #else
                _mmlog(msg_fail, "_mm_recalloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}
