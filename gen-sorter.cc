#include <algorithm>
#include <array>
#include <iostream>
#include <queue>
#include <vector>
#include <cassert>

enum Opcode : unsigned
{
    INSN_SHUFFLE,   // _mm_shuffle_ps
    INSN_UNPACKHI,  // _mm_unpackhi_ps
    INSN_UNPACKLO,  // _mm_unpacklo_ps
    INSN_MOVELH,    // _mm_movelh_ps
    INSN_MOVEHL,    // _mm_movehl_ps
};

struct Insn
{
    unsigned opcode;
    uint8_t r0;
    uint8_t r1;
};

using Register = std::array<int, 4>;

const unsigned registerKey(const Register& r)
{
    return r[0] | (r[1] << 3) | (r[2] << 6) | (r[3] << 9);
}

struct State
{
    std::vector<Register> registers;
    std::vector<Insn> insns;
};

void dumpInsn(const Insn& insn, int rs, int rd)
{
    const int r0 = insn.r0;
    const int r1 = insn.r1;

    std::cout << "const __m128 r" << rd << " = ";

    const auto reg = [rs, rd](int r) {
        if (r < 2) {
            return std::string("r") + std::to_string(rs + r);
        }
        return std::string("r") + std::to_string((rd - 1) + (r - 2));
    };

    switch (insn.opcode & 0x7)
    {
        case INSN_SHUFFLE:
            {
                const auto shuffleMask = insn.opcode >> 3;
                const auto lo0 = shuffleMask & 3;
                const auto lo1 = (shuffleMask >> 2) & 3;
                const auto hi0 = (shuffleMask >> 4) & 3;
                const auto hi1 = (shuffleMask >> 6) & 3;
                std::cout << "_mm_shuffle_ps(" << reg(r0) << ", " << reg(r1) << ", _MM_SHUFFLE(" << hi1 << ", " << hi0 << ", " << lo1 << ", " << lo0 << "));";
                break;
            }
        case INSN_UNPACKHI:


            std::cout << "_mm_unpackhi_ps(" << reg(r0) << ", " << reg(r1) << ");";
            break;
        case INSN_UNPACKLO:
            std::cout << "_mm_unpacklo_ps(" << reg(r0) << ", " << reg(r1) << ");";
            break;
        case INSN_MOVELH:
            std::cout << "_mm_movelh_ps(" << reg(r0) << ", " << reg(r1) << ");";
            break;
        case INSN_MOVEHL:
            std::cout << "_mm_movehl_ps(" << reg(r0) << ", " << reg(r1) << ");";
            break;
        default:
            assert(0);
            break;
    }
}

void dumpState(const State& state)
{
    const auto& insns = state.insns;
    for (int i = 0; i < insns.size(); ++i) {
        dumpInsn(insns[i], 0, 2);
        const auto &r = state.registers[i + 2];
        std::cout << " // {" << r[0] << ", " << r[1] << ", " << r[2] << ", " << r[3] << "}\n";
    }
}

std::vector<std::vector<Insn>> g_shuffleSequences;

void enumerateShuffles()
{
    g_shuffleSequences.resize(1 << 12);

    const Register r0 = { 0, 1, 2, 3 };
    const Register r1 = { 4, 5, 6, 7 };

    State initialState;
    initialState.registers.push_back(r0);
    initialState.registers.push_back(r1);

    std::queue<State> queue;
    queue.push(initialState);

    std::vector<bool> seen(1 << 12, false);
    seen[registerKey(r0)] = true;
    seen[registerKey(r1)] = true;

    while (!queue.empty()) {
        const auto curState = std::move(queue.front());
        queue.pop();

#if 0
        dumpState(curState);
        std::cout << "\n";
#endif

        for (uint8_t i = 0; i < curState.registers.size(); ++i)
        {
            for (uint8_t j = 0; j < curState.registers.size(); ++j)
            {
                const auto &r0 = curState.registers[i];
                const auto &r1 = curState.registers[j];

                const auto maybePushState = [&queue, &seen, &curState](const Insn& insn, const Register& result) {
                    if (seen[registerKey(result)]) {
                        return;
                    }

                    State nextState = curState;
                    nextState.registers.push_back(result);
                    nextState.insns.push_back(insn);
                    queue.push(nextState);

                    const auto key = registerKey(result);
                    seen[key] = true;
                    g_shuffleSequences[key] = nextState.insns;
                };

                {
                    // unpacklo
                    const Register r = { r0[0], r1[0], r0[1], r1[1] };
                    maybePushState({ INSN_UNPACKLO, i, j }, r);
                }

                {
                    // unpackhi
                    const Register r = { r0[2], r1[2], r0[3], r1[3] };
                    maybePushState({ INSN_UNPACKHI, i, j }, r);
                }

                {
                    // movelh
                    const Register r = { r0[0], r0[1], r1[0], r1[1] };
                    maybePushState({ INSN_MOVELH, i, j }, r);
                }

                {
                    // movehl
                    const Register r = { r1[2], r1[3], r0[2], r0[3] };
                    maybePushState({ INSN_MOVEHL, i, j }, r);
                }

                // shuffle
                for (int lo0 = 0; lo0 < 4; ++lo0)
                {
                    for (int lo1 = 0; lo1 < 4; ++lo1)
                    {
                        for (int hi0 = 0; hi0 < 4; ++hi0)
                        {
                            for (int hi1 = 0; hi1 < 4; ++hi1)
                            {
                                const Register r = { r0[lo0], r0[lo1], r1[hi0], r1[hi1] };
                                const auto shuffleMask = lo0 | (lo1 << 2) | (hi0 << 4) | (hi1 << 6);
                                maybePushState({ INSN_SHUFFLE | (shuffleMask << 3), i, j }, r);
                            }
                        }
                    }
                }
            }
        }
    }
}

