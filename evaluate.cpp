#include <algorithm>

#include "bitboard.h"
#include "evaluate.h"
#include "movegen.h"
#include "search.h"

int Evaluator::evaluate(Position& position) {
    return Evaluate(position).value();
}

Value Evaluate::value() {
    Score score;

    Colour us = position.getSide();
    Colour them = ~us;

    score += pawnScore(us) - pawnScore(them);
    score += pieceScore(us) - pieceScore(them);
    score += kingScore(us) - kingScore(them);
    score += passedPawnScore(us) - passedPawnScore(them);
    score += threatScore(us) - passedPawnScore(them);

    int phase = getGamePhase();

    return score.value(phase, 256);
}

Score Evaluate::pawnScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard ourPawns = position.getBitboard(PAWN, us);
    Bitboard enemyPawns = position.getBitboard(PAWN, them);

    Direction relativeNorth = us == WHITE ? NORTH : SOUTH;
    Direction relativeSouth = us == WHITE ? SOUTH : NORTH;
    Direction relativeNorthEast = us == WHITE ? NORTH_EAST : SOUTH_EAST;
    Direction relativeNorthWest = us == WHITE ? NORTH_WEST : SOUTH_WEST;

    this->blockedPawns[us] = shift((shift(ourPawns, relativeNorth) & enemyPawns), relativeSouth);
    Bitboard underAttack = shift(ourPawns, relativeNorthEast) | shift(ourPawns, relativeNorthWest);

    this->attackedByMore[us] |= this->attackedBy[us][ALL_PIECES] & underAttack;
    this->attackedBy[us][PAWN] |= underAttack;
    this->attackedBy[us][ALL_PIECES] |= underAttack;

    score += pieceValue[PAWN] * popCount(ourPawns);

    Bitboard pawnOptions = ourPawns;

    while(pawnOptions) {
        Square pawn = popLsb(pawnOptions);

        score += pieceSquareBonus[PAWN][relativeSquare(us, pawn)];

        if(relativeBoard(us, lookups::getNorth(relativeSquare(us, pawn))) & ourPawns) {
            score += doubledPawn;
        }
        else if(!(relativeBoard(us, lookups::getPassedPawnMask(relativeSquare(us, pawn))) & enemyPawns)) {
            this->passedPawns[us] ^= bitShift(pawn);
        }

        if(!(lookups::adjacent_files(pawn) & ourPawns)) {
            score += isolatedPawn;
        }
       
    }

    return score;
}

Score Evaluate::pieceScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();

    Direction relativeSouthEast = us == WHITE ? SOUTH_EAST : NORTH_EAST;
    Direction relativeSouthWest = us == WHITE ? SOUTH_WEST : NORTH_WEST;

    this->mobility[us] |= shift(position.getBitboard(PAWN, them), relativeSouthEast);
    this->mobility[us] |= shift(position.getBitboard(PAWN, them), relativeSouthWest);
    this->mobility[us] |= this->blockedPawns[us] | position.getBitboard(KING, us);
    this->mobility[us] = ~mobility[us];

    score += knightScore(us);
    score += bishopScore(us);
    score += rookScore(us);
    score += queenScore(us);

    return score;
}

Score Evaluate::knightScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard knights = position.getBitboard(KNIGHT, us);

    score += pieceValue[KNIGHT] * popCount(knights);

    while(knights) {
        Square pieceSquare = popLsb(knights);

        Bitboard attacksOnEnemyKing = lookups::getKingDangerZone(us, position.getPosition(KING, them));
        Bitboard attacks = lookups::attacks(KNIGHT, pieceSquare, occupied, us);

        this->attackedByMore[us] |= this->attackedBy[us][ALL_PIECES] & attacks;
        this->attackedBy[us][KNIGHT] |= attacks;
        this->attackedBy[us][ALL_PIECES] |= attacks;

        score += pieceSquareBonus[KNIGHT][relativeSquare(us, pieceSquare)];
        score += mobilityBonus[KNIGHT][popCount(attacks & mobility[us])];
        if(getPieceType(position.getPieceOnSquare(pieceSquare + pawnPush(us))) == PAWN) {
            score += minorBehindPawn;
        }
        if(relativeBoard(us, lookups::getOutpostMask(relativeSquare(us, pieceSquare)))
        && !(relativeBoard(us, lookups::getOutpostMask(relativeSquare(us, pieceSquare))) & position.getBitboard(PAWN, them))) {
            score += knightOutpost;
        }

        if(attacksOnEnemyKing) {
            this->kingAttacks[us] += (kingAttackWeight[KNIGHT] * popCount(attacksOnEnemyKing));
        }
    }

    return score;
}

