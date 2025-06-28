# decNumber Patches

This folder contains patches from:

    https://github.com/dnotq/decNumber

to fix several GCC warnings.

The patches **have already been applied** to the code in this repository. They
are kept here only for reference.

Commands used to apply the patches:

    cd decNumber
    patch --binary decBasic.c ./decNumber368_patches/decBasic.c.patch
    patch --binary decNumber.c ./decNumber368_patches/decNumber.c.patch
    patch --binary decNumberLocal.h ./decNumber368_patches/decNumberLocal.h.patch