using Round = std::array<std::pair<int, int>, 4>;
using SortingNetwork = std::vector<Round>;

void genSorter(const std::string& name, const SortingNetwork& sortingNetwork)
{
    std::array<int, 8> wires = { 0, 1, 2, 3, 4, 5, 6, 7 };

    const auto dumpShuffle = [](int rs, int rd, const std::array<int, 4>& mask) {
#if 1
        const auto& insns = g_shuffleSequences[registerKey(mask)];
        assert(!insns.empty());
        for (const auto &insn : insns) {
            std::cout << "    ";
            dumpInsn(insn, rs, rd);
            std::cout << '\n';
            ++rd;
        }
        return rd;
#else
        std::cout << "__m128 r" << rd << " = __builtin_shuffle(" <<
            "r" << rs << ", " << "r" << (rs + 1) << ", (mask_t) { " <<
            mask[0] << ", " << mask[1] << ", " <<
            mask[2] << ", " << mask[3] << " });\n";
        return rd + 1;
#endif
    };

    const auto makeShuffleMask = [&wires](int a, int b, int c, int d) -> std::array<int, 4> {
        const auto wireIndex = [&wires](int v) {
            const auto it = std::find(wires.begin(), wires.end(), v);
            assert(it != wires.end());
            return static_cast<int>(std::distance(wires.begin(), it));
        };
        return { wireIndex(a), wireIndex(b), wireIndex(c), wireIndex(d) };
    };

    int reg = 0;

    std::cout << "void " << name << R"((std::array<float, 8>& arr)
{
    const __m128 r0 = _mm_load_ps(arr.data());
    const __m128 r1 = _mm_load_ps(arr.data() + 4);
)";

    for (const auto& round : sortingNetwork) {
        int  rd = dumpShuffle(reg, reg + 2, makeShuffleMask(round[0].first, round[1].first, round[2].first, round[3].first));
        int lo = rd - 1;

        rd = dumpShuffle(reg, rd, makeShuffleMask(round[0].second, round[1].second, round[2].second, round[3].second));
        int hi = rd - 1;

        std::cout << "    const __m128 r" << rd << " = _mm_min_ps(r" << lo << ", r" << hi << ");\n";
        std::cout << "    const __m128 r" << (rd + 1) << " = _mm_max_ps(r" << lo << ", r" << hi << ");\n";

        for (int i = 0; i < 4; ++i) {
            wires[i] = round[i].first;
            wires[i + 4] = round[i].second;
        }

        reg = rd;
    }

    int rd = dumpShuffle(reg, reg + 2, makeShuffleMask(0, 1, 2, 3));
    int lo = rd - 1;

    rd = dumpShuffle(reg, rd, makeShuffleMask(4, 5, 6, 7));
    int hi = rd - 1;

    std::cout << "    _mm_store_ps(arr.data(), r" << lo << ");\n";
    std::cout << "    _mm_store_ps(arr.data() + 4, r" << hi << ");\n";
    std::cout << "}\n";
}

int main()
{
    enumerateShuffles();

    std::cout << R"(#include <array>
#include <xmmintrin.h>

)";

    SortingNetwork bitonic1 = {
        { { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 } } },
        { { { 0, 3 }, { 1, 2 }, { 4, 7 }, { 5, 6 } } },
        { { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 } } },
        { { { 0, 7 }, { 1, 6 }, { 2, 5 }, { 3, 4 } } },
        { { { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 } } },
        { { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 } } },
    };
    genSorter("sort_bitonic1", bitonic1);

    std::cout << '\n';

    SortingNetwork bitonic2 = {
        { { { 1, 0 }, { 2, 3 }, { 5, 4 }, { 6, 7 } } },
        { { { 2, 0 }, { 3, 1 }, { 4, 6 }, { 5, 7 } } },
        { { { 1, 0 }, { 3, 2 }, { 4, 5 }, { 6, 7 } } },
        { { { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } } },
        { { { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 } } },
        { { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 } } },
    };
    genSorter("sort_bitonic2", bitonic2);
}
