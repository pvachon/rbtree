A simple, intrusive, zero-allocation Red-Black tree implementation.

Designed exclusively for systems where determinism is needed.

Licensed under a 2-clause BSD license for the sake of simplicity.

## Augmented Pointer Mode

To save 8 bytes of struct space on 64-bit systems, if bit 63 of a pointer is
not valid for a pointer on your architecture, the `rbtree` library can use that
bit in the `parent` pointer to store the color of a node in the tree.

There also will be a mode where, if you guarantee that any node will be aligned
on a 2 byte boundary, the LSB (bit 0) of the parent pointer will be used for
the same purpose. I haven't written the code to do this yet, though.

This mode is enabled for `x64_64` hosts, and other hosts as they are tested. If
you want to experiment with a host that is not on the already supported list,
try enabling the `_RB_USE_AUGMENTED_PTR` macro during compilation.
