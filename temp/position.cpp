// Holds the board position and other information

#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include "bitboard.h"
#include "utils.h"
#include "position.h"

const std::string PieceToChar(" PNBRQK  pnbrqk");
constexpr Piece Pieces[] = { wP, wN, wB, wR, wQ, wK,
							 bP, bN, bB, bR, bQ, bK, };

namespace Zobrist {
	Key psq[PIECE_COUNT][SQUARE_COUNT];
	Key enpassant[FILE_COUNT];
	Key castling[ALL_CASTLING];
	Key side;
}

void Position::parseFen(std::string fenString) {
	int piece = 0;
	char token = '\0';

	std::istringstream stream(fenString);
	stream >> std::noskipws;

	int square = 0;
	// Fill out the board based on the first part of the( FEN string
	while((stream >> token) && !isspace(token)) {
		piece = NO_PIECE;
		if(isdigit(token)) {
			square += (token - '1');
		}
		else if(token == '/') {
			continue;
		}
		else if(isalpha(token)) {
			switch(token) {
			case('p'): piece = bP; break;
			case('r'): piece = bR; break;
			case('n'): piece = bN; break;
			case('b'): piece = bB; break;
			case('q'): piece = bQ; break;
			case('k'): piece = bK; break;
			case('P'): piece = wP; break;
			case('R'): piece = wR; break;
			case('N'): piece = wN; break;
			case('B'): piece = wB; break;
			case('Q'): piece = wQ; break;
			case('K'): piece = wK; break;
			}
		}

		if(piece != NO_PIECE) {
			int sq = square ^ 56;
			placePiece(Square(sq), Piece(piece));
		}
		++square;
	}
	// Pieces are now in place but we need to add the rest of the FEN string properties
	// Go past whitespace
	stream >> token;
	if(token == 'w') side = WHITE;
	else side = BLACK;
	// Go past whitespace
	stream >> token;
	// Iterate through the castling permissions and set the correct bit for each permission available
	while((stream >> token) && !isspace(token)) {
		switch(token) {
		case('K'): castlingRights |= WHITE_OO;  break;
		case('Q'): castlingRights |= WHITE_OOO; break;
		case('k'): castlingRights |= BLACK_OO;  break;
		case('q'): castlingRights |= BLACK_OOO; break;
		}
	}
	// Go past whitespace
	stream >> token;
	if(token != '-') {
		int file = token - 'a';
		stream >> token;
		int rank = token - '1';

		enPassantSquare = Square(file + rank * 8);
	}

	stream >> std::skipws >> fiftyMoveCount;
	int totalMoves = 0;
	stream >> totalMoves;
	totalMoves = std::max(2 * (totalMoves - 1), 0) + side;

	this->positionKey = generatePositionKey();
}

void Position::init(std::string fen) {
	/*Key psq[PIECE_NB][SQUARE_NB];
	Key enpassant[FILE_NB];
	Key castling[CASTLING_RIGHT_NB];
	Key side;
	Key noPawns;

	u64 psq_keys_bb[2][6][64];
	u64 castle_keys_bb[16];
	u64 ep_keys_bb[64];
	u64 stm_key_bb;
	*/
	clear();

	for(Piece piece : Pieces) {
		for(int square = A1; square <= H8; ++square) {
			Zobrist::psq[piece][square] = utils::rand_u64(0, UINT64_MAX);
		}
	}
	for(int file = FILE_A; file <= FILE_H; ++file) {
		Zobrist::enpassant[file] = utils::rand_u64(0, UINT64_MAX);
	}
	for(int castleRight = NO_CASTLING; castleRight < ALL_CASTLING; ++castleRight) {
		Zobrist::castling[castleRight] = utils::rand_u64(0, UINT64_MAX);
	}

	for(Square square = A1; square < SQUARE_COUNT; ++square) {
		castling::castlingRightsMask[square] = 15;
	}
	castling::castlingRightsMask[A1] = 13;
	castling::castlingRightsMask[E1] = 12;
	castling::castlingRightsMask[H1] = 14;
	castling::castlingRightsMask[A8] = 7;
	castling::castlingRightsMask[E8] = 3;
	castling::castlingRightsMask[H8] = 11;


	Zobrist::side = utils::rand_u64(0, UINT64_MAX);

	parseFen(fen);
}

