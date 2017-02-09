## Overview of the Wagyu Algorithm

The Wagyu algorithm is based on the [Vatti Clipping Algorithm](vatti.md), but has several different steps in addition
to the typical steps of the Vatti algorithm. It is due to these additional steps that Wagyu is able to gaurantee that
all output geometry is valid and simple.

The complete algorithm is based roughly on the following steps represented as psuedo code below:

```
wagyu(Geometries) {
    
    LocalMinimums = build_local_minimums(Geometries);
        
    HotPixels = build_hot_pixels(LocalMimums);
    
    Rings = vatti(HotPixels, LocalMimimums);
    
    CorrectedRings = correct_topology(Rings);

    return CorrectedRings;
}
```
