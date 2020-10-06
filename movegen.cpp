#include <algorithm>

#include "bitboard.h"
#include "defines.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "uci.h"
#include "utils.h"

std::string getMoveString(Move move) {
	Move from = getFrom(move);
	Move to = getTo(move);

	std::string newMove;
	newMove.push_back(getFile(Square(from)) + 'a');
	newMove.push_back(getRank(Square(from)) + '1');

	newMove.push_back(getFile(Square(to)) + 'a');
	newMove.push_back(getRank(Square(to)) + '1');

	if(getMoveType(move) == PROMOTION) {
		switch(getPromotionType(move)) {
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
	}

	return newMove;
}

// Just for debugging
void print_bb(Bitboard bb)
{
	for(int sq = 0; sq < SQUARE_COUNT; ++sq) {
		if(sq && !(sq & 7))
			std::cout << '\n';

		if(bitShift(sq ^ 56) & bb)
			std::cout << "X  ";
		else
			std::cout << "-  ";
	}
	std::cout << std::endl;
}

// Adds promotions to moveList
inline ExtMove* getPromotions(Square fromSquare, Square toSquare, ExtMove* moveList) {
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_QUEEN);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_KNIGHT);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_ROOK);
	*(moveList++) = getMove(fromSquare, toSquare, PROMOTION, PROMOTE_TO_BISHOP);

	return moveList;
}

