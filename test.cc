#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>

unsigned test(const std::function<void(std::array<float, 8>&)>& sort)
{
    const auto start = std::chrono::steady_clock::now();

    std::array<float, 8> arr;
    std::iota(arr.begin(), arr.end(), 0);
    do {
        auto temp = arr;
        sort(temp);
#if 0
        for (float n : temp)
            std::cout << n << ' ';
        std::cout << '\n';
        assert(std::is_sorted(temp.begin(), temp.end()));
#endif
    } while (std::next_permutation(arr.begin(), arr.end()));

    const auto finish = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    return static_cast<unsigned>(elapsed);
}

extern void sort_bitonic1(std::array<float, 8>& arr);
extern void sort_bitonic2(std::array<float, 8>& arr);

int main()
{
    std::cout << "bitonic1: " << test(sort_bitonic1) << " ms\n";
    std::cout << "bitonic2: " << test(sort_bitonic2) << " ms\n";
    std::cout << "std::sort: " << test([](std::array<float, 8>& arr) { std::sort(arr.begin(), arr.end()); }) << " ms\n";
}
