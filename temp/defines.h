//Contains datatypes constant values

// For enums, structs, inline functions/macros
#ifndef DEFINES_H
#define DEFINES_H


	#include "immintrin.h"
	#include "intrin.h"
	#include "utils.h"
	#include <atomic>
	#include <cstdint>
	#include <string>

	typedef uint64_t Bitboard;
	typedef uint64_t Key;
	typedef uint32_t Move;
	#define ALIGN64 alignas(64)

	typedef struct Magic Magic;
	typedef struct Undo Undo;
	typedef struct EvalTrace EvalTrace;
	typedef struct EvalInfo EvalInfo;
	typedef struct MovePicker MovePicker;
	typedef struct SearchInfo SearchInfo;
	typedef struct PVariation PVariation;
	typedef struct Thread Thread;
	typedef struct TTEntry TTEntry;
	typedef struct TTBucket TTBucket;
	typedef struct PKEntry PKEntry;
	typedef struct TTable TTable;
	typedef struct Limits Limits;
	typedef struct UCIGoStruct UCIGoStruct;

	#undef INFINITY
	constexpr int INFINITY = 99999;
	constexpr int MATE = 50000;

	constexpr int MAX_MOVES = 256;
	constexpr int MAX_PLY = 128;
	constexpr int MAX_THREADS = 64;
	constexpr int MAX_HISTORY_DEPTH = 12;
	constexpr int HISTORY_LIMIT = 8000;
	constexpr int EQUAL_BOUND = 50;


	inline bool allow_ponder = true;

	// constexpr char CMD_BUFF[MAX_CMD_BUFF];

	enum MoveType : int {
		NORMAL,
		PROMOTION = 1 << 15,
		ENPASSANT = 2 << 15,
		CASTLING = 3 << 15,
		NON_MOVE = 0,
		NULL_MOVE = 0,
	};

	enum PromotionType : int {
		PROMOTE_NONE		= 0,
		PROMOTE_TO_KNIGHT	= 1 << 12,
		PROMOTE_TO_BISHOP	= 2 << 12,
		PROMOTE_TO_ROOK		= 3 << 12,
		PROMOTE_TO_QUEEN	= 4 << 12,
		PROMOTION_MASK		= 7 << 12,
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

		valuePawnMg = 136,
		valueKnightMg = 782,
		valueBishopMg = 830,
		valueRookMg = 1289,
		valueQueenMg = 2529,

		valuePawnEg = 208,
		valueKnightEg = 865,
		valueBishopEg = 918,
		valueRookEg = 1378,
		valueQueenEg = 2687,
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
		MIDGAME,
		ENDGAME,
		PHASE_COUNT,
		MAX_PHASE = 256,
	};

	enum Bound : int {
		UPPER_BOUND,
		LOWER_BOUND,
		EXACT_BOUND,
	};

	struct TimeInputs {
		std::uint64_t nodes_searched;
		std::uint64_t tb_hits;
		time_ms start_time;
		time_ms end_time;
		std::atomic_bool time_dependent;
		std::atomic_bool analyzing;
		std::atomic_bool stop_search;
		bool limited_search;
		int max_ply;
		std::vector<uint32_t> search_moves;
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

	#define ENABLE_BASE_OPERATORS_ON(T)                                \
	constexpr T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
	constexpr T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
	constexpr T operator-(T d) { return T(-int(d)); }                  \
	inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }         \
	inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }

	#define ENABLE_INCR_OPERATORS_ON(T)                                \
	inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
	inline T& operator--(T& d) { return d = T(int(d) - 1); }

	#define ENABLE_FULL_OPERATORS_ON(T)                                \
	ENABLE_BASE_OPERATORS_ON(T)                                        \
	constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
	constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
	constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
	constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
	inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
	inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

	ENABLE_FULL_OPERATORS_ON(Value)
	ENABLE_FULL_OPERATORS_ON(Direction)

	ENABLE_INCR_OPERATORS_ON(PieceType)
	ENABLE_INCR_OPERATORS_ON(Piece)
	ENABLE_INCR_OPERATORS_ON(Square)
	ENABLE_INCR_OPERATORS_ON(File)
	ENABLE_INCR_OPERATORS_ON(Rank)

	//ENABLE_BASE_OPERATORS_ON(Score)

	#undef ENABLE_FULL_OPERATORS_ON
	#undef ENABLE_INCR_OPERATORS_ON
	#undef ENABLE_BASE_OPERATORS_ON

	inline int getSquare(int file, int rank) { return (rank << 3) ^ file; }
	inline Rank getRank(Square square) { return Rank(square >> 3); }
	inline Rank getRelativeRank(Colour colour, Square square) { return Rank((square >> 3 ) ^ (colour * 7)); }
	inline File getFile(Square square) { return File(square & 7); }
	inline Rank getRelativeFile(Colour colour, Square square) { return Rank((square >> 3) ^ (colour * 7)); }

	constexpr Colour operator~(Colour colour) {
		return Colour(colour ^ BLACK); // Toggle colour
	}
	constexpr Piece operator~(Piece pc) {
		return Piece(pc ^ 8); // Swap color of piece B_KNIGHT -> W_KNIGHT
	}

	inline Direction pawnPush(Colour col) {
		return col == WHITE ? NORTH : SOUTH;
	}

	constexpr Rank relativeRank(Colour c, Rank r) {
		return Rank(r ^ (c * 7));
	}

	constexpr Square relativeSquare(Colour c, Square s) {
		return Square(s ^ (c * 56));
	}

	constexpr MoveType getMoveType(Move move) {
		return MoveType(move & (3 << 15));
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
#endif