// Iterates through all the pieces types to find all possible capture type moves
ExtMove* generatePieceCaptures(const Position& position, ExtMove* moveList) {
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

ExtMove* generatePieceQuiets(const Position& position, ExtMove* moveList, bool checks) {
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

ExtMove* generatePawnCaptures(const Position& position, ExtMove* moveList) {
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

	while(leftCapture) {
		Square toSquare = popLsb(leftCapture);
		moveList = getPromotions(toSquare - upLeft, toSquare, moveList);
	}
	while(rightCapture) {
		Square toSquare = popLsb(rightCapture);
		moveList = getPromotions(toSquare - upRight, toSquare, moveList);
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

ExtMove* generatePawnQuiets(const Position& position, ExtMove* moveList, bool checks) {
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
ExtMove* generateQuietPromotions(const Position& position, ExtMove* moveList) {
	Colour us = position.getSide();

	Direction up = pawnPush(us);

	Bitboard rank7Mask = (us == WHITE ? RANK_7_MASK : RANK_2_MASK);
	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard pawnsOnRank7 = pawns & rank7Mask;

	Bitboard promotionSquares = shift(pawnsOnRank7, up) & ~position.getOccupied();
	while(promotionSquares) {
		Square promotionSquare = popLsb(promotionSquares);
		moveList = getPromotions((promotionSquare - up), promotionSquare, moveList);
	}

	return moveList;
}

ExtMove* generateCastling(const Position& position, ExtMove* moveList) {
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
					if((bitShift(castling::queenSide[us][square]) & vacant) && !position.attackersTo(castling::queenSide[us][square], them)) {
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
					if((bitShift(castling::queenSide[us][square]) & vacant) && !position.attackersTo(castling::queenSide[us][square], them)) {
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

	return moveList;
}

ExtMove* generateCheckerCaptures(const Position& position, ExtMove* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Direction down = -pawnPush(us);

	Bitboard notKingSquare = ~position.getBitboard(KING, us);
	Bitboard pawns = position.getBitboard(PAWN, us);
	Bitboard occupied = position.getOccupied();
	Bitboard checkers = position.checkersTo(us);

	Square enPassantSquare = position.getEnPassantSquare();
	if(enPassantSquare != NO_SQUARE && (checkers & shift(bitShift(enPassantSquare), down))) {
		Bitboard pawnAttackers = pawns & relativeBoard(us, lookups::pawn(enPassantSquare, them));

		while(pawnAttackers) {
			Square attacker = popLsb(pawnAttackers);
			*(moveList++) = getMove(attacker, enPassantSquare, ENPASSANT);
		}
	}

	while(checkers) {
		Square checker = popLsb(checkers);
		Piece checkerPiece = position.getPieceOnSquare(checker);

		Bitboard attackers = relativeBoard(us, position.attackersTo(checker, occupied, us)) & notKingSquare;

		while(attackers) {
			Square attacker = popLsb(attackers);
			// If the attacker is a pawn and gets to the 8th rank (promotes)
			if((bitShift(attacker) & pawns) && (bitShift(checker) & RANK_8_MASK)) {
				moveList = getPromotions(attacker, checker, moveList);
			}
			else {
				*(moveList++) = getMove(attacker, checker);
			}
		}
	}

	return moveList;
}

ExtMove* generateCheckBlocks(const Position& position, ExtMove* moveList) {
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
				moveList = getPromotions(pawnFrom, squareToBlock, moveList);
			}
			else {
				*(moveList++) = getMove(pawnFrom, squareToBlock);
			}
		}
		// Check if blockerSquare is on Rank 4, if it is, check if we can do a double push for a pawn on Rank 2
		else if((getRank(squareToBlock) == rank4Mask) && (pawnBlockers & vacant) && (shift(pawnBlockers, down) & pawns)) {
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

ExtMove* generateEvasions(const Position& position, ExtMove* moveList) {
	Colour us = position.getSide();
	Colour them = ~us;

	Square kingSquare = position.getPosition(KING, us);

	Bitboard checkers = position.checkersTo(us);
	Bitboard occupied = position.getOccupied();
	Bitboard kingless = occupied ^ bitShift(kingSquare);

	Bitboard evasionSquares = lookups::king(kingSquare) & ~position.getBitboardColour(us);

	while(evasionSquares) {
		Square squareForEvasion = popLsb(evasionSquares);

		if(!position.attackersTo(squareForEvasion, kingless, them)) {
			*(moveList++) = getMove(kingSquare, squareForEvasion);
		}
	}

	moveList = generateCheckerCaptures(position, moveList);
	moveList = generateCheckBlocks(position, moveList);

	return moveList;
}

ExtMove* generateTacticalMoves(const Position& position, ExtMove* moveList) {
	moveList = generatePawnCaptures(position, moveList);
	moveList = generatePieceCaptures(position, moveList);

	return moveList;
}

ExtMove* generateCaptureMoves(const Position& position, ExtMove* moveList) {
	moveList = generatePawnCaptures(position, moveList);
	moveList = generatePieceCaptures(position, moveList);

	return moveList;
}

ExtMove* generateQuietMoves(const Position& position, ExtMove* moveList, bool checks) {
	moveList = generateCastling(position, moveList);
	moveList = generatePawnQuiets(position, moveList, checks);
	moveList = generatePieceQuiets(position, moveList, checks);
	moveList = generateQuietPromotions(position, moveList);

	return moveList;
}

ExtMove* generateMoves(const Position& position, ExtMove* moveList) {
	moveList = generateTacticalMoves(position, moveList);
	moveList = generateQuietMoves(position, moveList);

	return moveList;
}

bool checkPseudoLegal(const Position& position, Move move) {
	if(move == NO_MOVE) {
		return false;
	}
	Colour us = position.getSide();
	Colour them = ~us;

	Square from = getFrom(move);
	Square to = getTo(move);
	Square kingSquare = position.getPosition(KING, us);
	if(position.getPieceOnSquare(from) == NO_PIECE
		|| position.getPieceColour(position.getPieceOnSquare(from)) != us
		|| (position.getPieceColour(position.getPieceOnSquare(from)) == position.getPieceColour(position.getPieceOnSquare(to)))) {
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
		return (getMoveType(move) == CASTLING) || !(position.attackersTo(to, them));
	}

	return true;
}

int generateLegalMoves(const Position& position, ExtMove* moveList) {
	ExtMove pseudoMoves[MAX_MOVES];
	ExtMove* end = pseudoMoves;
	ExtMove* start = pseudoMoves;
	int size = 0;

	if(position.checkersTo(position.getSide())) {
		end = generateEvasions(position, pseudoMoves);
	}
	else {
		end = generateMoves(position, pseudoMoves);
	}

	for(int i = 0; i < end - pseudoMoves; ++i) {
		if(position.checkLegality(pseudoMoves[i])) {
			moveList[size++] = pseudoMoves[i];
		}
	}

	return size;
}

bool checkTactical(Position* position, Move move) {
	return (position->getPieceOnSquare(getTo(move)) != NO_PIECE && getMoveType(move) != CASTLING)
		|| (getMoveType(move) == ENPASSANT || getMoveType(move) == PROMOTION);
}

uint64_t perft(Position position, Depth depth, bool root) {
	ExtMove moveList[MAX_MOVES];
	uint64_t leaves = uint64_t(0);
	Undo undo[1];
	int u = 0;
	int size = generateLegalMoves(position, moveList);
	if(depth == 1) {
		return size;
	}
	for(size -= 1; size >= 0; size--) {
		Position child = position;
		child.makeMove(undo, moveList[size]);
		if(child.checkersTo(~child.getSide())) {
			child.undoMove(undo, moveList[size]);
			continue;
		}

		uint64_t count = perft(child, depth - 1, false);
		leaves += count;


		if(root == true) {
			std::cout << getMoveString(moveList[size]) << ": " << count << std::endl;
		}
	}

	return leaves;
}