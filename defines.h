// For enums, structs, inline functions/macros
#ifndef DEFINES_H
#define DEFINES_H
	#include <cstdint>
	#include <string>

	typedef uint64_t Bitboard;
	typedef uint64_t Key;

	constexpr int MAX_CMD_BUFF = 256;	// Console command input buffer
	constexpr int MAX_MOVE_BUFF = 4096;
	constexpr int CMD_BUFF_COUNT = 0;
	// constexpr char CMD_BUFF[MAX_CMD_BUFF];

	enum Piece { NO_PIECE, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK };
	const std::string PieceName[13] = { ". ", "wP", "wN", "wB", "wR", "wQ", "wK", "bP", "bN", "bB", "bR", "bQ", "bK" };
	enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
	enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
	enum Colour { WHITE, BLACK, BOTH };
	enum Castling {
		WHITE_OO = 1,
		WHITE_OOO = 2,
		BLACK_OO = 4,
		BLACK_OOO = 8,
	};
	enum Square {
		A1, B1, C1, D1, E1, F1, G1, H1,
		A2, B2, C2, D2, E2, F2, G2, H2,
		A3, B3, C3, D3, E3, F3, G3, H3,
		A4, B4, C4, D4, E4, F4, G4, H4,
		A5, B5, C5, D5, E5, F5, G5, H5,
		A6, B6, C6, D6, E6, F6, G6, H6,
		A7, B7, C7, D7, E7, F7, G7, H7,
		A8, B8, C8, D8, E8, F8, G8, H8, NO_SQUARE = 64,
	};
	enum Value {
		valuePawn = 100,
		valueKnight = 300,
		valueBishop = 300,
		valueRook = 500,
		valueQueen = 800,
		valueKing = 99999,
	};

#endif