void Position::clear() {
	for(int i = 0; i < 6; ++i) {
		bitboardsType[i] = 0;
	}
	for(int i = 0; i < 2; ++i) {
		bitboardsColour[i] = 0;
	}
	for(int i = 0; i < 64; ++i) {
		board[i] = NO_PIECE;
	}
	enPassantSquare = NO_SQUARE;
	castlingRights = 0;
	ply = 0;
	positionKey = 0;
	prevPositionKey.clear();
	prevPositionKey.reserve(100);
}

void Position::display() const {
	for(int rank = RANK_8; rank >= RANK_1; --rank) {
		std::cout << "    +---+---+---+---+---+---+---+---+\n";
		std::cout << "  " << (rank + 1) << " |";
		for(int file = FILE_A; file <= FILE_H; ++file) {
			std::cout << " " << PieceName[board[rank * 8 + file]] << "|";
		}
		std::cout << "\n";
	}
	std::cout << "    +---+---+---+---+---+---+---+---+\n";
	std::cout << "      a   b   c   d   e   f   g   h\n";
}

Bitboard Position::attackersTo(Square square) const {
	return attackersTo(square, getOccupied());
}
Bitboard Position::attackersTo(Square square, Colour side) const {
	return attackersTo(square) & getBitboardColour(side);
}
Bitboard Position::attackersTo(Square square, Bitboard occupied) const {
	return (lookups::rook(square, occupied) & (getBitboard(ROOK) | getBitboard(QUEEN)))
		| (lookups::bishop(square, occupied) & (getBitboard(BISHOP) | getBitboard(QUEEN)))
		| (lookups::knight(square) & getBitboard(KNIGHT))
		| (lookups::pawn(square, WHITE) & getBitboard(PAWN, BLACK))
		| (lookups::pawn(square, BLACK) & getBitboard(PAWN, WHITE))
		| (lookups::king(square) & getBitboard(KING));
}
Bitboard Position::attackersTo(Square square, Bitboard occupied, Colour side) const {
	return attackersTo(square, occupied) & getBitboardColour(side);
}
Bitboard Position::checkersTo(Colour side) const {
	return attackersTo(getPosition(KING, side), ~side);
}

bool Position::checkPassedPawn(Square square) const {
	return (getPieceOnSquare(square) == PAWN)
		&& !(lookups::getPassedPawnMask(square) & getBitboard(PAWN, ~side))
		&& !(lookups::getNorth(square) & getBitboard(PAWN, side));
}

Bitboard Position::pinned(Colour side) const {
	Colour them = ~side;

	Square kingSquare = getPosition(KING, side);
	Bitboard occupied = getOccupied();
	Bitboard queen = getBitboard(QUEEN);
	Bitboard rookQueen = getBitboard(ROOK) | queen;
	Bitboard bishopQueen = getBitboard(BISHOP) | queen;
	Bitboard opponent = getBitboardColour(them);

	Bitboard pinners = (rookQueen & opponent & lookups::rook(kingSquare))
		| (bishopQueen & opponent & lookups::bishop(kingSquare));
	Bitboard pinned = 0;

	while(pinners) {
		Square square = popLsb(pinners);
		Bitboard board = lookups::intervening_sqs(square, kingSquare) & occupied;

		if(!(board & (board - 1))) {
			pinned ^= board & getBitboardColour(side);
		}
	}

	return pinned;
}

