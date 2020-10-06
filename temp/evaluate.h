#ifndef EVALUATE_H
#define EVALUATE_H

#include "bitboard.h"
#include "defines.h"
#include "position.h"

namespace Evaluator {
    int evaluate(Position& position);
}
// Score struct from Teki
struct Score {
public:
    Score() : mg(0), eg(0) {}
    Score(int val) : mg(val), eg(val) {}
    Score(int mg, int eg) : mg(mg), eg(eg) {}

    int value(int phase, int max_phase) const;
    int value() const;

    const Score operator-() const;

    const Score operator+(const Score& rhs) const;
    const Score operator-(const Score& rhs) const;
    const Score operator*(const Score& rhs) const;
    const Score operator/(const Score& rhs) const;

    const Score operator+(const int rhs) const;
    const Score operator-(const int rhs) const;
    const Score operator*(const int rhs) const;
    const Score operator/(const int rhs) const;

    const Score& operator+=(const Score& rhs);
    const Score& operator-=(const Score& rhs);
    const Score& operator*=(const Score& rhs);
    const Score& operator/=(const Score& rhs);

    const Score& operator+=(const int rhs);
    const Score& operator-=(const int rhs);
    const Score& operator*=(const int rhs);
    const Score& operator/=(const int rhs);

private:
    int mg;
    int eg;
};

struct Evaluate {
public:
    Evaluate(Position& position) : position(position) {}
    int value();
private:
    int get_game_phase();
    Score pawnScore();
    Score pieceScore();
    Score passedPawnScore();
    Score kingScore();

    // Data members
    int kingAttacks[COLOUR_COUNT] = {0};
    Bitboard blockedPawns[COLOUR_COUNT] = {0};
    Bitboard passedPawns[COLOUR_COUNT] = {0};
    Bitboard attackedBy[COLOUR_COUNT][7] = {0}; // Why 8
    Bitboard mobilityArea[COLOUR_COUNT] = {0};
    Position& position;
};

inline int Score::value() const { return mg; }
inline int Score::value(int phase, int max_phase) const { return ((mg * phase) + (eg * (max_phase - phase))) / max_phase; }

inline const Score Score::operator-() const { return Score(-mg, -eg); }

inline const Score Score::operator+(const Score& rhs) const { return Score(mg + rhs.mg, eg + rhs.eg); }
inline const Score Score::operator-(const Score& rhs) const { return Score(mg - rhs.mg, eg - rhs.eg); }
inline const Score Score::operator*(const Score& rhs) const { return Score(mg * rhs.mg, eg * rhs.eg); }
inline const Score Score::operator/(const Score& rhs) const { return Score(mg / rhs.mg, eg / rhs.eg); }

inline const Score Score::operator+(const int rhs) const { return Score(mg + rhs, eg + rhs); }
inline const Score Score::operator-(const int rhs) const { return Score(mg - rhs, eg - rhs); }
inline const Score Score::operator*(const int rhs) const { return Score(mg * rhs, eg * rhs); }
inline const Score Score::operator/(const int rhs) const { return Score(mg / rhs, eg / rhs); }

inline const Score& Score::operator+=(const Score& rhs)
{
    mg += rhs.mg;
    eg += rhs.eg;
    return *this;
}
inline const Score& Score::operator-=(const Score& rhs)
{
    mg -= rhs.mg;
    eg -= rhs.eg;
    return *this;
}
inline const Score& Score::operator*=(const Score& rhs)
{
    mg *= rhs.mg;
    eg *= rhs.eg;
    return *this;
}
inline const Score& Score::operator/=(const Score& rhs)
{
    mg /= rhs.mg;
    eg /= rhs.eg;
    return *this;
}

