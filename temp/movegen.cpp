//This actually makes the moves? Called by movegen to make moves?

/// Comments from Stockfish
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
/*
	enum MoveType {
		NORMAL,
		PROMOTION = 1 << 15,
		ENPASSANT = 2 << 15,
		CASTLING = 3 << 15,
	};
*/

#include "bitboard.h"
#include "movegen.h"
#include "position.h"
#include "utils.h"
#include <algorithm>

// Used for display purposes, not for finding moves
std::string getMoveString(Move move) {
	Move from = getFrom(move);
	Move to = getTo(move);

	std::string newMove;
	newMove.push_back(getFile(Square(from)) + 'a');
	newMove.push_back(getRank(Square(from)) + '1');

	newMove.push_back(getFile(Square(to)) + 'a');
	newMove.push_back(getRank(Square(to)) + '1');

	int promotion = getPromotionType(move);
	switch(promotion) {
		case PROMOTE_TO_QUEEN:
			newMove.push_back('q');
			break;
		case PROMOTE_TO_KNIGHT:
			newMove.push_back('n');
			break;
		case PROMOTE_TO_BISHOP:
			newMove.push_back('b');
			break;
		case PROMOTE_TO_ROOK:
			newMove.push_back('r');
			break;
		default:
			break;
	}

	return newMove;
}

// Adds promotions to moveList
Move* getPromotions(Square fromSquare, Square toSquare, Move* moveList) {
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_QUEEN);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_KNIGHT);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_ROOK);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_BISHOP);

	return moveList;
}

// Iterates through all the pieces types to find all possible capture type moves
Move* generatePieceCaptures(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Bitboard occupied = position.getOccupied();
	Bitboard themBoard = position.getBitboardColour(them);

	for(PieceType pieceType = KING; pieceType >= KNIGHT; --pieceType) {
		Bitboard currentPiece = position.getBitboard(PieceType(pieceType), position.getSide());
	
		while(currentPiece) {
			Square fromSquare = popLsb(currentPiece);
			Bitboard captures = (lookups::attacks(pieceType, fromSquare, occupied) & themBoard);
			while(captures) {
				Square toSquare = popLsb(captures);
				*(moveList++) = getMove(fromSquare, toSquare);
			}
		}
	}

	return moveList;
}

Move* generatePieceQuiets(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Bitboard occupied = position.getOccupied();
	Bitboard vacant = ~occupied;

	for(PieceType pieceType = KNIGHT; pieceType <= KING; ++pieceType) {
		Bitboard pieces = position.getBitboard(pieceType, us);

		while(pieces) {
			Square fromSquare = popLsb(pieces);
			Bitboard possibleMoves = lookups::attacks(pieceType, fromSquare, occupied, us) & vacant;
			while(possibleMoves) {
				Square toSquare = popLsb(possibleMoves);
				*(moveList++) = getMove(fromSquare, toSquare);
			}
		}
	}

	return moveList;
}

