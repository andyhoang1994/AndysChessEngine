#pragma warning(push)
#pragma warning(disable : 26812)
#pragma warning(pop)
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include "immintrin.h"
#include "intrin.h"
#include "utils.h"

typedef uint64_t Bitboard;
typedef std::condition_variable ConditionVariable;
typedef uint64_t Key;
typedef uint16_t Move;
typedef std::mutex Mutex;

#undef INFINITY
constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 128;
constexpr int MAX_HEIGHT = 128;
constexpr int MAX_THREADS = 64;
constexpr int MAX_HISTORY_DEPTH = 12;
constexpr int HISTORY_LIMIT = 8000;
constexpr int EQUAL_BOUND = 50;

constexpr int INFINITY = 99999;
constexpr int MATE = 32000 + MAX_PLY;
constexpr int MATE_IN_MAX = MATE - MAX_PLY;
constexpr int MATED_IN_MAX = MAX_PLY - MATE;
constexpr int TBWIN = 31000 + MAX_PLY;
constexpr int TBWIN_IN_MAX = TBWIN - MAX_PLY;

inline bool allow_ponder = true;

// constexpr char CMD_BUFF[MAX_CMD_BUFF];

enum MoveType : int {
	NORMAL,
	PROMOTION = 1 << 14,
	ENPASSANT = 2 << 14,
	CASTLING = 3 << 14,
	NO_MOVE = 0,
	NULL_MOVE = 0,
};

enum PromotionType : int {
	PROMOTE_NONE		= 0,
	PROMOTE_TO_KNIGHT	= 0 << 12,
	PROMOTE_TO_BISHOP	= 1 << 12,
	PROMOTE_TO_ROOK		= 2 << 12,
	PROMOTE_TO_QUEEN	= 3 << 12,
	PROMOTION_MASK		= 3 << 12,
};

enum Piece : int {
	NO_PIECE = 0,
	wP = 1, wN, wB, wR, wQ, wK,
	bP = 9, bN, bB, bR, bQ, bK,
	PIECE_COUNT = 16
};

enum PieceType : int {
	NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
	PIECE_TYPE_COUNT = 7,
	ALL_PIECES = 6,
};

const std::string PieceName[15] = { ". ", "wP", "wN", "wB", "wR", "wQ", "wK", "", "", "bP", "bN", "bB", "bR", "bQ", "bK" };
enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_COUNT = 8 };
enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_COUNT = 8 };
enum Colour { WHITE, BLACK, COLOUR_COUNT = 2 };
enum Castling {
	NO_CASTLING = 0,
	WHITE_OO = 1,
	WHITE_OOO = 2,
	BLACK_OO = 4,
	BLACK_OOO = 8,
	ALL_CASTLING = 16,
};
enum Square : int {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	NO_SQUARE,

	SQUARE_COUNT = 64,
};
enum Value : int {
	VALUE_ZERO = 0,
	VALUE_DRAW = 0,
	VALUE_KNOWN_WIN = 10000,
	VALUE_MATE = 32000,
	VALUE_INFINITE = 32001,
	VALUE_NONE = 32002,

	VALUE_MATE_IN_MAX_PLY = VALUE_MATE - 2 * MAX_PLY,
	VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

	valueKing = 99999,

	valuePawnMg = 92,
	valueKnightMg = 421,
	valueBishopMg = 431,
	valueRookMg = 610,
	valueQueenMg = 1256,

	valuePawnEg = 135,
	valueKnightEg = 434,
	valueBishopEg = 453,
	valueRookEg = 722,
	valueQueenEg = 1391,
};

enum Direction : int {
	NORTH = 8,
	EAST = 1,
	SOUTH = -NORTH,
	WEST = -EAST,

	NORTH_EAST = NORTH + EAST,
	SOUTH_EAST = SOUTH + EAST,
	SOUTH_WEST = SOUTH + WEST,
	NORTH_WEST = NORTH + WEST
};

enum Phase : int {
	PHASE_ENDGAME,
	PHASE_MIDGAME = 128,
	MG = 0, EG = 1, PHASE_NB = 2
};