Score Evaluate::bishopScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard bishops = position.getBitboard(BISHOP, us);

    score += pieceValue[BISHOP] * popCount(bishops);
    if(popCount(position.getBitboard(BISHOP, us)) >= 2) {
        score += bishopPair;
    }

    while(bishops) {
        Square pieceSquare = popLsb(bishops);

        Bitboard attacksOnEnemyKing = lookups::getKingDangerZone(us, position.getPosition(KING, them));
        Bitboard attacks = lookups::attacks(BISHOP, pieceSquare, occupied, us);

        this->attackedByMore[us] |= this->attackedBy[us][ALL_PIECES] & attacks;
        this->attackedBy[us][BISHOP] |= attacks;
        this->attackedBy[us][ALL_PIECES] |= attacks;

        score += pieceSquareBonus[BISHOP][relativeSquare(us, pieceSquare)];
        score += mobilityBonus[BISHOP][popCount(attacks & mobility[us])];
        if(getPieceType(position.getPieceOnSquare(pieceSquare + pawnPush(us))) == PAWN) {
            score += minorBehindPawn;
        }
        if(relativeBoard(us, lookups::getOutpostMask(relativeSquare(us, pieceSquare)))
            && !(relativeBoard(us, lookups::getOutpostMask(relativeSquare(us, pieceSquare))) & position.getBitboard(PAWN, them))) {
            score += bishopOutpost;
        }

        if(attacksOnEnemyKing) {
            this->kingAttacks[us] += (kingAttackWeight[BISHOP] * popCount(attacksOnEnemyKing));
        }
    }

    return score;
}

Score Evaluate::rookScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard rooks = position.getBitboard(ROOK, us);

    score += pieceValue[ROOK] * popCount(rooks);

    while(rooks) {
        Square pieceSquare = popLsb(rooks);

        Bitboard attacksOnEnemyKing = lookups::getKingDangerZone(us, position.getPosition(KING, them));
        Bitboard attacks = lookups::attacks(ROOK, pieceSquare, occupied, us);

        this->attackedByMore[us] |= this->attackedBy[us][ALL_PIECES] & attacks;
        this->attackedBy[us][ROOK] |= attacks;
        this->attackedBy[us][ALL_PIECES] |= attacks;

        score += pieceSquareBonus[ROOK][relativeSquare(us, pieceSquare)];
        score += mobilityBonus[ROOK][popCount(attacks & mobility[us])];
        if(relativeRank(us, pieceSquare) >= RANK_7 && relativeRank(us, position.getPosition(KING, them)) >= RANK_7) {
            score += rookOnSeventh;
        }

        if(attacksOnEnemyKing) {
            this->kingAttacks[us] += (kingAttackWeight[ROOK] * popCount(attacksOnEnemyKing));
        }
    }

    return score;
}

Score Evaluate::queenScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard queens = position.getBitboard(QUEEN, us);

    score += pieceValue[QUEEN] * popCount(queens);

    while(queens) {
        Square pieceSquare = popLsb(queens);

        Bitboard attacksOnEnemyKing = lookups::getKingDangerZone(us, position.getPosition(KING, them));
        Bitboard attacks = lookups::attacks(QUEEN, pieceSquare, occupied, us);

        this->attackedByMore[us] |= this->attackedBy[us][ALL_PIECES] & attacks;
        this->attackedBy[us][QUEEN] |= attacks;
        this->attackedBy[us][ALL_PIECES] |= attacks;

        score += pieceSquareBonus[QUEEN][relativeSquare(us, pieceSquare)];
        score += mobilityBonus[QUEEN][popCount(attacks & mobility[us])];

        if(attacksOnEnemyKing) {
            this->kingAttacks[us] += (kingAttackWeight[QUEEN] * popCount(attacksOnEnemyKing));
        }
    }

    return score;
}

