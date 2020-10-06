#pragma once

#include "bitboard.h"
#include "defines.h"
#include "position.h"

constexpr Value Tempo = Value(20);

namespace Evaluator {
    int evaluate(Position& position);
}
// Score struct from Teki
struct Score {
public:
    Score() : mg(VALUE_ZERO), eg(VALUE_ZERO) {}
    Score(int val) : mg(Value(val)), eg(Value(val)) {}
    Score(int mg, int eg) : mg(Value(mg)), eg(Value(eg)) {}

    Value value(int phase, int max_phase) const;
    Value value() const;

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
    Value mg;
    Value eg;
};

struct Evaluate {
public:
    Evaluate(Position& position) : position(position) {}
    Value value();
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

inline Value Score::value() const { return mg; }
inline Value Score::value(int phase, int max_phase) const { return Value(((mg * phase) + (eg * (max_phase - phase))) / max_phase); }

inline const Score Score::operator-() const { return Score(-mg, -eg); }

inline const Score Score::operator+(const Score& rhs) const { return Score(Value(mg + rhs.mg), Value(eg + rhs.eg)); }
inline const Score Score::operator-(const Score& rhs) const { return Score(Value(mg - rhs.mg), Value(eg - rhs.eg)); }
inline const Score Score::operator*(const Score& rhs) const { return Score(Value(mg * int(rhs.mg)), Value(eg * int(rhs.eg))); }
inline const Score Score::operator/(const Score& rhs) const { return Score(Value(mg / rhs.mg), Value(eg / rhs.eg)); }

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

inline Score S(Value val) { return Score(val); }
inline Score S(int mg, int eg) { return Score(mg, eg); }

// Maybe change piecePhase in the future
inline int piecePhase[6] = { 0, 1, 10, 10, 20, 40, };
inline int kingAttackValue[7] = { 0, 0, 3, 3, 4, 5, 0, };
inline Score pieceValue[7] = { Score(), S(valuePawnMg, valuePawnEg), S(valueKnightMg, valueKnightEg), S(valueBishopMg, valueBishopEg), S(valueRookMg, valueRookEg), S(valueQueenMg, valueQueenEg), Score() };
inline Score doubledPawn = S(-10, -10);
inline Score isolatedPawn = S(-10, -10);
inline Score rookOn7th = S(40, 20);
inline Score bishopPair = S(30, 100);
inline Score closeShelter = S(30, 5);
inline Score farShelter = S(20, 5);
inline Score tempo = S(20, 20);
inline Score mobilityBonus[6][32] = {
    { },
    { },
    { // Knight
        S(-92,-122), S(-42,-108), S(-21, -43), S(-8,  -8),
        S(5,   0), S(9,  17), S(16,  20), S(26,  19),
        S(39,   1),
    },
    { // Bishop
        S(-85,-173), S(-42,-117), S(-14, -57), S(-5, -24),
        S(5, -12), S(12,   5), S(14,  17), S(15,  21),
        S(14,  29), S(22,  28), S(23,  28), S(45,  13),
        S(51,  27), S(74, -15),
    },
    { // Rook
        S(-129,-122), S(-52,-114), S(-23, -77), S(-11, -28),
        S(-10,  -4), S(-13,  18), S(-11,  31), S(-5,  34),
        S(3,  39), S(7,  41), S(9,  50), S(16,  53),
        S(16,  58), S(33,  46), S(88,   5),
    },
    { // Queen
        S(-79,-267), S(-228,-392), S(-110,-214), S(-39,-211),
        S(-17,-160), S(-7, -88), S(0, -43), S(1, -10),
        S(6,  -3), S(8,  17), S(14,  22), S(16,  38),
        S(19,  30), S(21,  40), S(19,  41), S(17,  46),
        S(19,  44), S(11,  46), S(9,  43), S(13,  29),
        S(20,  11), S(32, -10), S(31, -31), S(28, -51),
        S(10, -65), S(13,-100), S(-52, -38), S(-29, -59),
    }
};

inline Score passedRank[RANK_COUNT] = {
    S(0, 0), S(10, 30), S(15, 30), S(15, 40), S(60, 70), S(170, 180), S(270, 260)
};

inline int kingAttackTable[100] = {
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


inline Score pieceSquareBonus[PIECE_TYPE_COUNT][SQUARE_COUNT] =  {
    { },
    { // Pawn
        S(0,   0), S(0,   0), S(0,   0), S(0,   0),
        S(0,   0), S(0,   0), S(0,   0), S(0,   0),
        S(-13,   8), S(-1,   6), S(-10,   3), S(-5,  -2),
        S(-6,   4), S(-6,   6), S(7,   5), S(-10,   6),
        S(-20,   6), S(-14,   8), S(-11,  -7), S(-6, -14),
        S(0, -12), S(-12,  -3), S(-8,   8), S(-21,   6),
        S(-13,  13), S(-7,  14), S(2, -12), S(5, -25),
        S(8, -25), S(4,  -7), S(-8,  14), S(-16,  11),
        S(-8,  19), S(-5,  12), S(-14,  -7), S(-11, -26),
        S(-6, -24), S(-12,  -7), S(-8,  14), S(-11,  14),
        S(-11,  40), S(-5,  37), S(-6,  21), S(12,  -6),
        S(14,  -4), S(-3,  25), S(-9,  42), S(-17,  39),
        S(-12, -53), S(-36, -14), S(4, -27), S(43, -41),
        S(40, -34), S(5, -23), S(-49,  -2), S(-16, -46),
        S(0,   0), S(0,   0), S(0,   0), S(0,   0),
        S(0,   0), S(0,   0), S(0,   0), S(0,   0),
    },
    { // Knight
        S(-29, -34), S(-5, -24), S(-15, -21), S(-13,  -2),
        S(-7,  -3), S(-19, -18), S(-5, -19), S(-35, -28),
        S(0,  -5), S(-7,   1), S(-2, -20), S(2,  -3),
        S(1,  -2), S(-5, -17), S(-6,  -4), S(-5,   0),
        S(8, -20), S(9,  -6), S(9,   0), S(11,  15),
        S(12,  15), S(5,  -1), S(7,  -5), S(5, -19),
        S(15,  15), S(14,  25), S(23,  33), S(23,  43),
        S(20,  46), S(20,  36), S(13,  24), S(12,  20),
        S(12,  26), S(20,  25), S(33,  43), S(26,  59),
        S(20,  58), S(33,  43), S(19,  27), S(8,  25),
        S(-23,  22), S(-7,  32), S(20,  49), S(14,  51),
        S(20,  47), S(21,  48), S(-14,  30), S(-23,  22),
        S(13,  -3), S(-8,  13), S(29,  -6), S(31,  18),
        S(31,  18), S(35,  -7), S(-15,  13), S(0,  -4),
        S(-161,  -5), S(-86,   8), S(-110,  34), S(-37,  13),
        S(-22,  14), S(-101,  41), S(-116,  19), S(-166, -17),
    },
    { // Bishop
        S(6, -21), S(1,  -3), S(-4,   2), S(1,   0),
        S(1,   5), S(-6,  -4), S(-1,  -2), S(6, -25),
        S(22, -16), S(3, -30), S(13,  -5), S(6,   4),
        S(7,   4), S(12,  -7), S(9, -31), S(22, -26),
        S(8,   0), S(19,   5), S(-3,  -5), S(16,  13),
        S(15,  14), S(-2,  -8), S(18,   1), S(15,   3),
        S(0,   7), S(9,  13), S(14,  26), S(14,  27),
        S(18,  28), S(10,  24), S(14,  13), S(0,  10),
        S(-16,  27), S(12,  25), S(-1,  31), S(13,  39),
        S(6,  40), S(4,  31), S(9,  28), S(-15,  30),
        S(-7,  23), S(-13,  39), S(-6,  19), S(-1,  34),
        S(6,  32), S(-16,  27), S(-11,  39), S(-10,  27),
        S(-50,  30), S(-40,  15), S(-9,  23), S(-30,  28),
        S(-31,  28), S(-11,  25), S(-57,  15), S(-55,  30),
        S(-58,  13), S(-58,  29), S(-109,  38), S(-98,  46),
        S(-103,  44), S(-88,  35), S(-30,  16), S(-67,  11),
    },
    { // Rook
        S(-24,  -2), S(-19,   2), S(-14,   3), S(-6,  -4),
        S(-7,  -4), S(-11,   2), S(-14,  -1), S(-18, -12),
        S(-63,   5), S(-23,  -9), S(-18,  -7), S(-11, -10),
        S(-10, -11), S(-19, -13), S(-17, -13), S(-73,   6),
        S(-36,   2), S(-18,  12), S(-25,   8), S(-13,   1),
        S(-12,   2), S(-27,   7), S(-11,  10), S(-37,   2),
        S(-28,  20), S(-19,  31), S(-20,  31), S(-7,  23),
        S(-8,  23), S(-20,  31), S(-14,  30), S(-29,  22),
        S(-15,  39), S(4,  32), S(13,  32), S(31,  25),
        S(28,  28), S(9,  32), S(11,  29), S(-12,  38),
        S(-25,  50), S(17,  33), S(-1,  45), S(26,  30),
        S(28,  28), S(0,  46), S(23,  29), S(-25,  50),
        S(-13,  32), S(-19,  38), S(-1,  31), S(13,  30),
        S(12,  29), S(-2,  30), S(-20,  42), S(-9,  32),
        S(28,  44), S(19,  52), S(-5,  62), S(1,  57),
        S(3,  59), S(-7,  62), S(26,  52), S(31,  48),
    },
    { // Queen
        S(20, -31), S(5, -24), S(10, -31), S(17, -16),
        S(17, -16), S(13, -42), S(8, -25), S(22, -38),
        S(7, -10), S(14, -19), S(21, -38), S(13,   1),
        S(16,  -2), S(23, -53), S(19, -31), S(3, -16),
        S(6,   2), S(19,  11), S(4,  33), S(1,  30),
        S(2,  29), S(4,  30), S(21,   6), S(11, -16),
        S(9,  18), S(12,  43), S(-4,  55), S(-19,  98),
        S(-18,  95), S(-6,  47), S(16,  39), S(5,  23),
        S(-6,  40), S(-8,  73), S(-17,  59), S(-29, 108),
        S(-32, 113), S(-23,  64), S(-11,  82), S(-15,  56),
        S(-24,  51), S(-21,  47), S(-28,  59), S(-17,  59),
        S(-16,  58), S(-23,  49), S(-27,  54), S(-35,  64),
        S(-11,  54), S(-59,  95), S(-12,  53), S(-47,  98),
        S(-53, 101), S(-17,  46), S(-63, 103), S(-9,  63),
        S(6,  32), S(18,  35), S(-3,  64), S(-5,  64),
        S(-9,  69), S(5,  49), S(18,  61), S(20,  40),
    },
    { // King
        S(53, -79), S(45, -54), S(-7, -14), S(-12, -34),
        S(-15, -36), S(-12, -14), S(43, -54), S(53, -82),
        S(27, -20), S(-18, -17), S(-43,   6), S(-70,  12),
        S(-69,  13), S(-48,   9), S(-18, -15), S(25, -20),
        S(-6, -28), S(5, -27), S(6,   0), S(-21,  20),
        S(-15,  18), S(7,   0), S(2, -26), S(-10, -25),
        S(-1, -35), S(88, -36), S(58,   5), S(-22,  34),
        S(-6,  30), S(45,   6), S(80, -34), S(-27, -29),
        S(30, -12), S(112, -28), S(56,  18), S(-11,  29),
        S(-4,  27), S(53,  16), S(99, -28), S(-6, -11),
        S(60, -24), S(137, -11), S(104,  10), S(32,   9),
        S(20,   9), S(92,   8), S(129, -14), S(51, -25),
        S(29, -60), S(61,  -6), S(43,   7), S(26,  -7),
        S(-19,  -1), S(37,   2), S(63,  -6), S(32, -64),
        S(-6,-131), S(68, -65), S(3, -38), S(-15, -14),
        S(-37, -25), S(-28, -28), S(62, -69), S(-49,-113),
    }
};

int moveBestCaseValue(Position* position);
int moveEstimatedValue(Position* position, Move move);