Move* generatePawnCaptures(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Direction up = pawnPush(us);
	Direction upLeft = (us == WHITE ? NORTH_WEST : SOUTH_EAST);
	Direction upRight = (us == WHITE ? NORTH_EAST : SOUTH_WEST);
	Bitboard pawns = position.getBitboard(PAWN, us);

	Bitboard emptySquares = ~position.getOccupied();
	Bitboard rank7Mask = (us == WHITE ? RANK_7_MASK : RANK_2_MASK);
	Bitboard rank2Mask = (us == WHITE ? RANK_2_MASK : RANK_7_MASK);
	Bitboard pawnsOnRank7 = pawns & rank7Mask;
	Bitboard pawnsOnRank2 = pawns & rank2Mask;
	Bitboard pawnsBeforeRank7 = pawns & ~rank7Mask;

	Bitboard leftCapture = shift(pawnsBeforeRank7, upLeft) & position.getBitboardColour(them);
	Bitboard rightCapture = shift(pawnsBeforeRank7, upRight) & position.getBitboardColour(them);

	// Adds NORMAL pawn captures to moveList
	while(leftCapture) {
		Square toSquare = popLsb(leftCapture);
		*(moveList++) = getMove(toSquare - upLeft, toSquare);
	}
	while(rightCapture) {
		Square toSquare = popLsb(rightCapture);
		*(moveList++) = getMove(toSquare - upRight, toSquare);
	}

	// Adds PROMOTION pawn moves to moveList
	leftCapture = shift(pawnsOnRank7, upLeft) & position.getBitboardColour(them);
	rightCapture = shift(pawnsOnRank7, upRight) & position.getBitboardColour(them);
	/*pawnsOnRank7 &= shift(pawnsOnRank7, up) & emptySquares;

	while(pawnsOnRank7) {
		Square toSquare = popLsb(pawnsOnRank7);
		getPromotions(toSquare - up, toSquare, moveList);
	}*/
	while(leftCapture) {
		Square toSquare = popLsb(leftCapture);
		getPromotions(toSquare - upLeft, toSquare, moveList);
	}
	while(rightCapture) {
		Square toSquare = popLsb(rightCapture);
		getPromotions(toSquare - upRight, toSquare, moveList);
	}

	if(position.getEnPassantSquare() != NO_SQUARE) {
		Bitboard enPassants = pawnsBeforeRank7 & lookups::pawn(position.getEnPassantSquare(), them);

		while(enPassants) {
			Square fromSquare = popLsb(enPassants);
			*(moveList++) = getMove(fromSquare, position.getEnPassantSquare(), ENPASSANT);
		}
	}

	return moveList;
}

//clean this up since we changed doublePush
Move* generatePawnQuiets(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard vacant = ~position.getOccupied();

	Bitboard rank2Mask = (us == WHITE ? RANK_2_MASK : RANK_7_MASK);
	Bitboard rank3Mask = (us == WHITE ? RANK_3_MASK : RANK_6_MASK);
	Bitboard rank7Mask = (us == WHITE ? RANK_7_MASK : RANK_2_MASK);

	Direction up = pawnPush(us);
	Direction upTwice = up * 2;

	Bitboard pawnsOnRank2 = pawns & rank2Mask;
	Bitboard pawnsBeforeRank7 = pawns & ~rank7Mask;

	Bitboard singlePush = shift(pawnsBeforeRank7, up) & vacant;
	Bitboard doublePush = shift((singlePush & rank3Mask), up) & vacant;

	while(singlePush) {
		Square toSquare = popLsb(singlePush);
		*(moveList++) = getMove(toSquare - up, toSquare);
	}

	while(doublePush) {
		Square toSquare = popLsb(doublePush);
		*(moveList++) = getMove(toSquare - upTwice, toSquare);
	}

	return moveList;
}
Move* generateQuietPromotions(const Position& position, Move* moveList) {
	Colour us = position.getSide();

	Direction up = pawnPush(us);

	Bitboard rank7Mask = (us == WHITE ? RANK_7_MASK : RANK_2_MASK);
	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard pawnsOnRank7 = pawns & rank7Mask;

	Bitboard promotionSquares = shift(pawnsOnRank7, up) & ~position.getOccupied();
	while(promotionSquares) {
		Square promotionSquare = popLsb(promotionSquares);
		getPromotions((promotionSquare - up), promotionSquare, moveList);
	}

	return moveList;
}