inline const Score& Score::operator+=(const int rhs)
{
    mg += rhs;
    eg += rhs;
    return *this;
}
inline const Score& Score::operator-=(const int rhs)
{
    mg -= rhs;
    eg -= rhs;
    return *this;
}
inline const Score& Score::operator*=(const int rhs)
{
    mg *= rhs;
    eg *= rhs;
    return *this;
}
inline const Score& Score::operator/=(const int rhs)
{
    mg /= rhs;
    eg /= rhs;
    return *this;
}

inline Score S(int val) { return Score(val); }
inline Score S(int mg, int eg) { return Score(mg, eg); }

// Maybe change piecePhase in the future
inline int piecePhase[6] = { 0, 1, 10, 10, 20, 40, };
inline int kingAttackValue[7] = { 0, 0, 3, 3, 4, 5, 0, };
inline Score pieceValue[6] = { Score(), S(valuePawnMg, valuePawnEg), S(valueKnightMg, valueKnightEg), S(valueBishopMg, valueBishopEg), S(valueRookMg, valueRookEg), S(valueQueenMg, valueQueenEg) };
inline Score doubledPawn = S(-10, -10);
inline Score isolatedPawn = S(-10, -10);
inline Score rookOn7th = S(40, 20);
inline Score bishopPair = S(30, 100);
inline Score closeShelter = S(30, 5);
inline Score farShelter = S(20, 5);

inline Score mobilityBonus[6][32] = {
    { },
    { },
    {   S(-62,-81), S(-53,-56), S(-12,-30), S(-4,-14), S(3,  8), S(13, 15), // Knights
        S(22, 23), S(28, 27), S(33, 33)
    },
    {   S(-48,-59), S(-20,-23), S(16, -3), S(26, 13), S(38, 24), S(51, 42), // Bishops
        S(55, 54), S(63, 57), S(63, 65), S(68, 73), S(81, 78), S(81, 86),
        S(91, 88), S(98, 97)
    },
    {   S(-58,-76), S(-27,-18), S(-15, 28), S(-10, 55), S(-5, 69), S(-2, 82), // Rooks
        S(9,112), S(16,118), S(30,132), S(29,142), S(32,155), S(38,165),
        S(46,166), S(48,169), S(58,171)
    },
    {   S(-39,-36), S(-21,-15), S(3,  8), S(3, 18), S(14, 34), S(22, 54), // Queens
        S(28, 61), S(41, 73), S(43, 79), S(48, 92), S(56, 94), S(60,104),
        S(60,113), S(66,120), S(67,123), S(70,126), S(71,133), S(73,136),
        S(79,140), S(88,143), S(88,148), S(99,166), S(102,170), S(102,175),
        S(106,184), S(109,191), S(113,206), S(116,212)
    }
};

inline Score passedRank[RANK_COUNT] = {
    S(0, 0), S(10, 30), S(15, 30), S(15, 40), S(60, 70), S(170, 180), S(270, 260)
};

