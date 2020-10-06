#ifndef BITBOARD_H
#define BITBOARD_H
	#include <bitset> // popCount for counting bits
	#include <string>
	#include "defines.h"
	
	constexpr Bitboard RANK_1_MASK = uint64_t(0xff);
	constexpr Bitboard RANK_2_MASK = uint64_t(0xff00);
	constexpr Bitboard RANK_3_MASK = uint64_t(0xff0000);
	constexpr Bitboard RANK_4_MASK = uint64_t(0xff000000);
	constexpr Bitboard RANK_5_MASK = uint64_t(0xff00000000);
	constexpr Bitboard RANK_6_MASK = uint64_t(0xff0000000000);
	constexpr Bitboard RANK_7_MASK = uint64_t(0xff000000000000);
	constexpr Bitboard RANK_8_MASK = uint64_t(0xff00000000000000);

	constexpr Bitboard FILE_A_MASK = uint64_t(0x0101010101010101);
	constexpr Bitboard FILE_B_MASK = uint64_t(0x0202020202020202);
	constexpr Bitboard FILE_C_MASK = uint64_t(0x0404040404040404);
	constexpr Bitboard FILE_D_MASK = uint64_t(0x0808080808080808);
	constexpr Bitboard FILE_E_MASK = uint64_t(0x1010101010101010);
	constexpr Bitboard FILE_F_MASK = uint64_t(0x2020202020202020);
	constexpr Bitboard FILE_G_MASK = uint64_t(0x4040404040404040);
	constexpr Bitboard FILE_H_MASK = uint64_t(0x8080808080808080);

	// From Stockfish
	constexpr Bitboard shift(Bitboard b, Direction D) {
		return  D == NORTH ? b << 8 : D == SOUTH ? b >> 8
			: D == NORTH + NORTH ? b << 16 : D == SOUTH + SOUTH ? b >> 16
			: D == EAST ? (b & ~FILE_H_MASK) << 1 : D == WEST ? (b & ~FILE_A_MASK) >> 1
			: D == NORTH_EAST ? (b & ~FILE_H_MASK) << 9 : D == NORTH_WEST ? (b & ~FILE_A_MASK) << 7
			: D == SOUTH_EAST ? (b & ~FILE_H_MASK) >> 7 : D == SOUTH_WEST ? (b & ~FILE_A_MASK) >> 9
			: 0;
	}

	// From Teki
	namespace lookups
	{
		extern void init();

		extern int distance(int from, int to);
		extern Bitboard ray(int from, int to);
		extern Bitboard xray(int from, int to);
		extern Bitboard full_ray(int from, int to);
		extern Bitboard intervening_sqs(int from, int to);
		extern Bitboard adjacent_files(int square);
		extern Bitboard adjacent_sqs(int square);
		extern Bitboard file_mask(int square);
		extern Bitboard rank_mask(int square);

		extern Bitboard getNorth(int square);
		extern Bitboard getSouth(int square);
		extern Bitboard getEast(int square);
		extern Bitboard getWest(int square);
		extern Bitboard getNortheast(int square);
		extern Bitboard getNorthwest(int square);
		extern Bitboard getSoutheast(int square);
		extern Bitboard getSouthwest(int square);
		extern Bitboard getNorthRegion(int square);
		extern Bitboard getSouthRegion(int square);
		extern Bitboard getEastRegion(int square);
		extern Bitboard getWestRegion(int square);

		extern Bitboard pawn(int square, int side);
		extern Bitboard knight(int square);
		extern Bitboard bishop(int square);
		extern Bitboard rook(int square);
		extern Bitboard queen(int square);
		extern Bitboard bishop(int square, Bitboard occupancy);
		extern Bitboard rook(int square, Bitboard occupancy);
		extern Bitboard queen(int square, Bitboard occupancy);
		extern Bitboard king(int square);
		extern Bitboard attacks(int piece_type, int square, Bitboard occupancy, int side = WHITE);

		extern Bitboard getPassedPawnMask(int square);
		extern Bitboard getKingDangerZone(Colour c, Square square);
		extern std::pair<Bitboard, Bitboard> kingShelterMasks(Colour c, Square square);
	}

	inline Bitboard getBit(int shift) {
		return shift < 64 ? (Bitboard(1) << shift) : 0;
	}

	// From Winglet
	unsigned int firstOne(Bitboard bitmap);
	unsigned int lastOne(Bitboard bitmap);
	extern int MS1BTABLE[256];

#endif