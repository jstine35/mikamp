
// ____________________________________________________________________________________
//
static void __cdecl MixStereoNormal(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SCAST  sample;

        sample = srce[himacro(index)];
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }
}

// ____________________________________________________________________________________
//
static void __cdecl MixStereoInterp(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   
        SLONG  sroot  = srce[himacro(index)];
        sroot = (SCAST)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        *dest++ += lvolsel * sroot;
        *dest++ += rvolsel * sroot;
        index  += increment;
    }
}

// ____________________________________________________________________________________
//
static void __cdecl MixSurroundNormal(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SLONG  sample;

    for (; todo; todo--)
    {   sample = lvolsel * srce[himacro(index)];
        index += increment;

        *dest++ += sample;
        *dest++ -= sample;
    }
}

// ____________________________________________________________________________________
//
static void __cdecl MixSurroundInterp(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for (; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * (SCAST)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        *dest++ += sroot;
        *dest++ -= sroot;
        index  += increment;
    }
}

// ____________________________________________________________________________________
//
static void __cdecl MixMonoNormal(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   *dest++ += lvolsel * srce[himacro(index)];
        index  += increment;
    }
}

// ____________________________________________________________________________________
//
static void __cdecl MixMonoInterp(SCAST *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * (SCAST)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));
        *dest++ += sroot;
        index  += increment;
    }
}
