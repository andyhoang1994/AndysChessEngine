#ifndef BITBOARD_H
#define BITBOARD_H
	#include <bitset> // popcount for counting bits
	#include <string>
	#include "defines.h"
	Bitboard whiteKing, whiteQueens, whiteRooks, whiteBishops, whiteKnights, whitePawns;
	Bitboard blackKing, blackQueens, blackRooks, blackBishops, blackKnights, blackPawns;
	Bitboard whitePieces, blackPieces, occupiedSquares;

	constexpr Bitboard RANK_1_MASK = 0xff;
	constexpr Bitboard RANK_2_MASK = 0xff00;
	constexpr Bitboard RANK_3_MASK = 0xff0000;
	constexpr Bitboard RANK_4_MASK = 0xff000000;
	constexpr Bitboard RANK_5_MASK = 0xff00000000;
	constexpr Bitboard RANK_6_MASK = 0xff0000000000;
	constexpr Bitboard RANK_7_MASK = 0xff000000000000;
	constexpr Bitboard RANK_8_MASK = 0xff00000000000000;

	constexpr Bitboard FILE_A_MASK = 0x0101010101010101;
	constexpr Bitboard FILE_B_MASK = 0x0202020202020202;
	constexpr Bitboard FILE_C_MASK = 0x0404040404040404;
	constexpr Bitboard FILE_D_MASK = 0x0808080808080808;
	constexpr Bitboard FILE_E_MASK = 0x1010101010101010;
	constexpr Bitboard FILE_F_MASK = 0x2020202020202020;
	constexpr Bitboard FILE_G_MASK = 0x4040404040404040;
	constexpr Bitboard FILE_H_MASK = 0x8080808080808080;

	// From Winglet
	unsigned int firstOne(Bitboard bitmap);
	unsigned int lastOne(Bitboard bitmap);

#endif