Score Evaluate::passedPawnScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Bitboard occupied = position.getOccupied();
    Bitboard passedPawnBoard = this->passedPawns[us];

    while(passedPawnBoard) {
        Square pawnSquare = popLsb(passedPawnBoard);

        Rank rank = relativeRank(us, pawnSquare);
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
                defended &= this->attackedBy[us][ALL_PIECES];
            }
            if(!(xrays & position.getBitboardColour(them))) {
                attacked &= this->attackedBy[us][ALL_PIECES];
            }

            if(!attacked) {
                score += passedRank[rank];
            }
            else if(attacked && !defended) {
                score += (passedRank[rank] * 0.7);
            }
            else {
                score += (passedRank[rank] * 0.4);
            }
        }
    }

    return score;
}

Score Evaluate::kingShelterScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Square kingSquare = position.getPosition(KING, us);

    Bitboard ourPawns = position.getBitboard(PAWN, us);
    Bitboard theirPawns = position.getBitboard(PAWN, them);

    for(File file = File(std::max(0, int(getFile(kingSquare)) - 1)); file <= std::min(int(FILE_H), int(getFile(kingSquare)) + 1); ++file) {
        Bitboard ourPawn = ourPawns & lookups::fileMask(file);
        int ourRank = !ourPawn ? 0 : relativeRank(us, relativeFirstBit(them, ourPawn));

        uint64_t theirPawn = theirPawns & lookups::fileMask(file);
        int theirRank = !theirPawn ? 0 : relativeRank(us, relativeFirstBit(them, theirPawn));

        int edgeDist = edgeDistance(file);
        score += pawnShelter[edgeDist][ourRank] / 2;

        if(ourRank && (ourRank == (theirRank - 1))) {
            score -= blockedPawnStorm[theirRank] / 2;
        }
        else {
            score -= unblockedPawnStorm[edgeDist][theirRank] / 2;
        }
    }

    return score;
}

Score Evaluate::kingScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Square kingSquare = position.getPosition(KING, us);

    Bitboard ourPawns = position.getBitboard(PAWN, us);
    Bitboard theirPawns = position.getBitboard(PAWN, them);
    Bitboard defenders = ourPawns | position.getBitboard(KNIGHT, us) | position.getBitboard(BISHOP, us);
    Bitboard attacksByKing = lookups::king(kingSquare);
    Bitboard weakSquares = this->attackedBy[them][ALL_PIECES] & ~this->attackedByMore[us]
        & (~this->attackedBy[us][ALL_PIECES] | this->attackedBy[us][QUEEN] | this->attackedBy[us][KING]);

    this->attackedBy[us][KING] |= attacksByKing;
    this->attackedBy[us][ALL_PIECES] |= attacksByKing;

    score += pieceSquareBonus[KING][relativeSquare(us, kingSquare)];
    score += kingDefenders[popCount(defenders & lookups::kingShelter(us, kingSquare))];
    if(popCount(lookups::kingShelter(us, kingSquare) & this->attackedBy[them][ALL_PIECES]) > 1 - popCount(position.getBitboard(QUEEN, them))) {
        Bitboard knightAttackSquares = lookups::knight(kingSquare);
        Bitboard bishopAttackSquares = lookups::bishop(kingSquare);
        Bitboard rookAttackSquares = lookups::rook(kingSquare);
        Bitboard queenAttackSquares = lookups::queen(kingSquare);

        Bitboard defended = ~position.getBitboardColour(them) & (~this->attackedBy[us][ALL_PIECES] | (weakSquares & this->attackedByMore[them]));

        Bitboard safeKnightChecks = knightAttackSquares & this->attackedBy[them][KNIGHT] & defended;
        Bitboard safeBishopChecks = bishopAttackSquares & this->attackedBy[them][BISHOP] & defended;
        Bitboard safeRookChecks = rookAttackSquares & this->attackedBy[them][ROOK] & defended;
        Bitboard safeQueenChecks = queenAttackSquares & this->attackedBy[them][QUEEN] & defended;

        Score safetyScore = safeKnightCheckScore * popCount(safeKnightChecks)
            + safeBishopCheckScore * popCount(safeBishopChecks)
            + safeRookCheckScore * popCount(safeRookChecks)
            + safeQueenCheckScore * popCount(safeQueenChecks)
            + weakSquare * popCount(weakSquares & lookups::kingShelter(us, kingSquare));

        score += S(-safetyScore.value(1, 1) * std::max(int(safetyScore.value(1, 1)), 0) / 720, -std::max(int(safetyScore.value(0, 1)), 0) / 20);
    }

    score += kingShelterScore(colour);

    return score;
}