Move* generateCastling(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Bitboard occupied = position.getOccupied();
	Bitboard vacant = ~occupied;

	Square kingSquare = position.getPosition(KING, us);
	int castlingRights = position.getCastlingRights();
	if(!position.checkersTo(us)) {
		if(us == WHITE) {
			if(castlingRights & WHITE_OO) {
				bool castlePath = true;

				for(int square = 0; square <= 1; ++square) {
					if((bitShift(castling::kingSide[us][square]) & vacant) && !position.attackersTo(castling::kingSide[us][square], them)) {
						continue;
					}
					else {
						castlePath = false;
					}
				}

				if(castlePath == true) {
					*(moveList++) = getMove(kingSquare, castling::kingSide[us][1], CASTLING);
				}
			}
			if(castlingRights & WHITE_OOO) {
				bool castlePath = true;

				for(int square = 0; square <= 2; ++square) {
					if(bitShift(castling::queenSide[us][square]) & vacant) {
						continue;
					}
					else {
						castlePath = false;
					}
				}

				if(castlePath == true) {
					for(int square = 1; square <= 2; ++square) {
						if(!position.attackersTo(castling::queenSide[us][square], them)) {
							continue;
						}
						else {
							castlePath = false;
						}
					}

					if(castlePath == true) {
						*(moveList++) = getMove(kingSquare, castling::queenSide[us][1], CASTLING);
					}
				}
			}
		}
		else if(us == BLACK) {
			if(castlingRights & BLACK_OO) {
				bool castlePath = true;

				for(int square = 0; square <= 1; ++square) {
					if((bitShift(castling::kingSide[us][square]) & vacant) && !position.attackersTo(castling::kingSide[us][square], them)) {
						continue;
					}
					else {
						castlePath = false;
					}
				}

				if(castlePath == true) {
					*(moveList++) = getMove(kingSquare, castling::kingSide[us][1], CASTLING);
				}
			}
			if(castlingRights & BLACK_OOO) {
				bool castlePath = true;

				for(int square = 0; square <= 2; ++square) {
					if(bitShift(castling::queenSide[us][square]) & vacant) {
						continue;
					}
					else {
						castlePath = false;
					}
				}

				if(castlePath == true) {
					for(int square = 1; square <= 2; ++square) {
						if(!position.attackersTo(castling::queenSide[us][square], them)) {
							continue;
						}
						else {
							castlePath = false;
						}
					}

					if(castlePath == true) {
						*(moveList++) = getMove(kingSquare, castling::queenSide[us][1], CASTLING);
					}
				}
			}
		}
	}

	return moveList;
}

Move* generateCheckerCaptures(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Direction down = -pawnPush(us);

	Bitboard notKingSquare = ~position.getBitboard(KING, us);
	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard occupied = position.getOccupied();
	Bitboard checkers = position.checkersTo(us);

	Square enPassantSquare = position.getEnPassantSquare();
	if(enPassantSquare != NO_SQUARE && (checkers & shift(bitShift(enPassantSquare), down))) {
		Bitboard pawnAttackers = pawns & lookups::pawn(enPassantSquare, them);

		while(pawnAttackers) {
			Square attacker = popLsb(pawnAttackers);
			*(moveList++) = getMove(attacker, enPassantSquare, ENPASSANT);
		}
	}

	while(checkers) {
		 Square checker = popLsb(checkers);
		 Piece checkerPiece = position.getPieceOnSquare(checker);

		 Bitboard attackers = position.attackersTo(checker, occupied, us) & notKingSquare;

		 while(attackers) {
			 Square attacker = popLsb(attackers);
			 // If the attacker is a pawn and gets to the 8th rank (promotes)
			 if((bitShift(attacker) & pawns) && (bitShift(checker) & RANK_8_MASK)) {
				getPromotions(attacker, checker, moveList);
			 }
			 else {
				 *(moveList++) = getMove(attacker, checker);
			 }
		 }
	}

	return moveList;
}

