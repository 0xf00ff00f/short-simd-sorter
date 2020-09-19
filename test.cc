#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <numeric>

void test(const std::function<void(std::array<float, 8> &)> &sort)
{
    std::array<float, 8> arr;
    std::iota(arr.begin(), arr.end(), 0);
    do {
        auto temp = arr;
        sort(temp);
#if 0
        for (float n : temp)
            std::cout << n << ' ';
        std::cout << '\n';
#endif
        assert(std::is_sorted(temp.begin(), temp.end()));
    } while (std::next_permutation(arr.begin(), arr.end()));
}

extern void sort_bitonic1(std::array<float, 8> &arr);
extern void sort_bitonic2(std::array<float, 8> &arr);

int main()
{
    test(sort_bitonic1);
    test(sort_bitonic2);
}
