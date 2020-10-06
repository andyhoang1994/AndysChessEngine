//Generates bitboards

#include "bitboard.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

// Below is from Teki
int distanceVal[64][64];
Bitboard attacksRay[64][64];
Bitboard attacksXray[64][64];
Bitboard attacksFullRay[64][64];
Bitboard attacksInterveningRay[64][64];
Bitboard adjacentFiles[64];
Bitboard adjacentSquares[64];
Bitboard maskFile[64];
Bitboard maskRank[64];

Bitboard attacksPawn[2][64];
Bitboard attacksKnight[64];
Bitboard attacksBishop[64];
Bitboard attacksRook[64];
Bitboard attacksQueen[64];
Bitboard attacksKing[64];

Bitboard north[64];
Bitboard south[64];
Bitboard east[64];
Bitboard west[64];
Bitboard northeast[64];
Bitboard northwest[64];
Bitboard southeast[64];
Bitboard southwest[64];
Bitboard northRegion[64];
Bitboard southRegion[64];
Bitboard eastRegion[64];
Bitboard westRegion[64];

Bitboard passedPawnMask[64];
Bitboard kingThreats[64];
Bitboard kingShelters[64][2];

void init_non_sliders() {
    for(Square square = A1; square < SQUARE_COUNT; ++square) {
        attacksKing[square] = attacksKnight[square] = 0;
        attacksPawn[WHITE][square] = attacksPawn[BLACK][square] = 0;

        // Pawn attacks
        attacksPawn[WHITE][square] |= ((getBit(square + 7) & ~FILE_H_MASK)
            | (getBit(square + 9) & ~FILE_A_MASK))
            & ~RANK_1_MASK;
        attacksPawn[BLACK][square] |= ((getBit(square - 7) & ~FILE_A_MASK)
            | (getBit(square - 9) & ~FILE_H_MASK))
            & ~RANK_8_MASK;

        // Knight attacks
        attacksKnight[square] |= getBit(square + 17) & ~(FILE_A_MASK | RANK_2_MASK | RANK_1_MASK);
        attacksKnight[square] |= getBit(square + 15) & ~(FILE_H_MASK | RANK_2_MASK | RANK_1_MASK);
        attacksKnight[square] |= getBit(square - 17) & ~(FILE_H_MASK | RANK_7_MASK | RANK_8_MASK);
        attacksKnight[square] |= getBit(square - 15) & ~(FILE_A_MASK | RANK_7_MASK | RANK_8_MASK);
        attacksKnight[square] |= getBit(square - 10) & ~(FILE_H_MASK | FILE_G_MASK | RANK_8_MASK);
        attacksKnight[square] |= getBit(square + 6) & ~(FILE_H_MASK | FILE_G_MASK | RANK_1_MASK);
        attacksKnight[square] |= getBit(square + 10) & ~(FILE_A_MASK | FILE_B_MASK | RANK_1_MASK);
        attacksKnight[square] |= getBit(square - 6) & ~(FILE_A_MASK | FILE_B_MASK | RANK_8_MASK);

        // King attacks
        attacksKing[square] |= getBit(square + 8) & ~RANK_1_MASK;
        attacksKing[square] |= getBit(square - 8) & ~RANK_8_MASK;
        attacksKing[square] |= getBit(square + 1) & ~FILE_A_MASK;
        attacksKing[square] |= getBit(square - 1) & ~FILE_H_MASK;
        attacksKing[square] |= getBit(square + 9) & ~(FILE_A_MASK | RANK_1_MASK);
        attacksKing[square] |= getBit(square - 9) & ~(FILE_H_MASK | RANK_8_MASK);
        attacksKing[square] |= getBit(square + 7) & ~(FILE_H_MASK | RANK_1_MASK);
        attacksKing[square] |= getBit(square - 7) & ~(FILE_A_MASK | RANK_8_MASK);
    }
}

void init_pseudo_sliders()
{
    for(Square square = A1; square < SQUARE_COUNT; ++square) {
        attacksBishop[square] = lookups::getNortheast(square) | lookups::getNorthwest(square)
            | lookups::getSoutheast(square) | lookups::getSouthwest(square);
        attacksRook[square] = lookups::getNorth(square) | lookups::getSouth(square)
            | lookups::getEast(square) | lookups::getWest(square);
        attacksQueen[square] = attacksRook[square] | attacksBishop[square];
    }
}