Move* generateCheckBlocks(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Direction down = -pawnPush(us);

	Square kingSquare = position.getPosition(KING, us);

	Rank rank4Mask = (us == WHITE ? RANK_4 : RANK_5);

	Bitboard occupied = position.getOccupied();
	Bitboard vacant = ~occupied;
	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard checkers = position.checkersTo(us);
	Bitboard movablePieces = ~(pawns | position.getBitboard(KING, us) | position.pinned(us));
	Bitboard blockerSquares = lookups::intervening_sqs(fbitscan(checkers), kingSquare);

	while(blockerSquares) {
		Square squareToBlock = popLsb(blockerSquares);

		Bitboard pawnBlockers = shift(bitShift(squareToBlock), down);
		if(pawnBlockers & pawns) {
			Square pawnFrom = fbitscan(pawnBlockers);

			if(bitShift(pawnFrom) & RANK_8_MASK) {
				getPromotions(pawnFrom, squareToBlock, moveList);
			}
			else {
				*(moveList++) = getMove(pawnFrom, squareToBlock);
			}
		}
		// Check if blockerSquare is on Rank 4, if it is, check if we can do a double push for a pawn on Rank 2
		else if((getRank(squareToBlock) ==  rank4Mask) && (pawnBlockers & vacant) && (shift(pawnBlockers, down) & pawns)) {
			*(moveList++) = getMove(fbitscan(shift(pawnBlockers, down)), squareToBlock);
		}

		Bitboard blockerCandidates = position.attackersTo(squareToBlock, occupied, us) & movablePieces;
		while(blockerCandidates) {
			Square blockingPiece = popLsb(blockerCandidates);
			*(moveList++) = getMove(blockingPiece, squareToBlock);
		}
	}

	return moveList;
}

int generateEvasions(const Position& position, Move* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	const Move* start = moveList;

	Square kingSquare = position.getPosition(KING, us);

	Bitboard checkers = position.checkersTo(us);
	Bitboard occupied = position.getOccupied();
	Bitboard kingless = occupied ^ bitShift(kingSquare);

	Bitboard evasionSquares = lookups::king(kingSquare) & ~position.getBitboardColour(us);

	while(evasionSquares) {
		Square squareForEvasion = popLsb(evasionSquares);
		// King moves
		if(!position.attackersTo(squareForEvasion, kingless, them)) {
			*(moveList++) = getMove(kingSquare, squareForEvasion);
		}
	}

	moveList = generateCheckerCaptures(position, moveList);
	moveList = generateCheckBlocks(position, moveList);

	return moveList - start;
}

Move* generateTacticalMoves(const Position& position, Move* moveList) {
	moveList = generatePieceCaptures(position, moveList);
	moveList = generatePawnCaptures(position, moveList);
	moveList = generateQuietPromotions(position, moveList);

	return moveList;
}

Move* generateQuiets(const Position& position, Move* moveList) {
	moveList = generateCastling(position, moveList);
	moveList = generatePawnQuiets(position, moveList);
	moveList = generatePieceQuiets(position, moveList);

	return moveList;
}

int generateMoves(const Position& position, Move* moveList) {
	const Move* start = moveList;
	moveList = generateTacticalMoves(position, moveList);
	moveList = generateQuiets(position, moveList);

	return moveList - start;
}

bool Position::checkLegality(Move move) const {
	Colour us = getSide();
	Colour them = ~us;

	Square from = getFrom(move);
	Square to = getTo(move);
	Square kingSquare = getPosition(KING, us);
	if(getPieceOnSquare(from) == NO_PIECE || getPieceColour(getPieceOnSquare(from)) != us) {
		return false;
	}

	if(move & ENPASSANT) {
		Direction down = -pawnPush(us);

		Bitboard toBoard = bitShift(getEnPassantSquare());
		Bitboard capture = shift(to, down);
		Bitboard pieces = (getOccupied() ^ bitShift(from) ^ capture) | toBoard;

		Bitboard queen = getBitboard(QUEEN);
		Bitboard rookQueen = getBitboard(ROOK) | queen;
		Bitboard bishopQueen = getBitboard(BISHOP) | queen;
		Bitboard opponent = getBitboardColour(them);
		
		return !(lookups::rook(kingSquare, pieces) & (rookQueen & opponent))
			&& !(lookups::bishop(kingSquare, pieces) & (bishopQueen & opponent));
	}
	else if(from == kingSquare) {
		return (move & CASTLING) || !(attackersTo(to, them));
	}
	else {
		return !(pinned(us) & bitShift(from)) || (bitShift(to) & lookups::full_ray(from, kingSquare));
	}
}

