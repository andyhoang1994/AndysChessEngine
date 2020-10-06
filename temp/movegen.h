/// Comments from Stockfish
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)

#ifndef MOVEGEN_H
#define MOVEGEN_H

	#include <string>
	#include "defines.h"
	#include "position.h"

	inline Square getFrom(Move move) { return Square((move >> 6) & 0x3f); }
	inline Square getTo(Move move) { return Square(move & 0x3f); }
	inline PromotionType getPromotionType(Move move) {
		return PromotionType(move & PROMOTION_MASK);
	}
	inline Move getMove(Square fromSquare, Square toSquare, Move moveType = NORMAL, PromotionType promotionType = PROMOTE_NONE) {
		return (toSquare | (fromSquare << 6) | moveType | promotionType);
	}

	std::string getMoveString(Move move);
	Move* generateTacticalMoves(const Position& position, Move* moveList);
	// int generateLegalEvasions(const Position& position, std::vector<Move>& moveList);
	// int generateLegalTacticalMoves(const Position& position, std::vector<Move>& moveList);
	int generateEvasions(const Position& position, Move* moveList);
	int generateLegalMoves(const Position& position, Move* moveList);
	int generateMoves(const Position& position, Move* moveList);
	bool checkPseudoLegal(const Position& position, Move move);
#endif