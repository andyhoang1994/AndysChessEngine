#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include "bitboard.h"
#include "movegen.h"
#include "utils.h"
#include "position.h"
#include "thread.h"

const std::string PieceToChar(" PNBRQK  pnbrqk");
constexpr Piece Pieces[] = { wP, wN, wB, wR, wQ, wK,
							 bP, bN, bB, bR, bQ, bK, };

namespace Zobrist {
	Key psq[PIECE_COUNT][SQUARE_COUNT];
	Key enpassant[FILE_COUNT];
	Key castling[ALL_CASTLING];
	Key side;
	Key exclusion;
}

Key Position::getExclusionKey() const { return positionKey ^ Zobrist::exclusion; }

Position& Position::operator=(const Position& pos) {
	std::memcpy(this, &pos, sizeof(Position));
	nodes = 0;

	return *this;
}

void Position::init(std::string fen, Thread* thread) {
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
	Zobrist::exclusion = utils::rand_u64(0, UINT64_MAX);
	std::fill(std::begin(prevPositionKey), std::end(prevPositionKey), 0);
	thisThread = thread;
	parseFen(fen);
}

void Position::clear() {
	for(int i = 0; i < 7; ++i) {
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
}

void Position::parseFen(std::string fenString) {
	int piece = 0;
	char token = '\0';

	std::istringstream stream(fenString);
	stream >> std::noskipws;

	int square = 0;

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
	ply = totalMoves;
	this->positionKey = generatePositionKey();
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

bool Position::checkLegality(Move move) const {
	Colour us = getSide();
	Colour them = ~us;

	Square from = getFrom(move);
	Square to = getTo(move);
	Square kingSquare = getPosition(KING, us);

	Bitboard occupied = getOccupied();

	if(getPieceOnSquare(from) == NO_PIECE || getPieceColour(getPieceOnSquare(from)) != us) {
		return false;
	}
	if(getPieceOnSquare(to) != NO_PIECE && getPieceColour(getPieceOnSquare(to)) == us) {
		return false;
	}
	if(getPieceType(getPieceOnSquare(to)) == KING) {
		return false;
	}

	if(getMoveType(move) == ENPASSANT) {
		if(side == WHITE && getRank(enPassantSquare) != RANK_5) {
			return false;
		}
		if(side == BLACK && getRank(enPassantSquare) != RANK_4) {
			return false;
		}
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
	else if(getMoveType(move) == CASTLING) {
		if(to < from) {
			for(int square = 0; square <= 2; ++square) {
				if((bitShift(castling::queenSide[us][square]) & occupied) || attackersTo(castling::queenSide[us][square], them)) {
					return false;
				}
			}
		}
		else {
			for(int square = 0; square <= 1; ++square) {
				if((bitShift(castling::kingSide[us][square]) & occupied) || attackersTo(castling::kingSide[us][square], them)) {
					return false;
				}
			}
		}

		return true;
	}
	else if(from == kingSquare) {
		return !attackersTo(to, them);
	}
	else {
		return !(pinned(us) & bitShift(from)) || (bitShift(to) & lookups::full_ray(from, kingSquare));
	}
}

bool Position::givesCheck(Move move) const {
	Colour us = side;
	Colour them = ~us;

	Square from = getFrom(move);
	Square to = getTo(move);
	Bitboard bitFrom = bitShift(from);
	Bitboard bitTo = bitShift(to);
	PieceType fromPieceType = getPieceType(getPieceOnSquare(from));
	PieceType toPieceType = getPieceType(getPieceOnSquare(to));

	Bitboard occupied = getOccupied();

	Square enemyKing = getPosition(KING, them);
	Bitboard pawns = getBitboard(PAWN, us);
	Bitboard knights = getBitboard(KNIGHT, us);
	Bitboard bishops = getBitboard(BISHOP, us);
	Bitboard rooks = getBitboard(ROOK, us);
	Bitboard queens = getBitboard(QUEEN, us);
	
	switch(getMoveType(move)) {
		case NORMAL: {
			switch(fromPieceType) {
				case PAWN:
					pawns ^= bitFrom;
					pawns ^= bitTo;
					break;
				case KNIGHT:
					knights ^= bitFrom;
					knights ^= bitTo;
					break;
				case BISHOP:
					bishops ^= bitFrom;
					bishops ^= bitTo;
					break;
				case ROOK:
					rooks ^= bitFrom;
					rooks ^= bitTo;
					break;
				case QUEEN:
					queens ^= bitFrom;
					queens ^= bitTo;
					break;
				case KING:
					break;
				default:
					return false;
			}

			occupied ^= bitFrom;
			occupied |= bitTo;
			break;
		}
		case PROMOTION: {
			occupied ^= bitFrom;
			pawns ^= bitFrom;

			switch(getPromotionType(move)) {
				case PROMOTE_TO_KNIGHT: 
					knights ^= bitTo;
					break;
				case PROMOTE_TO_BISHOP:
					bishops ^= bitTo;
					break;
				case PROMOTE_TO_ROOK:
					rooks ^= bitTo;
					break;
				case PROMOTE_TO_QUEEN:
					queens ^= bitTo;
					break;
			}
			if(toPieceType == NO_PIECE) {
				occupied ^= bitTo;
			}
			break;
		}

		case ENPASSANT: {
			occupied ^= bitFrom;
			pawns ^= bitFrom;
			pawns ^= bitTo;

			occupied ^= bitShift(enPassantSquare);
			break;
		}
		case CASTLING: {
			Square rookSquare;
			if(from > to) {
				rookSquare = us == WHITE ? D1 : D8;
			}
			else {
				rookSquare = us == WHITE ? F1 : F8;
			}

			occupied ^= bitShift(rookSquare);
			return lookups::rook(enemyKing, occupied) & bitShift(rookSquare);
		}
		default:
			assert(false);
			return false;
	}

	return (lookups::rook(enemyKing, occupied) & (rooks | queens))
		| (lookups::bishop(enemyKing, occupied) & (bishops | queens))
		| (lookups::knight(enemyKing) & knights)
		| (lookups::pawn(enemyKing, them) & pawns);
}

void Position::makeMove(Undo* undo, Move move) {
	Square from = getFrom(move);
	Square to = getTo(move);
	Piece fromPiece = getPieceOnSquare(from);
	Piece toPiece = getPieceOnSquare(to);
	
	undo->positionKey = positionKey;
	undo->castleRights = castlingRights;
	undo->fiftyMoveCount = fiftyMoveCount;
	undo->ply = ply;
	undo->enpassantSquare = enPassantSquare;
	undo->captureSquare = to;
	undo->capturePiece = toPiece;

	// For castling
	Piece rook = (side == WHITE) ? wR : bR;
	Square queenSide = (side == WHITE) ? C1 : C8;
	Square kingSide = (side == WHITE) ? G1 : G8;
	Square queenRook = (side == WHITE) ? A1 : A8;
	Square kingRook = (side == WHITE) ? H1 : H8;

	castlingRights &= castling::castlingRightsMask[from] & castling::castlingRightsMask[to];
	enPassantSquare = NO_SQUARE;

	if(getPieceType(fromPiece) == PAWN) {
		resetFiftyMoveCount();
	}
	else {
		incrementFiftyMoveCount();
	}

	switch(getMoveType(move)) {
	case NORMAL: {
		if(checkCapture(move)) {
			resetFiftyMoveCount();
		}

		if(getPieceType(fromPiece) == PAWN && ((to ^ from) == 16)) {
			setEnPassant(Square(to - pawnPush(side)));
		}
		movePiece(from, to);
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
	++ply;
}

void Position::undoMove(Undo* undo, Move move) {

	switchSides();
	if(ply > 0) {
		--ply;
	}
	positionKey = undo->positionKey;
	castlingRights = undo->castleRights;
	fiftyMoveCount = undo->fiftyMoveCount;
	enPassantSquare = undo->enpassantSquare;

	Colour us = getSide();
	Square to = getTo(move);
	Square from = getFrom(move);
	Piece toPiece = getPieceOnSquare(to);
	
	if(getMoveType(move) == PROMOTION) {
		Piece pawn = (getSide() == WHITE) ? wP : bP;
		removePiece(to, toPiece);
		placePiece(getFrom(move), pawn);
	}
	else if(getMoveType(move) == CASTLING) {
		switch(to) {
			case C1: {
				castlingRights |= WHITE_OOO;
				movePiece(C1, E1);
				movePiece(D1, A1);
				break;

			}
			case G1: {
				castlingRights |= WHITE_OO;
				movePiece(G1, E1);
				movePiece(F1, H1);
				break;
			}
			case C8: {
				castlingRights |= BLACK_OOO;
				movePiece(C8, E8);
				movePiece(C8, A8);
				break;
			}
			case G8: {
				castlingRights |= BLACK_OO;
				movePiece(G8, E8);
				movePiece(F8, H8);
				break;
			}
			default: break;
		}
	}
	else {
		movePiece(to, from);

		if(undo->capturePiece != NO_PIECE) {
			placePiece(undo->captureSquare, undo->capturePiece);
		}
	}
}

bool Position::validateMove(Move move) const {
	return getPieceColour(getPieceOnSquare(getFrom(move))) == side;
}

void Position::makeNullMove(Undo* undo) {
	undo->positionKey = positionKey;
	undo->fiftyMoveCount = fiftyMoveCount;
	undo->enpassantSquare = enPassantSquare;

	incrementFiftyMoveCount();
	enPassantSquare = NO_SQUARE;
	switchSides();
	positionKey = generatePositionKey();
	++ply;
	if(enPassantSquare != NO_SQUARE) {
		positionKey ^= Zobrist::enpassant[getFile(enPassantSquare)];
		enPassantSquare = NO_SQUARE;
	}
}

void Position::undoNullMove(Undo* undo) {
	switchSides();
	if(ply > 0) {
		--ply;
	}
	positionKey = undo->positionKey;
	fiftyMoveCount = undo->fiftyMoveCount;
	enPassantSquare = undo->enpassantSquare;
}

Colour Position::getSide() const {
	return side;
}

Key Position::generatePositionKey() {
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

	prevPositionKey[ply] = key;
	return key;
}

template<int pieceType>
PieceType minAttacker(const Position* position, Square to, Bitboard sideToMoveAttackers,
						Bitboard& occupied, Bitboard& attackers) {

	Bitboard b = sideToMoveAttackers & position->getBitboard(PieceType(pieceType));
	if(!b) {
		return minAttacker<pieceType + 1>(position, to, sideToMoveAttackers, occupied, attackers);
	}

	occupied ^= b & ~(b - 1);

	if(pieceType == PAWN || pieceType == BISHOP || pieceType == QUEEN) {
		attackers |= position->attackersTo(to, occupied) & (position->getBitboard(BISHOP) | position->getBitboard(QUEEN));
	}

	if(pieceType == ROOK || pieceType == QUEEN) {
		attackers |= position->attackersTo(to, occupied) & (position->getBitboard(ROOK) | position->getBitboard(QUEEN));
	}

	attackers &= occupied; // After X-ray that may add already processed pieces
	return (PieceType)pieceType;
}
template<>
PieceType minAttacker<KING>(const Position*, Square, Bitboard, Bitboard&, Bitboard&) {
	return KING;
}

Value Position::seeSign(Move move) const {
	if(pieceValue[getPieceOnSquare(getFrom(move))].value() <= pieceValue[getPieceOnSquare(getTo(move))].value()) {
		return VALUE_KNOWN_WIN;
	}

	return see(move);
}

Value Position::see(Move move) const {
	Value swapList[32];
	int slIndex = 1;

	Square from = getFrom(move);
	Square to = getTo(move);
	swapList[0] = Value(pieceValue[getPieceOnSquare(to)].value());
	Colour sideToMove = getPieceColour(getPieceOnSquare(from));
	Bitboard occupied  = getOccupied() ^ bitShift(from);

	if(getMoveType(move) == CASTLING) {
		return VALUE_ZERO;
	}

	if(getMoveType(move) == ENPASSANT) {
		occupied ^= bitShift(to - pawnPush(sideToMove));
		swapList[0] = Value(pieceValue[PAWN].value());
	}

	Bitboard attackers = attackersTo(to, occupied) & occupied;

	sideToMove = ~sideToMove;
	Bitboard sideToMoveAttackers = attackers & getBitboardColour(sideToMove);
	if(!sideToMoveAttackers) {
		return swapList[0];
	}

	PieceType captured = getPieceType(getPieceOnSquare(from));

	do {
		assert(slIndex < 32);

		swapList[slIndex] = -swapList[slIndex - 1] + pieceValue[captured].value();

		captured = minAttacker<PAWN>(this, to, sideToMoveAttackers, occupied, attackers);
		sideToMove = ~sideToMove;
		sideToMoveAttackers = attackers & getBitboardColour(sideToMove);
		++slIndex;

	} while(sideToMoveAttackers && (captured != KING || (--slIndex, false)));

	while(--slIndex) {
		swapList[slIndex - 1] = std::min(-swapList[slIndex], swapList[slIndex - 1]);
	}

	return swapList[0];
}

bool Position::checkRepetition() const {
	Key currentKey = getPositionKey();

	int repetitions = 0;
	for(int i = ply - 2; i >= 0; i -= 2) {
		if(prevPositionKey[i] == currentKey) {
			++repetitions;
		}
	}

	return repetitions >= 2;
}

bool Position::checkDraw() const {
	if(fiftyMoveCount > 99 || checkRepetition()) {
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

bool Position::checkNonPawnMaterial(Colour colour) const {
	for(PieceType pieceType = KNIGHT; pieceType < KING; ++pieceType) {
		if((bitboardsColour[colour] & bitboardsType[pieceType]) > uint64_t(0)) return true;
	}

	return false;
}