/* Divine Entertainment Presents -->

  == The Random Bit
  == A comp-sci standard in decent random number generation

  This is a replacement random number generator for the woefully not-so-random
  Visual C random number generator.  I have tested this bitch and it *is*
  quite effectively random.  And fast.  Good stuff!

  For added coolness:
   I have encapsulated the generation code into a mini-object which makes
   this thing nice and easy to use for generating full integers.

  Notes:
   - It seems the random number sequence in the beginning (after a new seed has
     been assigned) isn't very random.  As a fix, I now automatically call the
     getbit function 30 times.  That seems to get us past that area of crappy
     generations.


Alternate Generation Schemes
-----------

#define tnrandseed  50

#define random(num) (nrand() % num)

long blnrandnext = tnrandseed;

tu16 nrand() {
  blnrandnext = (((blnrandnext * 1103515249) + 12347) % 0xFFFF);
  return blnrandnext;
}

-----------

tu16 nrand() {
   blnrandnext = ((blnrandnext * 1103515245) + 12345) );
// lower bit are no too random and only 31 bits are valid
   return (blnrandnext>>15) & 0xFFFF;
}

uint mrand( uint max ) {
   return (((((blnrandnext = ((blnrandnext * 1103515245) +
12345))>>15)&0xFFFF)*max)>>16) ;

}

-----------
*/

#include "random.h"
#include <stdlib.h>

#define INIT_BITS        29    // See notes above.

#define IB1               1
#define IB2               2
#define IB5              16
#define IB18         131072
#define MASK   (IB1+IB2+IB5)

static ulong    globseed;

// _____________________________________________________________________________________
// generates a random bit!
//
BOOL __inline rand_getbit(void)
{
    if(globseed & IB18)
    {   globseed = ((globseed - MASK) << 1) | IB1;
        return 1;
    } else
    {   globseed <<= 1;
        return 0;
    }
}

// _____________________________________________________________________________________
//
void rand_setseed(ulong seed)
{
    int  i;
    
    if(!(seed & (IB18-1))) seed += 8815;
    globseed = seed;
    for(i=0; i<INIT_BITS; i++) rand_getbit();
}

// _____________________________________________________________________________________
// Generates an integer with the specified number of bits.
// Bits are shifted on in reverse since if we do them 'normally' the numbers generated
// tend to be in 'groups' - ie, it will pick several numbers in the 10-50 range, then
// the 80-170 range, and so on.
//
long rand_getlong(uint bits)
{
    ulong retval = 0;
    uint  i;

    for(i=bits; i; i--)
        retval |= rand_getbit() << (i-1);

    return (long)retval;
}

// _____________________________________________________________________________________
// Retrieves a number in the given range.  The number of bits specified should be enough
// to produce a value that can encompass the entirety of the specified range * 2.
//
int rand_getrange_ex( DE_RAND *handle, int min, int max, int bits )
{
    return ( ( rand_getlong( bits ) * max ) / (1<<bits) ) + min;
}

// _____________________________________________________________________________________
// Retrieves a number between 0 and the maximum value specified.  The number of bits
// specified should be enough to produce a value that can encompass the entirety of the
// specified range * 2.
//
int rand_getrange( DE_RAND *handle, int max, int bits )
{
    return ( ( rand_getlong( bits ) * max ) / (1<<bits) );
}

// _____________________________________________________________________________________
// generates a random bit!  Used to generate the noise sample.
//
BOOL __inline getbit(ulong *iseed)
{
    if(*iseed & IB18)
    {   *iseed = ((*iseed - MASK) << 1) | IB1;
        return 1;
    } else
    {   *iseed <<= 1;
        return 0;
    }
}

// _____________________________________________________________________________________
//
DE_RAND *drand_create(MM_ALLOC *allochandle, ulong seed)
{
    DE_RAND  *newer_than_you;

    newer_than_you = (DE_RAND *)_mm_malloc(allochandle, sizeof(DE_RAND));
    newer_than_you->allochandle = allochandle;
    drand_setseed(newer_than_you, seed);
    return newer_than_you;
}

// _____________________________________________________________________________________
// Note that the seed is not garunteed to be the same as what you pass, so please read
// drand->seed after calling this procedure!
//
void drand_setseed( DE_RAND *Sette, ulong seed )
{
    if(Sette)
    {
        int       i;

        if( !(seed & (IB18-1)) ) seed += 8815;

        Sette->seed  = seed;
        Sette->iseed = seed;

        for(i=0; i<INIT_BITS; i++) getbit(&Sette->iseed);
    }
}

// _____________________________________________________________________________________
// generates a random bit!
//
BOOL drand_getbit( DE_RAND *hand )
{
    if( hand->iseed & IB18 )
    {
        hand->iseed = ((hand->iseed - MASK) << 1) | IB1;
        return 1;
    } else
    {
        hand->iseed <<= 1;
        return 0;
    }
}

// _____________________________________________________________________________________
// Generates an integer with the specified number of bits.
//
long drand_getlong( DE_RAND *hand, uint bits )
{
    ulong retval = 0;
    uint  i;
    
    for(i=bits; i; i--)
        retval |= getbit(&hand->iseed) << (i-1);

    return retval;
}

// _____________________________________________________________________________________
// Retrieves a number in the given range.  The number of bits specified should be enough
// to produce a value that can encompass the entirety of the specified range * 2.
//
int drand_getrange_ex( DE_RAND *handle, int min, int max, int bits )
{
    return ( ( drand_getlong( handle, bits ) * max ) / (1<<bits) ) + min;
}

// _____________________________________________________________________________________
// Retrieves a number between 0 and the maximum value specified.  The number of bits
// specified should be enough to produce a value that can encompass the entirety of the
// specified range * 2.
//
int drand_getrange( DE_RAND *handle, int max, int bits )
{
    return ( ( drand_getlong( handle, bits ) * max ) / (1<<bits) );
}

// _____________________________________________________________________________________
//
void drand_free(DE_RAND *freeme)
{
    if(freeme) _mm_free(freeme->allochandle, freeme);
}

// _____________________________________________________________________________________
//
void drand_reset(DE_RAND *resetme)
{
    resetme->iseed = resetme->seed;
}
