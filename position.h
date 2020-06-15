#ifndef POSITION_H
#define POSITION_H
	#include <string>
	#include "defines.h"

	extern Bitboard whiteKing, whiteQueens, whiteRooks, whiteBishops, whiteKnights, whitePawns;
	extern Bitboard blackKing, blackQueens, blackRooks, blackBishops, blackKnights, blackPawns;
	extern Bitboard whitePieces, blackPieces, occupiedSquares;
	extern const std::string PieceName[];
	class Position {
		public:
			void init();
			void display();
			void parseFen(const std::string& fenString);

		private:
			Piece board[64] = {NO_PIECE};
			Colour side = WHITE;
			Square enPassant = NO_SQUARE;
			int fiftyMoveCounter = 0;
			int ply = 0;
			int plyHistory = 0;
			int material[2] = { 0 };
			int castlePermisson = 0;
			bool rotatedBoard = false;
			Key positionKey;
			Key generatePositionKey(Piece[]);

			void placePiece(Square square, Piece pc);
			void removePiece(Square square);
			void movePiece(Square from, Square to);

			int pieceNum[13];
			int pieceFull[2];
			int pieceMajor[2];
			int pieceMinor[2];
	};

	inline void Position::placePiece(Square square, Piece pc) {
		board[square] = pc;
		switch(pc) {
			case wP: whitePawns |= ((uint64_t)1 << square);
			case wN: whiteKnights |= ((uint64_t)1 << square);
			case wB: whiteBishops |= ((uint64_t)1 << square);
			case wR: whiteRooks |= ((uint64_t)1 << square);
			case wQ: whiteQueens |= ((uint64_t)1 << square);
			case wK: whiteKing |= ((uint64_t)1 << square);

			case bP: blackPawns |= ((uint64_t)1 << square);
			case bN: blackKnights |= ((uint64_t)1 << square);
			case bB: blackBishops |= ((uint64_t)1 << square);
			case bR: blackRooks |= ((uint64_t)1 << square);
			case bQ: blackQueens |= ((uint64_t)1 << square);
			case bK: blackKing |= ((uint64_t)1 << square);
		}
		if(pc < 7 && pc > 0) whitePieces |= ((uint64_t)1 << square);
		else if(pc > 7)  blackPieces |= ((uint64_t)1 << square);

		occupiedSquares |= ((uint64_t)1 << square);
	}

	inline void Position::removePiece(Square square) {
		Piece pc = board[square];
		board[square] = NO_PIECE;
		switch(pc) {
			case wP: whitePawns ^= ((uint64_t)1 << square);
			case wN: whiteKnights ^= ((uint64_t)1 << square);
			case wB: whiteBishops ^= ((uint64_t)1 << square);
			case wR: whiteRooks ^= ((uint64_t)1 << square);
			case wQ: whiteQueens ^= ((uint64_t)1 << square);
			case wK: whiteKing ^= ((uint64_t)1 << square);

			case bP: blackPawns ^= ((uint64_t)1 << square);
			case bN: blackKnights ^= ((uint64_t)1 << square);
			case bB: blackBishops ^= ((uint64_t)1 << square);
			case bR: blackRooks ^= ((uint64_t)1 << square);
			case bQ: blackQueens ^= ((uint64_t)1 << square);
			case bK: blackKing ^= ((uint64_t)1 << square);
		}
		if(pc != NO_PIECE) {
			if(pc < 7 && pc > 0) whitePieces ^= ((uint64_t)1 << square);
			else if(pc > 7)  blackPieces ^= ((uint64_t)1 << square);

			occupiedSquares ^= ((uint64_t)1 << square);
		}
	}

	inline void Position::movePiece(Square from, Square to) {
		Piece toPc = board[to];

		removePiece(from);
		removePiece(to);
		placePiece(to, toPc);
	}

#endif