void init_misc()
{
    Square high = NO_SQUARE;
    Square low = NO_SQUARE;
    for(Square i = A1; i < SQUARE_COUNT; ++i) {
        maskFile[i] = lookups::getNorth(i) | lookups::getSouth(i) | getBit(i);
        maskRank[i] = lookups::getEast(i) | lookups::getWest(i) | getBit(i);
        passedPawnMask[i] = 0;
        if(getFile(i) != FILE_A)
        {
            passedPawnMask[i] |= north[i - 1] | north[i];
            adjacentFiles[i] |= getBit(i - 1) | north[i - 1] | south[i - 1];
            adjacentSquares[i] |= getBit(i - 1);
        }
        if(getFile(i) != FILE_H)
        {
            passedPawnMask[i] |= north[i + 1] | north[i];
            adjacentFiles[i] |= getBit(i + 1) | north[i + 1] | south[i + 1];
            adjacentSquares[i] |= getBit(i + 1);
        }
        for(Square j = A1; j < SQUARE_COUNT; ++j) {
            distanceVal[i][j] = std::max(abs(int(getRank(i)) - int(getRank(j))),
                abs(int(getFile(i)) - int(getFile(j))));
            attacksInterveningRay[i][j] = 0;
            if(i == j)
                continue;
            high = j;
            if(i > j) {
                high = i;
                low = j;
            }
            else
                low = i;
            if(getFile(high) == getFile(low))
            {
                attacksFullRay[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | getBit(i) | getBit(j);
                attacksXray[low][high] = lookups::getNorth(low);
                attacksXray[high][low] = lookups::getSouth(high);
                attacksRay[i][j] = getBit(high);
                for(high -= 8; high >= low; high -= 8) {
                    attacksRay[i][j] |= getBit(high);
                    if(high != low)
                        attacksInterveningRay[i][j] |= getBit(high);
                }
            }
            else if(getRank(high) == getRank(low))
            {
                attacksFullRay[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | getBit(i) | getBit(j);
                attacksXray[low][high] = lookups::getEast(low);
                attacksXray[high][low] = lookups::getWest(high);
                attacksRay[i][j] = getBit(high);
                for(--high; high >= low; --high) {
                    attacksRay[i][j] |= getBit(high);
                    if(high != low)
                        attacksInterveningRay[i][j] |= getBit(high);
                }
            }
            else if(getRank(high) - getRank(low) == getFile(high) - getFile(low))
            {
                attacksFullRay[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | getBit(i) | getBit(j);
                attacksXray[low][high] = lookups::getNortheast(low);
                attacksXray[high][low] = lookups::getSouthwest(high);
                attacksRay[i][j] = getBit(high);
                for(high -= 9; high >= low; high -= 9) {
                    attacksRay[i][j] |= getBit(high);
                    if(high != low)
                        attacksInterveningRay[i][j] |= getBit(high);
                }
            }
            else if(getRank(high) - getRank(low) == getFile(low) - getFile(high))
            {
                attacksFullRay[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | getBit(i) | getBit(j);
                attacksXray[low][high] = lookups::getNorthwest(low);
                attacksXray[high][low] = lookups::getSoutheast(high);
                attacksRay[i][j] = getBit(high);
                for(high -= 7; high >= low; high -= 7) {
                    attacksRay[i][j] |= getBit(high);
                    if(high != low)
                        attacksInterveningRay[i][j] |= getBit(high);
                }
            }
        }
    }
}

void init_directions() {
    Bitboard ne, nw, se, sw;
    Bitboard n, w, s, e;
    Bitboard ne_mask, nw_mask, se_mask, sw_mask;
    Bitboard n_mask, w_mask, s_mask, e_mask;
    nw_mask = ~(RANK_1_MASK | FILE_H_MASK);
    ne_mask = ~(RANK_1_MASK | FILE_A_MASK);
    sw_mask = ~(RANK_8_MASK | FILE_H_MASK);
    se_mask = ~(RANK_8_MASK | FILE_A_MASK);
    n_mask = ~(RANK_1_MASK);
    e_mask = ~(FILE_A_MASK);
    s_mask = ~(RANK_8_MASK);
    w_mask = ~(FILE_H_MASK);

    for(Square square = A1; square < SQUARE_COUNT; ++square) {
        Bitboard sq_bb = getBit(square);

        nw = (sq_bb << 7) & nw_mask;
        ne = (sq_bb << 9) & ne_mask;
        sw = (sq_bb >> 9) & sw_mask;
        se = (sq_bb >> 7) & se_mask;
        n = (sq_bb << 8) & n_mask;
        e = (sq_bb << 1) & e_mask;
        s = (sq_bb >> 8) & s_mask;
        w = (sq_bb >> 1) & w_mask;

        for(int times = 0; times < 6; ++times) {
            nw |= (nw << 7) & nw_mask;
            ne |= (ne << 9) & ne_mask;
            sw |= (sw >> 9) & sw_mask;
            se |= (se >> 7) & se_mask;
            n |= (n << 8) & n_mask;
            e |= (e << 1) & e_mask;
            s |= (s >> 8) & s_mask;
            w |= (w >> 1) & w_mask;
        }

        north[square] = n;
        south[square] = s;
        east[square] = e;
        west[square] = w;
        northeast[square] = ne;
        northwest[square] = nw;
        southeast[square] = se;
        southwest[square] = sw;
    }
}

void init_eval_masks() {
    for(Square i = A1; i < SQUARE_COUNT; ++i) {
        kingThreats[i] =
            getBit(i)
            | lookups::king(i)
            | (lookups::king(i) >> 8);
        if(i < 56) {
            kingShelters[i][0] = getBit(i + 8)
                | (lookups::king(i) & (lookups::king(i + 8) << 8));
            if(i < 48)
                kingShelters[i][1] = kingShelters[i][0] << 8;
        }
    }
}

void init_regions() {
    Bitboard nr, wr, sr, er;
    for(Square square = A1; square < SQUARE_COUNT; ++square) {
        nr = wr = sr = er = 0ULL;
        for(int r = getRank(square) + 1; r <= RANK_8; ++r)
            nr |= lookups::rank_mask(getSquare(FILE_A, r));
        for(int r = getRank(square) - 1; r >= RANK_1; --r)
            sr |= lookups::rank_mask(getSquare(FILE_A, r));
        for(int f = getFile(square) + 1; f <= FILE_H; ++f)
            er |= lookups::file_mask(getSquare(f, RANK_1));
        for(int f = getFile(square) - 1; f >= FILE_A; --f)
            wr |= lookups::file_mask(getSquare(f, RANK_1));

        northRegion[square] = nr;
        southRegion[square] = sr;
        eastRegion[square] = er;
        westRegion[square] = wr;
    }
}

namespace lookups
{
    void init() {
        init_non_sliders();
        init_directions();
        init_pseudo_sliders();
        init_misc();
        init_eval_masks();
        init_regions();
    }

    int distance(int from, int to) { return distanceVal[from][to]; }
    Bitboard ray(int from, int to) { return attacksRay[from][to]; }
    Bitboard xray(int from, int to) { return attacksXray[from][to]; }
    Bitboard full_ray(int from, int to) { return attacksFullRay[from][to]; }
    Bitboard intervening_sqs(int from, int to) { return attacksInterveningRay[from][to]; }
    Bitboard adjacent_files(int square) { return adjacentFiles[square]; }
    Bitboard adjacent_sqs(int square) { return adjacentSquares[square]; }
    Bitboard file_mask(int square) { return maskFile[square]; }
    Bitboard rank_mask(int square) { return maskRank[square]; }

    Bitboard getNorth(int square) { return north[square]; }
    Bitboard getSouth(int square) { return south[square]; }
    Bitboard getEast(int square) { return east[square]; }
    Bitboard getWest(int square) { return west[square]; }
    Bitboard getNortheast(int square) { return northeast[square]; }
    Bitboard getNorthwest(int square) { return northwest[square]; }
    Bitboard getSoutheast(int square) { return southeast[square]; }
    Bitboard getSouthwest(int square) { return southwest[square]; }
    Bitboard getNorthRegion(int square) { return northRegion[square]; }
    Bitboard getSouthRegion(int square) { return southRegion[square]; }
    Bitboard getEastRegion(int square) { return eastRegion[square]; }
    Bitboard getWestRegion(int square) { return westRegion[square]; }

    Bitboard pawn(int square, int side) { return attacksPawn[side][square]; }
    Bitboard knight(int square) { return attacksKnight[square]; }
    Bitboard bishop(int square) { return attacksBishop[square]; }
    Bitboard rook(int square) { return attacksRook[square]; }
    Bitboard queen(int square) { return attacksQueen[square]; }
    Bitboard king(int square) { return attacksKing[square]; }

    Bitboard bishop(int square, Bitboard occupancy)
    {
        Bitboard atk = bishop(square);
        Bitboard nw_blockers = (getNorthwest(square) & occupancy) | getBit(A8);
        Bitboard ne_blockers = (getNortheast(square) & occupancy) | getBit(H8);
        Bitboard sw_blockers = (getSouthwest(square) & occupancy) | getBit(A1);
        Bitboard se_blockers = (getSoutheast(square) & occupancy) | getBit(H1);

        atk ^= getNorthwest(fbitscan(nw_blockers));
        atk ^= getNortheast(fbitscan(ne_blockers));
        atk ^= getSouthwest(rbitscan(sw_blockers));
        atk ^= getSoutheast(rbitscan(se_blockers));

        return atk;
    }

    Bitboard rook(int square, Bitboard occupancy)
    {
        Bitboard atk = rook(square);
        Bitboard n_blockers = (getNorth(square) & occupancy) | getBit(H8);
        Bitboard s_blockers = (getSouth(square) & occupancy) | getBit(A1);
        Bitboard e_blockers = (getEast(square) & occupancy) | getBit(H8);
        Bitboard w_blockers = (getWest(square) & occupancy) | getBit(A1);

        atk ^= getNorth(fbitscan(n_blockers));
        atk ^= getSouth(rbitscan(s_blockers));
        atk ^= getEast(fbitscan(e_blockers));
        atk ^= getWest(rbitscan(w_blockers));

        return atk;
    }

    Bitboard queen(int square, Bitboard occupancy)
    {
        Bitboard atk = queen(square);
        Bitboard nw_blockers = (getNorthwest(square) & occupancy) | getBit(A8);
        Bitboard ne_blockers = (getNortheast(square) & occupancy) | getBit(H8);
        Bitboard sw_blockers = (getSouthwest(square) & occupancy) | getBit(A1);
        Bitboard se_blockers = (getSoutheast(square) & occupancy) | getBit(H1);
        Bitboard n_blockers = (getNorth(square) & occupancy) | getBit(H8);
        Bitboard s_blockers = (getSouth(square) & occupancy) | getBit(A1);
        Bitboard w_blockers = (getWest(square) & occupancy) | getBit(A1);
        Bitboard e_blockers = (getEast(square) & occupancy) | getBit(H8);

        atk ^= getNorthwest(fbitscan(nw_blockers));
        atk ^= getNortheast(fbitscan(ne_blockers));
        atk ^= getSouthwest(rbitscan(sw_blockers));
        atk ^= getSoutheast(rbitscan(se_blockers));
        atk ^= getNorth(fbitscan(n_blockers));
        atk ^= getSouth(rbitscan(s_blockers));
        atk ^= getWest(rbitscan(w_blockers));
        atk ^= getEast(fbitscan(e_blockers));

        return atk;
    }

    Bitboard attacks(int piece_type, int square, Bitboard occupancy, int side)
    {
        switch(piece_type) {
        case PAWN: return pawn(square, side);
        case KNIGHT: return knight(square);
        case BISHOP: return bishop(square, occupancy);
        case ROOK: return rook(square, occupancy);
        case QUEEN: return queen(square, occupancy);
        case KING: return king(square);
        default: return -1;
        }
    }

    Bitboard getPassedPawnMask(int square) { return passedPawnMask[square]; }
    Bitboard getKingDangerZone(Colour c, Square square) {
        return relativeBoard(c, kingThreats[relativeSquare(c, square)]);
    }
    std::pair<Bitboard, Bitboard> kingShelterMasks(Colour c, Square square) {
        return {
            relativeBoard(c, kingShelters[relativeSquare(c, square)][0]),
            relativeBoard(c, kingShelters[relativeSquare(c, square)][1])
        };
    }
}
