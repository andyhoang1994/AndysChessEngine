#pragma once

#include <cassert>
#include <iostream> // temporary for debugging
#include <string>
#include <vector>

#include "bitboard.h"
#include "defines.h"

class Position;
class Thread;

extern Square getTo(Move move);
extern Square getFrom(Move move);

constexpr int CAPTURE_MASK = 6 << 11;

namespace castling {
	constexpr Square kingSide[2][2] = { {F1, G1}, {F8, G8} };
	constexpr Square queenSide[2][3] = { {B1, C1, D1}, {B8, C8, D8} };
	inline int castlingRightsMask[SQUARE_COUNT];
}

struct Undo {
	int castleRights = 0;
	int fiftyMoveCount = 0;
	int ply = 0;
	Square enpassantSquare = NO_SQUARE;

	Key positionKey = 0;
	Square captureSquare = NO_SQUARE;
	Piece capturePiece = NO_PIECE;
};

class Position {
public:
	Position() = default; // To define the global object RootPos
	// Position(const Position&) = delete;
	Position(const Position& pos, Thread* thread) { *this = pos; thisThread = thread; }
	Position(const std::string& f, Thread* th) { init(f, th); }
	Position& operator=(const Position&); // To assign RootPos from UCI

	void init(std::string fen, Thread* thread);
	void display() const;
	void parseFen(std::string fenString);

	Bitboard attackersTo(Square square) const;
	Bitboard attackersTo(Square square, Colour side) const;
	Bitboard attackersTo(Square square, Bitboard occupied) const;
	Bitboard attackersTo(Square, Bitboard occupied, Colour side) const;
	Bitboard checkersTo(Colour side) const;
	Bitboard pinned(Colour side) const;
	bool checkLegality(Move move) const;
	bool givesCheck(Move move) const;

	Piece getPieceOnSquare(Square square) const;
	Piece getMovedPiece(Move move) const;
	MoveType getMoveType(Move move) const;
	Colour getPieceColour(Piece piece) const;
	PieceType getPieceType(Piece piece) const;
	Piece getPromotion(Move move) const;
	Colour getSide() const;
	Square getEnPassantSquare() const;
	Bitboard getBitboard(PieceType type) const;
	Bitboard getBitboard(PieceType type, Colour col) const;
	Bitboard getBitboard(Piece piece) const;
	Bitboard getBitboardColour(Colour colour) const;
	Bitboard getOccupied() const;
	Key getPositionKey() const;
	Key getPrevPositionKey() const;
	Key getExclusionKey() const;
	Square getPosition(PieceType piece, Colour col) const;
	bool checkPassedPawn(Square square) const;
	bool checkCapture(Move move) const;
	bool checkAdvancedPawnPush(Move move) const;
	bool checkNonPawnMaterial(Colour colour) const;
	int getCastlingRights() const;
	int getPly() const;
	int getFiftyMoveCount() const;

	Value see(Move move) const;
	Value seeSign(Move move) const;

	bool checkRepetition() const;
	bool checkDraw() const;

	void setEnPassant(Square square);
	Piece getCapture() const;
	void setSide(Colour colour);
	void switchSides();
	Thread* getThread() const;
	uint64_t nodes_searched() const;
	void set_nodes_searched(uint64_t n);

	void makeMove(Undo* newUndo, Move move);
	void undoMove(Undo* undo, Move move);
	bool testMove(Undo* undo, Move move);
	void makeNullMove(Undo* undo);
	void undoNullMove(Undo* undo);
	bool validateMove(Move move) const;

private:
	void clear();
	Piece board[64] = { NO_PIECE };
	Bitboard bitboardsType[PIECE_TYPE_COUNT];
	Bitboard bitboardsColour[COLOUR_COUNT];
	Colour side = WHITE;
	Thread* thisThread;

	Square enPassantSquare = NO_SQUARE;
	Piece capture = NO_PIECE;
	int fiftyMoveCount = 0;
	int ply = 0;
	int castlingRights = NO_CASTLING;
	uint64_t nodes = 0;

	Key positionKey;
	Key generatePositionKey();
	Key prevPositionKey[512];

	void resetFiftyMoveCount();
	void incrementFiftyMoveCount();
	void decrementFiftyMoveCount();

