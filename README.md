# SSE Sorting Networks

## Huh?

So I was trying to implement a [bitonic sorting network](https://en.wikipedia.org/wiki/Bitonic_sorter) with
SIMD intrinsics to sort an array of 8 floats. The code should look like this:

```__m128 r0 = array[0:4]
__m128 r1 = array[4:8]
for each round:
    __m128 t0 = shuffle(lo, hi, ...) # move half of the elements to be compared to a register
    __m128 t1 = shuffle(lo, hi, ...) # do the same for the other half
    __m128 min = _mm_min_ps(t0, t1)  # perform all comparisons in parallel
    __m128 max = _mm_min_ps(t0, t1)
    r0 = min                         # min/max are the new wires
    r1 = max
array[0:4] = shuffle(r0, r1, ...)
array[4:8] = shuffle(40, 41, ...)
```

This should be simple, but writing it by hand got a bit annoying because:

* I kept losing track of which SIMD component corresponded to each wire
* `shuffle` intrinsics are not generic enough and sometimes you need more than one instruction (for instance, with `_mm_shuffle_ps` the lower part of the result always comes from the first argument, and the higher part always comes from the second argument)

So instead I wrote some code that generates an optimal (?) sequence of intrinsics from a definition of the sorting network.

## TODO

* AVX
* Code assumes that for each round all elements are swapped; this is true for a bitonic sorting network, but not for an odd-even network, for instance