bool checkPseudoLegal(const Position& position, Move move) {
	Colour us = position.getSide();
	Colour them = ~us;

	Square from = getFrom(move);
	Square to = getTo(move);
	Square kingSquare = position.getPosition(KING, us);
	if(position.getPieceOnSquare(from) == NO_PIECE 
		|| position.getPieceColour(position.getPieceOnSquare(from)) != us 
		|| (position.getPieceColour(position.getPieceOnSquare(from)) == position.getPieceColour(position.getPieceOnSquare(to)) && !position.getMoveType(CASTLING))) {
		return false;
	}

	if(move & ENPASSANT) {
		Direction down = -pawnPush(us);

		Bitboard toBoard = bitShift(position.getEnPassantSquare());
		Bitboard capture = shift(to, down);
		Bitboard pieces = (position.getOccupied() ^ bitShift(from) ^ capture) | toBoard;

		Bitboard queen = position.getBitboard(QUEEN);
		Bitboard rookQueen = position.getBitboard(ROOK) | queen;
		Bitboard bishopQueen = position.getBitboard(BISHOP) | queen;
		Bitboard opponent = position.getBitboardColour(them);

		return !(lookups::rook(kingSquare, pieces) & (rookQueen & opponent))
			&& !(lookups::bishop(kingSquare, pieces) & (bishopQueen & opponent));
	}
	else if(from == kingSquare) {
		return (move & CASTLING) || !(position.attackersTo(to, them));
	}

	return true;
}

/*void generateLegalEvasions(const Position& position, std::vector<Move>& moveList) {
	generateEvasions(position, moveList);

	for(uint64_t i = 0; i < moveList.size();) {
		if(!position.checkLegality(moveList[i])) {
			moveList.erase(moveList.begin() + i);
		}
		else {
			++i;
		}
	}
}
void generateLegalTacticalMoves(const Position& position, std::vector<Move>& moveList) {
	if(position.checkersTo(position.getSide())) {
		generateEvasions(position, moveList);
	}
	else {
		generateTacticalMoves(position, moveList);
	}

	for(uint64_t i = 0; i < moveList.size();) {
		if(!position.checkLegality(moveList[i])) {
			moveList.erase(moveList.begin() + i);
		}
		else {
			++i;
		}
	}
}*/

int generateLegalMoves(const Position& position, Move* moveList) {
	int size = 0;
	int moves = 0;
	Move pseudoMoves[MAX_MOVES];

	if(position.checkersTo(position.getSide())) {
		moves = generateEvasions(position, pseudoMoves);
	}
	else {
		moves = generateMoves(position, pseudoMoves);
	}

	for(int i = 0; i < moves; ++i) {
		if(position.checkLegality(pseudoMoves[i])) {
			moveList[size++] = pseudoMoves[i];
		}
	}

	return size;
	//moveList.erase(std::remove_if(moveList.begin(), moveList.end(), [&](const Move& m){ return position.checkLegality(m); }), moveList.end());
}

uint64_t Position::perft(int depth, bool root = true) const {
	Move moveList[MAX_MOVES];
	uint64_t leaves = uint64_t(0);

	int size = generateLegalMoves(*this, moveList);
	if(depth == 1) {
		return size;
	}

	for(size -= 1; size >= 0; size--) {
		Position position = *this;

		position.makeMove(moveList[size]);
		if(position.checkersTo(~position.getSide())) {
			continue;
		}
		uint64_t count = position.perft((depth - 1), false);
		leaves += count;

		if(root == true) {
			std::cout << getMoveString(moveList[size]) << ": " << count << std::endl;
		}
	}

	return leaves;
}