	void placePiece(Square square, Piece piece);
	void removePiece(Square square, Piece piece);
	void movePiece(Square from, Square to);
};

inline Piece Position::getPieceOnSquare(Square square) const {
	return board[square];
}

inline Piece Position::getMovedPiece(Move move) const {
	return getPieceOnSquare(getFrom(move));
}

inline MoveType Position::getMoveType(Move move) const {
	return MoveType(move & (3 << 14));
}

inline Square Position::getEnPassantSquare() const {
	return enPassantSquare;
}

inline PieceType Position::getPieceType(Piece piece) const {
	return PieceType(piece & 7);
}
inline Colour Position::getPieceColour(Piece piece) const {
	return (piece >> 3) == 0 ? WHITE : BLACK;
}

inline Piece Position::getPromotion(Move move) const {
	int promotionType = (move & PROMOTION_MASK) >> 12;

	return side == WHITE ? Piece(promotionType + KNIGHT) : Piece(promotionType + 8 + KNIGHT);
}

inline Bitboard Position::getBitboard(PieceType type) const {
	return bitboardsType[type];
}
inline Bitboard Position::getBitboard(PieceType type, Colour col) const {
	return bitboardsType[type] & bitboardsColour[col];
}
inline Bitboard Position::getBitboard(Piece piece) const {
	return bitboardsType[getPieceType(piece)] & bitboardsColour[getPieceColour(piece)];
}
inline Bitboard Position::getBitboardColour(Colour colour) const {
	return bitboardsColour[colour];
}

inline Bitboard Position::getOccupied() const {
	return bitboardsColour[WHITE] ^ bitboardsColour[BLACK];
}

inline Square Position::getPosition(PieceType piece, Colour col) const {
	return fbitscan(getBitboard(piece, col));
}

inline void Position::switchSides() {
	side = ~side;
}

inline void Position::resetFiftyMoveCount() {
	fiftyMoveCount = 0;
}

inline void Position::incrementFiftyMoveCount() {
	++fiftyMoveCount;
}

inline void Position::decrementFiftyMoveCount() {
	--fiftyMoveCount;
}

inline void Position::setEnPassant(Square square) {
	enPassantSquare = square;
}

inline void Position::setSide(Colour colour) {
	side = colour;
}

inline Key Position::getPositionKey() const {
	return positionKey;
}
inline Key Position::getPrevPositionKey() const {
	return ply ? prevPositionKey[ply - 1] : 0;
}

inline bool Position::checkCapture(Move move) const {
	return (getPieceOnSquare(getTo(move)) == NO_SQUARE && getMoveType(move) != CASTLING) || getMoveType(move) == ENPASSANT;
}

inline bool Position::checkAdvancedPawnPush(Move move) const {
	return (getPieceType(getPieceOnSquare(getFrom(move))) == PAWN) && (relativeRank(side, getRank(getFrom(move))) > RANK_4);
}

inline int Position::getCastlingRights() const {
	return castlingRights;
}
inline int Position::getPly() const {
	return ply;
}
inline int Position::getFiftyMoveCount() const {
	return fiftyMoveCount;
}

inline Piece Position::getCapture() const {
	return capture;
}

inline Thread* Position::getThread() const {
	return thisThread;
}
inline uint64_t Position::nodes_searched() const {
	return nodes;
}
inline void Position::set_nodes_searched(uint64_t n) {
	nodes = n;
}


inline void Position::placePiece(Square square, Piece piece) {
	assert(piece >= 0 && piece <= 14);
	assert(square >= 0 && square <= 63);
	Bitboard bit = bitShift(square);

	bitboardsType[getPieceType(piece)] |= bit;
	bitboardsColour[getPieceColour(piece)] |= bit;
	board[square] = piece;
}

inline void Position::removePiece(Square square, Piece piece) {
	if(getPieceOnSquare(square) == NO_PIECE) return;

	Bitboard bit = bitShift(square);

	bitboardsType[getPieceType(piece)] ^= bit;
	bitboardsColour[getPieceColour(piece)] ^= bit;
	board[square] = NO_PIECE;
}

inline void Position::movePiece(Square from, Square to) {
	Piece fromPiece = getPieceOnSquare(from);
	removePiece(from, fromPiece);
	removePiece(to, getPieceOnSquare(to));
	placePiece(to, fromPiece);
}