void Position::makeMove(Move move) {
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
	Square from = getFrom(move);
	Square to = getTo(move);
	Piece fromPiece = getPieceOnSquare(from);
	Piece toPiece = getPieceOnSquare(to);
	Piece rook = (side == WHITE) ? wR : bR;

	Square queenSide = (side == WHITE) ? C1 : C8;
	Square kingSide = (side == WHITE) ? G1 : G8;
	Square queenRook = (side == WHITE) ? A1 : A8;
	Square kingRook = (side == WHITE) ? H1 : H8;

	castlingRights &= castling::castlingRightsMask[from] & castling::castlingRightsMask[to];

	enPassantSquare = NO_SQUARE;

	if(getPieceType(fromPiece) == PAWN) {
		resetFiftyMoveCount();
		clearPrevPositionKeys();
	}
	else {
		incrementFiftyMoveCount();
		prevPositionKey.push_back(positionKey);
	}

	switch(getMoveType(move)) {
	case NORMAL: {
		if(getPieceType(fromPiece) == PAWN && ((to ^ from) == 16)) {
			setEnPassant(Square(to - pawnPush(side)));
		}
		movePiece(from, to);

		if(checkCapture(move)) {
			resetFiftyMoveCount();
			clearPrevPositionKeys();
		}
		break;
	}
	case PROMOTION: {
		removePiece(from, fromPiece);
		removePiece(to, toPiece);
		placePiece(to, getPromotion(move));
		break;
	}
	case ENPASSANT: {
		Square enPassantee = to - pawnPush(side);
		removePiece(enPassantee, getPieceOnSquare(enPassantee));
		movePiece(from, to);
		break;
	}
	case CASTLING: {
		if(to == queenSide) {
			removePiece(from, fromPiece);
			removePiece(queenRook, rook);
			placePiece(to, fromPiece);
			placePiece(to + 1, rook);
		}
		else if(to == kingSide) {
			removePiece(from, fromPiece);
			removePiece(kingRook, rook);
			placePiece(to, fromPiece);
			placePiece(to - 1, rook);
		}
		break;
	}
	default: {
		std::cout << "Move type error" << std::endl;
		break;
	}
	}
	this->switchSides();
	positionKey = generatePositionKey();
}

bool Position::validateMove(Move move) const {
	return getPieceColour(getPieceOnSquare(getFrom(move))) == side;
}

void Position::makeNullMove() {
	incrementFiftyMoveCount();
	setEnPassant(NO_SQUARE);
	prevPositionKey.push_back(positionKey);
	switchSides();
	positionKey = generatePositionKey();
}

Colour Position::getSide() const {
	return side;
}

Key Position::generatePositionKey() {
	/*
		Key psq[PIECE_NB][SQUARE_NB];
		Key enpassant[FILE_NB];
		Key castling[CASTLING_RIGHT_NB];
		Key side;
	*/

	Key key = Key(0);
	Piece piece = NO_PIECE;

	for(Piece piece : Pieces) {
		Bitboard bb = getBitboard(piece);

		while(bb) {
			key ^= Zobrist::psq[piece][fbitscan(bb)];
			bb &= bb - 1;
		}
	}

	if(enPassantSquare != NO_SQUARE) {
		key ^= Zobrist::enpassant[getFile(enPassantSquare)];
	}

	key ^= Zobrist::castling[getCastlingRights()];

	return key;
}

void Position::reset() {
	for(auto board : bitboardsType) {
		board = 0;
	}
	for(auto board : bitboardsColour) {
		board = 0;
	}
	enPassantSquare = NO_SQUARE;
	castlingRights = 0;
	ply = 0;
	positionKey = 0;
	prevPositionKey.clear();
	prevPositionKey.reserve(100);
}

bool Position::checkRepetition() const {

	Key currentKey = getPositionKey();
	int keyCount = prevPositionKey.size();

	for(int i = keyCount - 2; i >= keyCount - getPly(); i -= 2) {
		if(prevPositionKey[i] == currentKey) {
			return true;
		}
	}

	return false;
}

bool Position::checkDraw() const {
	if(getPly() > 99 || checkRepetition()) {
		return true;
	}

	int pieceCount = popCount(getOccupied());

	switch(pieceCount) {
	case 2: {
		return true;
	}
	case 3: {
		if(getBitboard(KNIGHT) || getBitboard(BISHOP)) {
			return true;
		}
	}
	case 4: {
		if(popCount(getBitboard(KNIGHT)) == 2) {
			return true;
		}
		if(popCount(getBitboardColour(WHITE)) == popCount(getBitboardColour(BLACK))) {
			return true;
		}
		else if(popCount(getBitboard(BISHOP))) {
			return false;
		}
		else return true;
	}
	}

	return false;
}