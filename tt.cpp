#include <cstring>
#include <iostream>

#include "bitboard.h"
#include "tt.h"

TranspositionTable TT; // Our global transposition table

void TranspositionTable::resize(size_t mbSize) {
    size_t newClusterCount = size_t(1) << rbitscan((mbSize * 1024 * 1024) / sizeof(Cluster));

    if(newClusterCount == clusterCount)
        return;

    clusterCount = newClusterCount;

    free(mem);
    mem = calloc(clusterCount * sizeof(Cluster) + CacheLineSize - 1, 1);

    if(!mem)
    {
        std::cerr << "Failed to allocate " << mbSize
            << "MB for transposition table." << std::endl;
        exit(EXIT_FAILURE);
    }

    table = (Cluster*)((uintptr_t(mem) + CacheLineSize - 1) & ~(CacheLineSize - 1));
}

void TranspositionTable::clear() {
    std::memset(table, 0, clusterCount * sizeof(Cluster));
}

TTEntry* TranspositionTable::probe(const Key key, bool& found) const {

    TTEntry* const tte = first_entry(key);
    const uint16_t key16 = key >> 48;

    for(int i = 0; i < ClusterSize; ++i) {
        if(!tte[i].key16 || tte[i].key16 == key16) {
            if((tte[i].genBound8 & 0xFC) != generation8 && tte[i].key16)
                tte[i].genBound8 = uint8_t(generation8 | tte[i].bound());

            return found = (bool)tte[i].key16, &tte[i];
        }
    }

    TTEntry* replace = tte;

    for(int i = 1; i < ClusterSize; ++i) {
        if(replace->depth8 - ((259 + generation8 - replace->genBound8) & 0xFC) * 2 * ONE_PLY
    > tte[i].depth8 - ((259 + generation8 - tte[i].genBound8) & 0xFC) * 2 * ONE_PLY) {
            replace = &tte[i];
        }
    }

    return found = false, replace;
}

int TranspositionTable::hashfull() const {
    int count = 0;
    for(int i = 0; i < 1000 / ClusterSize; i++) {
        const TTEntry* tte = &table[i].entry[0];
        for(int j = 0; j < ClusterSize; j++) {
            if((tte[j].genBound8 & 0xFC) == generation8) {
                count++;
            }
        }
    }
    return count;
}