inline int kingAttackTable[100] = { // Taken from CPW(Glaurung 1.2)
      0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
      8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
     55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
    250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
    450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

inline int kingAttackWeight[6] = { 0, 2, 3, 3, 4, 5 };


namespace PST {
    inline Score pieceSquareBonus[PIECE_TYPE_COUNT][SQUARE_COUNT];
    inline Score pieceSquareHelper[7][RANK_COUNT][int(FILE_COUNT) / 2] = {
        { },
        { },
        { // Knight
           { S(-169,-105), S(-96,-74), S(-80,-46), S(-79,-18) },
           { S(-79, -70), S(-39,-56), S(-24,-15), S(-9,  6) },
           { S(-64, -38), S(-20,-33), S(4, -5), S(19, 27) },
           { S(-28, -36), S(5,  0), S(41, 13), S(47, 34) },
           { S(-29, -41), S(13,-20), S(42,  4), S(52, 35) },
           { S(-11, -51), S(28,-38), S(63,-17), S(55, 19) },
           { S(-67, -64), S(-21,-45), S(6,-37), S(37, 16) },
           { S(-200, -98), S(-80,-89), S(-53,-53), S(-32,-16) }
        },
        { // Bishop
           { S(-49,-58), S(-7,-31), S(-10,-37), S(-34,-19) },
           { S(-24,-34), S(9, -9), S(15,-14), S(1,  4) },
           { S(-9,-23), S(22,  0), S(-3, -3), S(12, 16) },
           { S(4,-26), S(9, -3), S(18, -5), S(40, 16) },
           { S(-8,-26), S(27, -4), S(13, -7), S(30, 14) },
           { S(-17,-24), S(14, -2), S(-6,  0), S(6, 13) },
           { S(-19,-34), S(-13,-10), S(7,-12), S(-11,  6) },
           { S(-47,-55), S(-7,-32), S(-17,-36), S(-29,-17) }
        },
        { // Rook
           { S(-24, 0), S(-15, 3), S(-8, 0), S(0, 3) },
           { S(-18,-7), S(-5,-5), S(-1,-5), S(1,-1) },
           { S(-19, 6), S(-10,-7), S(1, 3), S(0, 3) },
           { S(-21, 0), S(-7, 4), S(-4,-2), S(-4, 1) },
           { S(-21,-7), S(-12, 5), S(-1,-5), S(4,-7) },
           { S(-23, 3), S(-10, 2), S(1,-1), S(6, 3) },
           { S(-11,-1), S(8, 7), S(9,11), S(12,-1) },
           { S(-25, 6), S(-18, 4), S(-11, 6), S(2, 2) }
        },
        { // Queen
           { S(3,-69), S(-5,-57), S(-5,-47), S(4,-26) },
           { S(-3,-55), S(5,-31), S(8,-22), S(12, -4) },
           { S(-3,-39), S(6,-18), S(13, -9), S(7,  3) },
           { S(4,-23), S(5, -3), S(9, 13), S(8, 24) },
           { S(0,-29), S(14, -6), S(12,  9), S(5, 21) },
           { S(-4,-38), S(10,-18), S(6,-12), S(8,  1) },
           { S(-5,-50), S(6,-27), S(10,-24), S(8, -8) },
           { S(-2,-75), S(-2,-52), S(1,-43), S(-2,-36) }
        },
        { // King
           { S(272,  0), S(325, 41), S(273, 80), S(190, 93) },
           { S(277, 57), S(305, 98), S(241,138), S(183,131) },
           { S(198, 86), S(253,138), S(168,165), S(120,173) },
           { S(169,103), S(191,152), S(136,168), S(108,169) },
           { S(145, 98), S(176,166), S(112,197), S(69, 194) },
           { S(122, 87), S(159,164), S(85, 174), S(36, 189) },
           { S(87,  40), S(120, 99), S(64, 128), S(25, 141) },
           { S(64,   5), S(87,  60), S(49,  75), S(0,   75) }
        }
    };
    inline Score pawnSquareBonus[RANK_COUNT][FILE_COUNT] = { // Pawn (asymmetric distribution)
       { },
       { S(3,-10), S(3, -6), S(10, 10), S(19,  0), S(16, 14), S(19,  7), S(7, -5), S(-5,-19) },
       { S(-9,-10), S(-15,-10), S(11,-10), S(15,  4), S(32,  4), S(22,  3), S(5, -6), S(-22, -4) },
       { S(-4,  6), S(-23, -2), S(6, -8), S(20, -4), S(40,-13), S(17,-12), S(4,-10), S(-8, -9) },
       { S(13, 10), S(0,  5), S(-13,  4), S(1, -5), S(11, -5), S(-2, -5), S(-13, 14), S(5,  9) },
       { S(5, 28), S(-12, 20), S(-7, 21), S(22, 28), S(-8, 30), S(-5,  7), S(-15,  6), S(-8, 13) },
       { S(-7,  0), S(7,-11), S(-3, 12), S(-13, 21), S(5, 25), S(-16, 19), S(10,  4), S(-8,  7) }
    };

    void init();
}

#endif