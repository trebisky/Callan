
The libgcc project.

I actually build libm68k.a

I am pulling source from gcc-2.95 which has assembly language code for the mc68000.

Note that "coldfire" is not relevant to what I am doing (though it is mentioned in
the source).  Coldfire was a stripped down mc68000 offered by NXP for embedded systems.

I only make the subset of libgcc that I need.  I don't do anything with floating point,
double or otherwise.