Score Evaluate::threatScore(Colour colour) {
    Score score;

    Colour us = colour;
    Colour them = ~us;

    Direction up = pawnPush(us);
    Direction upLeft = (us == WHITE ? NORTH_WEST : SOUTH_EAST);
    Direction upRight = (us == WHITE ? NORTH_EAST : SOUTH_WEST);
    Bitboard relativeRank3 = us == WHITE ? RANK_3_MASK : RANK_6_MASK;

    Bitboard candidates = 0;
    Bitboard occupied = position.getOccupied();
    Bitboard bitboardUs = position.getBitboardColour(us);
    Bitboard bitboardThem = position.getBitboardColour(them);
    Bitboard ourPawns = position.getBitboard(PAWN, us);
    Bitboard nonPawnEnemies = bitboardThem & ~position.getBitboard(PAWN);

    Bitboard safeSquares = ~this->attackedBy[them][ALL_PIECES] | this->attackedBy[us][ALL_PIECES];
    Bitboard pawnThreats = this->attackedBy[them][PAWN];

    // Strongly defended by the enemy
    Bitboard stronglyDefended = pawnThreats | (this->attackedByMore[them] & ~this->attackedByMore[us]);
    Bitboard defendedPieces = nonPawnEnemies & stronglyDefended;
    Bitboard weakPieces = bitboardThem & ~stronglyDefended & this->attackedBy[us][ALL_PIECES];

    if(defendedPieces | weakPieces) {
        candidates = (defendedPieces | weakPieces) & (this->attackedBy[us][KNIGHT] | this->attackedBy[us][BISHOP]);
        while(candidates) {
            Square candidateSquare = popLsb(candidates);
            score += threatByMinor[getPieceType(position.getPieceOnSquare(candidateSquare))];
        }

        candidates = weakPieces & this->attackedBy[us][ROOK];
        while(candidates) {
            Square candidateSquare = popLsb(candidates);
            score += threatByRook[getPieceType(position.getPieceOnSquare(candidateSquare))];
        }

        if(weakPieces & this->attackedBy[us][KING]) {
            score += threatByKing;
        }

        candidates = ~this->attackedBy[them][ALL_PIECES] | (nonPawnEnemies & this->attackedByMore[us]);
        score += hangingPiece * popCount(weakPieces & candidates);
    }

    candidates = safeSquares & ourPawns;
    score += threatBySafePawn * popCount((shift(candidates, upLeft) | shift(candidates, upRight)) & nonPawnEnemies);

    candidates = shift(ourPawns, up) & ~occupied;
    candidates |= shift(candidates & relativeRank3, up) & ~occupied;
    candidates &= ~this->attackedBy[them][PAWN] & safeSquares;
    score += threatByPawnPush * popCount((shift(candidates, upLeft) | shift(candidates, upRight)) & nonPawnEnemies);

    return score;
}

int Evaluate::getGamePhase() {
    int phase = 24;
    for(PieceType pieceType = KNIGHT; pieceType < KING; ++pieceType) {
        phase -= piecePhase[pieceType] * popCount(position.getBitboard(pieceType));
    }

    phase = (phase * 256 + 12) / 24;

    return phase;
}