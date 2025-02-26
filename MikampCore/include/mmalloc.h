
// OBSOLETE : Most or all of this module. :)
// TODO : remove embedded naming persistence and use a global std::unordered_map to
//        associate debug names with pointers.

#pragma once

// ===================================================
//  Memory allocation with error handling - MMALLOC.C
// ===================================================

// Block Types In-Depth
// --------------------
// By default, a global blocktype is created which is the default block used when the
// user called _mm_malloc
//

// -------------------------------------------------------------------------------------
//
typedef struct MM_BLOCKINFO
{
#ifndef MM_NO_NAMES
    CHAR            name[MM_NAME_LENGTH];  // name of the block, only when names enabled.
#endif

    uint            size;                  // size of the memory allocation, in dwords
    uint            nitems;
    ULONG          *block;
    void          (*deallocate)(void *block, uint count);

    int            next, prev;             // you know what a linked list is
                                           // (this one is based on indexes, not pointers)
} MM_BLOCKINFO;


// -------------------------------------------------------------------------------------
// An allocation handle for use with the mmio allocation functions.
//
typedef struct _MM_ALLOC
{
#ifdef _DEBUG
    BOOL               checkplease;
#endif
    int                blocklist;
    uint               ident;            // blocktype identifier!

    struct _MM_ALLOC  *next, *prev;      // you know what a linked list is.
    struct _MM_ALLOC  *children, *parent;

    BOOL               killswitch;
    BOOL               processing;       // Set TRUE if we're in list-processing callback state

#ifndef MM_NO_NAMES
    CHAR               name[MM_NAME_LENGTH]; // every allocation type has a name.  wowie zowie!
#endif

    uint               size;             // size of allochandle in dwords
    void             (*shutdown)(void *blockdata);
    void              *shutdata;

#ifndef MMALLOC_NO_FUNCPTR
    void *(*fptr_malloc)    (size_t size);
    void *(*fptr_calloc)    (size_t nitems, size_t size);
    void *(*fptr_realloc)   (void *old_blk, size_t size);
    void *(*fptr_recalloc)  (void *old_blk, size_t nitems, size_t size);
    void *(*fptr_free)      (void *d);
#endif
} MM_ALLOC;


// -------------------------------------------------------------------------------------
// Air's own version of the C Vector / C# Collection junk.  An array that automatically
// resizes as needed (with additional resize functionality provided).
// So simple, dogs rejoice at the thought of gnawing on this.
//
typedef struct MM_VECTOR
{
    MM_ALLOC    *allochandle;          // allocation handle we're bound to
    uint         alloc;
    uint         count;

    uint         increment;            // amount to increment array by
    uint         threshold;            // no fewer than this many free elements allowed
    uint         strucsize;            // structure size, in bytes

#ifdef _DEBUG
    void        *ptr;                  // set by first call to alloc, and used for assert checking on 
                                       // subsequeent calls.
#endif

} MM_VECTOR;


// -------------------------------------------------------------------------------------
// Ultra elite void-ptr-based node lists!  Bi-directional even.  So elite they give Dick
// Chaney's auto-robotic heart palpitations everytime we use them!
//
typedef struct MM_NODE
{
    struct MM_NODE  *next, *prev;
    void            *data;              // data object bound to this node!
    int              usrval;            // additional user-attached value (handy dandy stuff)

    int              processing;        // incremented every time the node is accessed
    BOOL             killswitch;        // flagged if node deleted while in processing state

    MM_ALLOC        *allochandle;

} MM_NODE;


#define _mmalloc_callback_enter(t) \
    BOOL  _mma_processing = (t)->processing; \
    (t)->processing = TRUE;

#define _mmalloc_callback_exit(t)  { (t)->processing = _mma_processing; if((t)->killswitch) _mmalloc_cleanup(t); }
#define _mmalloc_callback_next(t) \
{   MM_ALLOC *_ttmp321 = (t)->next; \
    _mmalloc_callback_exit(t); \
    (t) = _ttmp321;  } \