enum Bound : int {
	BOUND_NONE,
	BOUND_UPPER,
	BOUND_LOWER,
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Depth {

	ONE_PLY = 1,

	DEPTH_ZERO = 0,
	DEPTH_QS_CHECKS = 0,
	DEPTH_QS_NO_CHECKS = -1,
	DEPTH_QS_RECAPTURES = -5,

	DEPTH_NONE = -6,
	DEPTH_MAX = MAX_PLY
};

// From Stockfish

/// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
inline Square& operator-=(Square& s, Direction d) { return s = s - d; }

constexpr Square operator+(Square s, int d) { return Square(int(s) + d); }
constexpr Square operator-(Square s, int d) { return Square(int(s) - d); }
inline Square& operator+=(Square& s, int d) { return s = s + d; }
inline Square& operator-=(Square& s, int d) { return s = s - d; }

inline Colour operator++(Colour& c) { return c == WHITE ? BLACK : WHITE; }

inline Value operator+(Value v, int i) { return Value(int(v) + i); }
inline Value operator-(Value v, int i) { return Value(int(v) - i); }
inline Value& operator+=(Value& v, int i) { return v = v + i; }
inline Value& operator-=(Value& v, int i) { return v = v - i; }

inline PieceType operator+(PieceType pt, int i) { return PieceType(int(pt) + i); }
inline PieceType operator-(PieceType pt, int i) { return PieceType(int(pt) - i); }

inline Depth operator-(Depth d, int i) { return Depth(int(d) - i); }

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
constexpr T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }         \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }		   \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Direction)
ENABLE_FULL_OPERATORS_ON(Depth)
ENABLE_FULL_OPERATORS_ON(File)
ENABLE_FULL_OPERATORS_ON(Piece)
ENABLE_FULL_OPERATORS_ON(PieceType)
ENABLE_FULL_OPERATORS_ON(Rank)
ENABLE_FULL_OPERATORS_ON(Square)
ENABLE_FULL_OPERATORS_ON(Value)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

inline int getSquare(int file, int rank) { return (rank << 3) ^ file; }
inline Rank getRank(Square square) { return Rank(square >> 3); }
inline Rank relativeRank(Colour colour, Square square) { return Rank((square >> 3) ^ (colour * 7)); }
inline Rank relativeRank(Colour c, Rank r) {
	return Rank(r ^ (c * 7));
}
inline File getFile(Square square) { return File(square & 7); }
inline Rank getRelativeFile(Colour colour, Square square) { return Rank((square >> 3) ^ (colour * 7)); }

constexpr Colour operator~(Colour colour) {
	return Colour(colour ^ BLACK);
}
constexpr Piece operator~(Piece pc) {
	return Piece(pc ^ 8);
}

inline PieceType getPieceType(Piece piece) {
	return PieceType(piece & 7);
}

inline Direction pawnPush(Colour col) {
	return col == WHITE ? NORTH : SOUTH;
}

constexpr Square relativeSquare(Colour c, Square s) {
	return Square(s ^ (c * 56));
}

inline Square makeSquare(File file, Rank rank) {
	return Square((rank << 3) | file);
}

constexpr MoveType getMoveType(Move move) {
	return MoveType(move & (3 << 14));
}

inline Move fromAndTo(Square from, Square to) {
	return Move(to | (from << 6));
}

inline int popCount(Bitboard bb) {
	return _mm_popcnt_u64(bb);
}
inline Square fbitscan(Bitboard bb) {
	return Square(_tzcnt_u64(bb));
}
inline Square rbitscan(Bitboard bb) {
	return Square(63 - _lzcnt_u64(bb));
}
inline Bitboard flipBoard(Bitboard bb) {
	return bb ^ 56;
}
inline Bitboard relativeBoard(Colour c, Bitboard bb) {
	return bb ^ (56 * c);
}

inline Square popLsb(Bitboard& bitboard) {
	int s = fbitscan(bitboard);
	bitboard &= (bitboard - 1);

	return Square(s);
}

inline Bitboard bitShift(int shift) { return shift < 64 ? uint64_t(1) << shift : 0; }