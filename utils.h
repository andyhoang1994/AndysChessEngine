#pragma once

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <ostream>
#include <random>
#include <string>
#include <vector>

#include "defines.h"

void prefetch(void* addr);
void start_logger(bool b);

void dbg_hit_on(bool b);
void dbg_hit_on(bool c, bool b);
void dbg_mean_of(int v);
void dbg_print();

typedef std::chrono::milliseconds::rep TimePoint;

inline TimePoint now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
};

enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK

namespace utils {
    inline std::uint64_t rand_u64(std::uint64_t low_incl, std::uint64_t high_incl)
    {
        static std::mt19937_64 rng(now());
        std::uniform_int_distribution<std::uint64_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    inline std::uint32_t rand_u32(std::uint32_t low_incl, std::uint32_t high_incl)
    {
        static std::mt19937 rng(now());
        std::uniform_int_distribution<std::uint32_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    inline std::int32_t rand_int(std::int32_t low_incl, std::int32_t high_incl)
    {
        static std::mt19937 rng(now());
        std::uniform_int_distribution<std::int32_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    template <typename T>
    inline void remove_from_vec(T& val, std::vector<T>& vec)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
    }
}