#ifdef _DEBUG
#define _mmalloc_check_set(a)    (a)->checkplease = TRUE
#define _mmalloc_check_clear(a)  (a)->checkplease = FALSE
#else
#define _mmalloc_check_set(a)
#define _mmalloc_check_clear(a)
#endif

#define MMBLK_IDENT_ALLOCHANDLE      0
#define MMBLK_IDENT_USER         0x100

MMEXPORT void      _mmalloc_freeall(MM_ALLOC *type);
MMEXPORT void      _mmalloc_close(MM_ALLOC *type);
MMEXPORT void      _mmalloc_closeall(void);
MMEXPORT void      _mmalloc_passive_close(MM_ALLOC *type);
MMEXPORT BOOL      _mmalloc_cleanup(MM_ALLOC *type);

MMEXPORT void      _mmalloc_setshutdown(MM_ALLOC *type, void (*shutdown)(void *block), void *block);
MMEXPORT void      _mmalloc_report(MM_ALLOC *type);

#ifdef _DEBUG
MMEXPORT void      _mm_checkallblocks(MM_ALLOC *type);
MMEXPORT void      _mm_allocreportall(void);
#else
#define _mm_checkallblocks(x)
#endif

// Defined in vector.c
// -------------------

MMEXPORT MM_VECTOR *_mm_vector_create(MM_ALLOC *allochandle, uint strucsize, uint increment);
MMEXPORT void      _mm_vector_init(MM_VECTOR *array, MM_ALLOC *allochandle, uint strucsize, uint increment);
MMEXPORT void      _mm_vector_alloc(MM_VECTOR *array, void **ptr);
MMEXPORT void      _mm_vector_close(MM_VECTOR *array, void **ptr);
MMEXPORT void      _mm_vector_free(MM_VECTOR **array, void **ptr);

// Defined in node.c
// -----------------

MMEXPORT void      _mm_node_cleanup(MM_NODE *node);
MMEXPORT void      _mm_node_process(MM_NODE *node);
MMEXPORT MM_NODE  *_mm_node_create(MM_ALLOC *allochandle, MM_NODE *insafter, void *data);
MMEXPORT void      _mm_node_delete(MM_NODE *node);

// mm_alloc macro parameters
// -------------------------
//
// a  - Allochandle (MM_ALLOC *)
// n  - Number of blocks
// s  - Size of Array (in blocks)
// o  - old/original pointer
//
// p  - Parent struct (must inherit allochandle)
// c  - number of elements within the array
// t  - Array type

