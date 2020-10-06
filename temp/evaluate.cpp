#include "evaluate.h"

int Evaluator::evaluate(Position& position) {
    return Evaluate(position).value();
}

int Evaluate::value() {
    Score score;

        score += pawnScore();
        score += pieceScore();
        score += kingScore();
        score += passedPawnScore();
        position.switchSides();

        score -= pawnScore();
        score -= pieceScore();
        score -= kingScore();
        score -= passedPawnScore();

        position.switchSides();

    int phase = get_game_phase();

    return score.value(phase, MAX_PHASE);
}

Score Evaluate::pawnScore() {
    Score score;

    Colour us = position.getSide();
    Colour them = ~us;

    Bitboard ourPawns = position.getBitboard(PAWN, us);
    Bitboard enemyPawns = position.getBitboard(PAWN, them);

    Direction relativeNorth = us == WHITE ? NORTH : SOUTH;
    Direction relativeSouth = us == WHITE ? SOUTH : NORTH;
    Direction relativeNorthEast = us == WHITE ? NORTH_EAST : SOUTH_EAST;
    Direction relativeNorthWest = us == WHITE ? NORTH_WEST : SOUTH_WEST;

    this->blockedPawns[us] = shift((shift(ourPawns, relativeNorth) & enemyPawns), relativeSouth);
    Bitboard underAttack = shift(ourPawns, relativeNorthEast) | shift(ourPawns, relativeNorthWest);

    this->attackedBy[us][PAWN] |= underAttack;
    this->attackedBy[us][ALL_PIECES] |= underAttack;

    score += pieceValue[PAWN] * popCount(ourPawns);

    Bitboard pawnOptions = ourPawns;

    while(pawnOptions) {
        Square pawn = popLsb(pawnOptions);

        score += PST::pawnSquareBonus[getRelativeRank(us, pawn)][getFile(pawn)];

        if(relativeBoard(us, lookups::getNorth(pawn)) & ourPawns) {
            score += doubledPawn;
        }
        else if(!relativeBoard(us, lookups::getPassedPawnMask(pawn)) & enemyPawns) {
            this->passedPawns[us] ^= bitShift(pawn);
        }

        if(!(lookups::adjacent_files(pawn) & ourPawns)) {
            score += isolatedPawn;
        }
       
    }

    return score;
}

Score Evaluate::pieceScore() {
    Score score;

    Colour us = position.getSide();
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard mobility = 0;

    Bitboard relative7th = us == WHITE ? RANK_7_MASK : RANK_2_MASK;
    Direction relativeSouthEast = us == WHITE ? SOUTH_EAST : NORTH_EAST;
    Direction relativeSouthWest = us == WHITE ? SOUTH_WEST : NORTH_WEST;

    mobility |= shift(position.getBitboard(PAWN, them), relativeSouthEast);
    mobility |= shift(position.getBitboard(PAWN, them), relativeSouthWest);
    mobility |= this->blockedPawns[us] | position.getBitboard(KING, us);
    mobility = ~mobility;

    if(popCount(position.getBitboard(BISHOP, us)) >= 2) {
        score += bishopPair;
    }

    score += rookOn7th * (popCount(position.getBitboard(ROOK, us) & relative7th));

    for(PieceType pieceType = KNIGHT; pieceType < KING; ++pieceType) {
        Bitboard pieceBoard = position.getBitboard(pieceType, us);

        score += pieceValue[pieceType] * popCount(pieceBoard);

        while(pieceBoard) {
            Square pieceSquare = popLsb(pieceBoard);

            Bitboard attacksOnEnemyKing = lookups::getKingDangerZone(us, position.getPosition(KING, them));
            Bitboard attacks = lookups::attacks(pieceType, pieceSquare, occupied, us);
            this->attackedBy[us][pieceType] |= attacks;
            this->attackedBy[us][ALL_PIECES] |= attacks;

            score += PST::pieceSquareBonus[pieceType][relativeSquare(us, pieceSquare)];
            score += mobilityBonus[pieceType][popCount(attacks & mobility)];

            if(attacksOnEnemyKing) {
                this->kingAttacks[us] += (kingAttackWeight[pieceType] * popCount(attacksOnEnemyKing));
            }
        }
    }

    return score;
}

Score Evaluate::passedPawnScore() {
    Score score;

    Colour us = position.getSide();
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard passedPawnBoard = this->passedPawns[us];

    while(passedPawnBoard) {
        Square pawnSquare = popLsb(passedPawnBoard);

        Rank rank = getRelativeRank(us, pawnSquare);
        Bitboard upOne = bitShift(shift(pawnSquare, pawnPush(us)));

        if(upOne & occupied) {
            score += passedRank[rank];
        }
        else {
            Bitboard queenLine = us == WHITE ? lookups::getNorth(pawnSquare) : lookups::getSouth(pawnSquare);
            Bitboard defended = queenLine;
            Bitboard attacked = queenLine;
            Bitboard xrays = (position.getBitboard(ROOK) | position.getBitboard(QUEEN)) & (us == WHITE ? lookups::getSouth(pawnSquare) : lookups::getNorth(pawnSquare)) & lookups::rook(pawnSquare, occupied);
        
            if(!(xrays & position.getBitboardColour(us))) {
                defended &= attackedBy[us][ALL_PIECES];
            }
            if(!(xrays & position.getBitboardColour(them))) {
                attacked &= attackedBy[us][ALL_PIECES];
            }

            if(!attacked) {
                score += passedRank[rank];
            }
            else if(attacked && !defended) {
                score += (passedRank[rank] * 0.8);
            }
            else {
                score += (passedRank[rank] * 0.6);
            }
        }
    }

    return score;
}

// Re-examine king safety
// Does not have king_attack_table
// Check out stockfish Evaluation<T>::king() function
Score Evaluate::kingScore() {
    Score score;

    Colour us = position.getSide();
    Colour them = ~us;

    Square kingSquare = position.getPosition(KING, us);

    std::pair<Bitboard, Bitboard> shelter = lookups::kingShelterMasks(us, kingSquare);

    Bitboard pawns = position.getBitboard(PAWN, us);
    Bitboard attacksOnKing = lookups::king(kingSquare);

    this->attackedBy[us][KING] |= attacksOnKing;
    this->attackedBy[us][ALL_PIECES] |= attacksOnKing;
    
    score += PST::pieceSquareBonus[KING][relativeSquare(us, kingSquare)];
    score += closeShelter * popCount(pawns & shelter.first);
    score += farShelter * popCount(pawns & shelter.second);

    int kingAttack = kingAttackTable[std::min(kingAttacks[us], 99)];
    score += S(kingAttack, (kingAttack / 2));

    return score;
}

int Evaluate::get_game_phase() {
    int phase = 0;
    for(PieceType pieceType = PAWN; pieceType < KING; ++pieceType) {
        phase += piecePhase[pieceType] * popCount(position.getBitboard(pieceType));
    }

    return phase;
}

void PST::init() {
    for(PieceType pieceType = KNIGHT; pieceType <= KING; ++pieceType) {
        for(int fileDistance = 0; fileDistance < 4; ++fileDistance) {
            for(Rank rank = RANK_1; rank <= RANK_8; ++rank) {
                int leftSquare = 0;
                int rightSquare = 0;
                pieceSquareBonus[pieceType][(rank * FILE_COUNT) + fileDistance] = pieceSquareHelper[pieceType][rank][fileDistance];
                pieceSquareBonus[pieceType][(rank * FILE_COUNT) + (FILE_H - fileDistance)] = pieceSquareHelper[pieceType][rank][fileDistance];
            }
        }
    }
}