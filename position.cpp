#include <iostream>
#include <sstream>
#include <algorithm>
#include "position.h"

const std::string fenStart = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void Position::parseFen(const std::string& fenString) {
	int piece = 0;
	char token = '\0';

	std::istringstream stream(fenString);
	stream >> std::noskipws;

	int square = 0;
	// Fill out the board based on the first part of the( FEN string
	while((stream >> token) && !isspace(token)) {
		if(isdigit(token)) {
			square += token - '1';
		}
		else if(token == '/') {
			piece = NO_PIECE;
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
			// std::cout << "piece found:" << piece << " on square:" << square << "\n";
			placePiece(Square(square), Piece(piece));
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
		case('K'): castlePermisson |= WHITE_OO;  break;
		case('Q'): castlePermisson |= WHITE_OOO; break;
		case('k'): castlePermisson |= BLACK_OO;  break;
		case('q'): castlePermisson |= BLACK_OOO; break;
		}
	}
	// Go past whitespace
	stream >> token;
	if(token != '-') {
		int file = token - 'a';
		stream >> token;
		int rank = token - '1';

		enPassant = Square(file + rank * 8);
	}

	stream >> std::skipws >> fiftyMoveCounter;
	int totalMoves = 0;
	stream >> totalMoves;
	totalMoves = std::max(2 * (totalMoves - 1), 0) + side;

	// this->positionKey = generatePositionKey(pos);
}

void Position::init() {
	parseFen(fenStart);
}

void Position::display() {
	std::cout << "display starting:\n";
	for(int rank = RANK_1; rank <= RANK_8; rank++) {
		std::cout << "    +---+---+---+---+---+---+---+---+\n";
		std::cout << "  " << (8 - rank) << " |";
		for(int file = FILE_A; file <= FILE_H; file++) {
			std::cout << " " << PieceName[board[rank * 8 + file]] << "|";
		}
		std::cout << "\n";
	}
	std::cout << "    +---+---+---+---+---+---+---+---+\n";
	std::cout << "      a   b   c   d   e   f   g   h\n";
}

/*Key Position::generatePositionKey(Piece board[]) {
    Key key = 0;
    Piece piece = NO_PIECE;

    for(int square = 0; square < BOARD_SQUARE_NUM; ++square) {
        piece = pos->pieces[square];
        if(piece != NO_SQUARE && piece != EMPTY && piece != OFFBOARD) key ^= pieceKeys[piece][square];
    }
    if(pos->side == WHITE) key ^= sideKey;
    if(pos->enPassant != NO_SQUARE) key ^= pieceKeys[EMPTY][pos->enPassant];
    key ^= castleKeys[pos->castlePermisson];

    return key;
}*/