#ifdef MM_NO_NAMES

    #define _mm_mallocx(crap,a,s,b)       mm_mallocx(a,s,b)
    #define _mm_callocx(crap,a,n,s)       mm_callocx(a,n,s, NULL)
    #define _mm_reallocx(crap,a,o,s)      mm_reallocx(a,o,s, NULL)
    #define _mm_recallocx(crap,a,o,n,s)   mm_recallocx(a,o,n,s, NULL)

    MMEXPORT void     *mm_mallocx(MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_callocx(MM_ALLOC *type, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_reallocx(MM_ALLOC *type, void *old_blk, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_recallocx(MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));

    MMEXPORT MM_ALLOC *mmalloc_create(MM_ALLOC *parent);
    MMEXPORT MM_ALLOC *mmalloc_create_ex(MM_ALLOC *parent, uint size);

    #define _mmalloc_create(name, p)    mmalloc_create(p)
    #define _mmalloc_create_ex(name, p) mmalloc_create_ex(p,s)

    #define _mm_malloc(a,s)         _mm_mallocx(a,s, NULL)
    #define _mm_calloc(a,n,s)       _mm_callocx(a,n,s, NULL)
    #define _mm_realloc(a,o,s)      _mm_reallocx(a,o,s, NULL)
    #define _mm_recalloc(a,o,n,s)   _mm_recallocx(a,o,n,s, NULL)

    #define _mmobj_new(p,t)         (t*)_mmalloc_create_ex((MM_ALLOC *)(p), sizeof(t))
    #define _mmobj_array(p,c,t)     (t*)_mm_calloc        ((MM_ALLOC *)(p), c, sizeof(t))
    #define _mmobj_allocblock(p,t)  (t*)_mm_calloc        ((MM_ALLOC *)(p), 1, sizeof(t))

    //#define _mm_new(a,t)            (t*)_mm_calloc(a, 1, sizeof(t))
    //#define _mm_array(a,c,t)        (t*)_mm_calloc(a, c, sizeof(t))

#else

    MMEXPORT void     *_mm_mallocx(const CHAR *name, MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_callocx(const CHAR *name, MM_ALLOC *type, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_reallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_recallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));

    MMEXPORT MM_ALLOC *_mmalloc_create(const CHAR *name, MM_ALLOC *parent);
    MMEXPORT MM_ALLOC *_mmalloc_create_ex(const CHAR *name, MM_ALLOC *parent, uint size);

    #define _mm_malloc(a,s)         _mm_mallocx  (NULL, a, s,       NULL)
    #define _mm_calloc(a,n,s)       _mm_callocx  (NULL, a, n, s,    NULL)
    #define _mm_realloc(a,o,s)      _mm_reallocx (NULL, a, o, s,    NULL)
    #define _mm_recalloc(a,o,n,s)   _mm_recallocx(NULL, a, o, n, s, NULL)

    #define _mmobj_new(p,t)         (t*)_mmalloc_create_ex(#t, (MM_ALLOC *)(p), sizeof(t))
    #define _mmobj_array(p,c,t)     (t*)_mm_callocx       (#t, (MM_ALLOC *)(p), c, sizeof(t), NULL)
    #define _mmobj_allocblock(p,t)  (t*)_mm_callocx       (#t, (MM_ALLOC *)(p), 1, sizeof(t), NULL)

    //#define _mm_new(a,t)            (t*)_mm_callocx(#t, a, 1, sizeof(t), NULL)
    //#define _mm_array(a,c,t)        (t*)_mm_callocx(#t, a, c, sizeof(t), NULL)

#endif

MMEXPORT void      _mm_a_free(MM_ALLOC *type, void **d);

// Adding a cast to (void **) gets rid of a few dozen warnings.
#define _mm_free(obj, blk)          _mm_a_free(obj, (void **)&(blk))
#define _mmobj_free(obj, blk)       _mm_a_free((MM_ALLOC *)(obj), (void **)&(blk))
#define _mmobj_close(obj)           _mmalloc_close((MM_ALLOC *)(obj))
#define _mmobj_strdup(obj, str)     _mm_strdup((MM_ALLOC *)(obj), str);

// Defined in mmcopy.c
// -------------------

MMEXPORT CHAR   *_mm_strcpy(CHAR *dest, const CHAR *src, const uint maxlen);
MMEXPORT CHAR   *_mm_strcat(CHAR *dest, const CHAR *src, const uint maxlen);
MMEXPORT int     _mm_strcmp(const CHAR *str1, const CHAR *str2);
MMEXPORT CHAR   *_mm_strdup(MM_ALLOC *allochandle, const CHAR *src);
MMEXPORT CHAR   *_mm_strndup(MM_ALLOC *allochandle, const CHAR *src, uint length);
MMEXPORT void   *_mm_structdup(MM_ALLOC *allochandle, const void *src, const size_t size);
MMEXPORT void   *_mm_objdup(MM_ALLOC *parent, const void *src, const uint blksize);
MMEXPORT void    _mm_insertchr(CHAR *dest, const CHAR src, const uint pos);
MMEXPORT void    _mm_deletechr(CHAR *dest, uint pos);
MMEXPORT void    _mm_insertstr(CHAR *dest, const CHAR *src, const uint pos);
MMEXPORT void    _mm_splitpath(const CHAR *fullpath, CHAR *path, CHAR *fname);
MMEXPORT int     _mmstr_filecmp(const CHAR *str1, const CHAR *str2);
MMEXPORT CHAR   *_mmstr_pathcat(CHAR *dest, const CHAR *src);
MMEXPORT CHAR   *_mmstr_newext(CHAR *path, const CHAR *ext);

// meant to be for noisy debug logs.
// existing usage is inconsistent with concept of "debug" logs and so it's enabled globally for the time being.
#define _mmlogd(...)         _mmlog(__VA_ARGS__)
