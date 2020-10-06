#ifndef POSITION_H
#define POSITION_H
	#include <cassert>
	#include <iostream> //temp
	#include <string>
	#include <vector>

	#include "bitboard.h"
	#include "defines.h"

	extern Square getTo(Move move);
	extern Square getFrom(Move move);

	constexpr int CAPTURE_MASK = 6 << 11;

	namespace castling {
		constexpr Square kingSide[2][2] = { {F1, G1}, {F8, G8} };
		constexpr Square queenSide[2][3] = { {B1, C1, D1}, {B8, C8, D8} };
		inline int castlingRightsMask[SQUARE_COUNT];
	}

	class Position {
		public:
			void init(std::string fen);
			void display() const;
			void parseFen(std::string fenString);
			uint64_t perft(int depth, bool root) const;

			Bitboard attackersTo(Square square) const;
			Bitboard attackersTo(Square square, Colour side) const;
			Bitboard attackersTo(Square square, Bitboard occupied) const;
			Bitboard attackersTo(Square, Bitboard occupied, Colour side) const;
			Bitboard checkersTo(Colour side) const;
			Bitboard pinned(Colour side) const;
			bool checkLegality(Move move) const;

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
			Square getPosition(PieceType piece, Colour col) const;
			bool checkPassedPawn(Square square) const;
			bool checkCapture(Move move) const;
			int getCastlingRights() const;
			int getPly() const;
			int getFiftyMoveCount() const;

			bool checkRepetition() const;
			bool checkDraw() const;

			void setEnPassant(Square square);
			void setSide(Colour colour);
			void switchSides();

			void makeMove(Move move);
			void makeNullMove();
			bool validateMove(Move move) const;
			std::pair<Move, Move> bestMove();

		private:
			void clear();
			Piece board[64] = { NO_PIECE }; // from Stockfish. Useful for display
			Bitboard bitboardsType[PIECE_TYPE_COUNT]; // both
			Bitboard bitboardsColour[COLOUR_COUNT]; // both
			int pieceCount[PIECE_COUNT]; // just stockfish
			Colour side = WHITE;
			Square enPassantSquare = NO_SQUARE;
			int fiftyMoveCount = 0;
			int ply = 0;
			int material[2] = { 0 };
			int castlingRights = NO_CASTLING;

			Key positionKey;
			std::vector<Key> prevPositionKey;
			Key generatePositionKey();

			void resetFiftyMoveCount();
			void incrementFiftyMoveCount();
			void clearPrevPositionKeys();
			void reset();

			void placePiece(Square square, Piece piece);
			void removePiece(Square square, Piece piece);
			void movePiece(Square from, Square to);

			int pieceNum[13];
			int pieceFull[2];
			int pieceMajor[2];
			int pieceMinor[2];
	};

	inline Piece Position::getPieceOnSquare(Square square) const {
		return board[square];
	}
	
	inline Piece Position::getMovedPiece(Move move) const {
		return getPieceOnSquare(getFrom(move));
	}

	inline MoveType Position::getMoveType(Move move) const {
		return MoveType(move & (3 << 15));
	}

	inline Square Position::getEnPassantSquare() const {
		return enPassantSquare;
	}

	inline PieceType Position::getPieceType(Piece piece) const { //??? Maybe later
		return PieceType(piece & 7);
	}
	inline Colour Position::getPieceColour(Piece piece) const { //??? Maybe later
		return (piece >> 3) == 0 ? WHITE : BLACK;
	}

	inline Piece Position::getPromotion(Move move) const {
		int promotionType = (move & PROMOTION_MASK) >> 12;

		return side == WHITE ? Piece(promotionType + 1) : Piece(promotionType + 9);
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

	inline int Position::getFiftyMoveCount() const {
		return fiftyMoveCount;
	}

	inline void Position::resetFiftyMoveCount() {
		fiftyMoveCount = 0;
	}

	inline void Position::incrementFiftyMoveCount() {
		++fiftyMoveCount;
	}

	inline void Position::clearPrevPositionKeys() {
		prevPositionKey.clear();
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
	
	inline bool Position::checkCapture(Move move) const {
		return (getPieceOnSquare(getTo(move)) == NO_SQUARE && getMoveType(move) != CASTLING ) || getMoveType(move) == ENPASSANT;
	}

	inline int Position::getCastlingRights() const {
		return castlingRights;
	}
	inline int Position::getPly() const {
		return ply;
	}

	inline void Position::placePiece(Square square, Piece piece) { // maybe redo these
		Bitboard bit = bitShift(square);

		bitboardsType[getPieceType(piece)] |= bit;
		bitboardsColour[getPieceColour(piece)] |= bit;
		board[square] = piece;
	}

	inline void Position::removePiece(Square square, Piece piece) { // maybe redo these
		if(getPieceOnSquare(square) == NO_PIECE) return;

		Bitboard bit = bitShift(square);

		bitboardsType[getPieceType(piece)] ^= bit;
		bitboardsColour[getPieceColour(piece)] ^= bit;
		board[square] = NO_PIECE;
	}

	inline void Position::movePiece(Square from, Square to) { //redo these
		Piece fromPiece = getPieceOnSquare(from);

		removePiece(from, fromPiece);
		removePiece(to, getPieceOnSquare(to));
		placePiece(to, fromPiece);
	}

#endif