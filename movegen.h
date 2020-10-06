#pragma once

#include <string>
#include "defines.h"

class Position;

enum GenType {
	CAPTURES,
	QUIETS,
	QUIET_CHECKS,
	EVASIONS,
	NON_EVASIONS,
	LEGAL
};

struct ExtMove {
	Move move;
	Value value;

	operator Move() const { return move; }
	void operator=(Move m) { move = m; }
};

inline bool operator<(const ExtMove& f, const ExtMove& s) {
	return f.value < s.value;
}

inline Square getFrom(Move move) { return Square((move >> 6) & 0x3f); }
inline Square getTo(Move move) { return Square(move & 0x3f); }
inline PromotionType getPromotionType(Move move) {
	return PromotionType(move & PROMOTION_MASK);
}
inline Move getMove(Square fromSquare, Square toSquare, MoveType moveType = NORMAL, PromotionType promotionType = PROMOTE_NONE) {
	return Move(toSquare | (fromSquare << 6) | moveType | promotionType);
}

std::string getMoveString(Move move);
// int generateLegalEvasions(const Position& position, std::vector<Move>& moveList);
// int generateLegalTacticalMoves(const Position& position, std::vector<Move>& moveList);
ExtMove* generateTacticalMoves(const Position& position, ExtMove* moveList);
ExtMove* generateCaptureMoves(const Position& position, ExtMove* moveList);
ExtMove* generateQuietMoves(const Position& position, ExtMove* moveList, bool checks = false);
ExtMove* generateEvasions(const Position& position, ExtMove* moveList);
int generateLegalMoves(const Position& position, ExtMove* moveList);
ExtMove* generateMoves(const Position& position, ExtMove* moveList);
bool checkPseudoLegal(const Position& position, Move move);
bool checkTactical(Position* position, Move move);
void print_bb(Bitboard bb);

uint64_t perft(Position position, Depth depth, bool root = true);