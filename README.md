# SSE Sorting Networks

## Huh?

So, as an exercise, I was trying to sort an array of 8 floats with SIMD intrinsics
using a [bitonic sorting network](https://en.wikipedia.org/wiki/Bitonic_sorter). The code should look like this:

```
__m128 r0 = array[0:4]
__m128 r1 = array[4:8]
for each round:
    __m128 t0 = shuffle(r0, r1, ...) # move half of the elements to be compared to a register
    __m128 t1 = shuffle(r0, r1, ...) # do the same for the other half
    __m128 min = _mm_min_ps(t0, t1)  # perform all comparisons in parallel
    __m128 max = _mm_max_ps(t0, t1)
    r0 = min                         # min/max are the new wires
    r1 = max
array[0:4] = shuffle(r0, r1, ...)
array[4:8] = shuffle(r0, r1, ...)
```

This should be simple, but writing it by hand got a bit annoying because:

* I kept losing track of which SIMD component corresponded to each wire
* `shuffle` instructions  (e.g. `_mm_shuffle_ps`) are not generic enough and sometimes you need more than one instruction

So instead I wrote some code that generates an optimal (?) sequence of intrinsics from a definition of the sorting network.

## So how does the generated code look like?

```
void sort_bitonic(std::array<float, 8>& arr)
{
    const __m128 r0 = _mm_load_ps(arr.data());
    const __m128 r1 = _mm_load_ps(arr.data() + 4);
    const __m128 r2 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(2, 1, 2, 1));
    const __m128 r3 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(3, 0, 3, 0));
    const __m128 r4 = _mm_min_ps(r2, r3);
    const __m128 r5 = _mm_max_ps(r2, r3);
    const __m128 r6 = _mm_shuffle_ps(r4, r5, _MM_SHUFFLE(2, 1, 2, 1));
    const __m128 r7 = _mm_shuffle_ps(r6, r6, _MM_SHUFFLE(1, 3, 2, 0));
    const __m128 r8 = _mm_shuffle_ps(r4, r5, _MM_SHUFFLE(3, 0, 3, 0));
    const __m128 r9 = _mm_shuffle_ps(r8, r8, _MM_SHUFFLE(3, 1, 0, 2));
    const __m128 r10 = _mm_min_ps(r7, r9);
    const __m128 r11 = _mm_max_ps(r7, r9);
    const __m128 r12 = _mm_shuffle_ps(r10, r11, _MM_SHUFFLE(2, 1, 2, 1));
    const __m128 r13 = _mm_shuffle_ps(r12, r12, _MM_SHUFFLE(3, 1, 0, 2));
    const __m128 r14 = _mm_shuffle_ps(r10, r11, _MM_SHUFFLE(3, 0, 3, 0));
    const __m128 r15 = _mm_shuffle_ps(r14, r14, _MM_SHUFFLE(3, 1, 0, 2));
    const __m128 r16 = _mm_min_ps(r13, r15);
    const __m128 r17 = _mm_max_ps(r13, r15);
    const __m128 r18 = _mm_unpacklo_ps(r17, r16);
    const __m128 r19 = _mm_unpackhi_ps(r16, r17);
    const __m128 r20 = _mm_min_ps(r18, r19);
    const __m128 r21 = _mm_max_ps(r18, r19);
    const __m128 r22 = _mm_movelh_ps(r20, r21);
    const __m128 r23 = _mm_shuffle_ps(r20, r21, _MM_SHUFFLE(3, 2, 3, 2));
    const __m128 r24 = _mm_min_ps(r22, r23);
    const __m128 r25 = _mm_max_ps(r22, r23);
    const __m128 r26 = _mm_shuffle_ps(r24, r25, _MM_SHUFFLE(2, 0, 2, 0));
    const __m128 r27 = _mm_shuffle_ps(r26, r26, _MM_SHUFFLE(3, 1, 2, 0));
    const __m128 r28 = _mm_shuffle_ps(r24, r25, _MM_SHUFFLE(3, 1, 3, 1));
    const __m128 r29 = _mm_shuffle_ps(r28, r28, _MM_SHUFFLE(3, 1, 2, 0));
    const __m128 r30 = _mm_min_ps(r27, r29);
    const __m128 r31 = _mm_max_ps(r27, r29);
    const __m128 r32 = _mm_unpacklo_ps(r30, r31);
    const __m128 r33 = _mm_unpackhi_ps(r30, r31);
    _mm_store_ps(arr.data(), r32);
    _mm_store_ps(arr.data() + 4, r33);
}
```

## TODO

* Sort 16-element arrays with AVX
* Code assumes that for each round all elements are swapped; this is true for a bitonic sorting network, but not for an odd-even network, for instance. Make it more generic.
