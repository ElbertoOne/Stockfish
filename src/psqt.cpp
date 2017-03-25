/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>

#include "types.h"

Value PieceValue[PHASE_NB][PIECE_NB] = {
  { VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
  { VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg }
};

namespace PSQT {

#define S(mg, eg) make_score(mg, eg)

// Bonus[PieceType][Square / 2] contains Piece-Square scores. For each piece
// type on a given square a (middlegame, endgame) score pair is assigned. Table
// is defined for files A..D and white side: it is symmetric for black side and
// second half of the files.
const Score Bonus[][RANK_NB][int(FILE_NB) / 2] = {
  { },
  { // Pawn
   { S(  0, 0), S(  0, 0), S(  0, 0), S( 0, 0) },
   { S(-11, 7), S(  6,-4), S(  7, 8), S( 3,-2) },
   { S(-18,-4), S( -2,-5), S( 19, 5), S(24, 4) },
   { S(-17, 3), S( -9, 3), S( 20,-8), S(35,-3) },
   { S( -6, 8), S(  5, 9), S(  3, 7), S(21,-6) },
   { S( -6, 8), S( -8,-5), S( -6, 2), S(-2, 4) },
   { S( -4, 3), S( 20,-9), S( -8, 1), S(-4,18) }
  },
  { // Knight
   { S(-133,-88), S(-87,-72), S(-72,-38), S(-65,-8) },
   { S(-75,-60), S(-37,-46), S(-16,-11), S(-6,14) },
   { S(-64,-42), S(-17,-32), S(4,-1), S(12,32) },
   { S(-20,-33), S(21,-18), S(45,11), S(49,42) },
   { S(-21,-38), S(19,-18), S(40,8), S(51,44) },
   { S(-7,-46), S(39,-31), S(57,-1), S(66,31) },
   { S(-55,-56), S(-12,-42), S(9,-17), S(17,18) },
   { S(-180,-98), S(-59,-79), S(-36,-42), S(-24,-7) }
  },
  { // Bishop
   { S(-41,-52), S(-12,-25), S(-23,-31), S(-31,-14) },
   { S(-19,-28), S(18,-4), S(11,-9), S(1,8) },
   { S(-8,-18), S(25,5), S(19,2), S(10,20) },
   { S(-10,-21), S(26,2), S(19,0), S(9,20) },
   { S(-10,-21), S(22,1), S(15,-2), S(8,18) },
   { S(-16,-19), S(15,3), S(11,5), S(2,17) },
   { S(-21,-28), S(15,-5), S(5,-7), S(-2,10) },
   { S(-32,-49), S(-10,-26), S(-18,-30), S(-27,-12) }
  },
  { // Rook
   { S(-25, 0), S(-16, 0), S(-16, 0), S(-9, 0) },
   { S(-21, 0), S( -8, 0), S( -3, 0), S( 0, 0) },
   { S(-21, 0), S( -9, 0), S( -4, 0), S( 2, 0) },
   { S(-22, 0), S( -6, 0), S( -1, 0), S( 2, 0) },
   { S(-22, 0), S( -7, 0), S(  0, 0), S( 1, 0) },
   { S(-21, 0), S( -7, 0), S(  0, 0), S( 2, 0) },
   { S(-12, 0), S(  4, 0), S(  8, 0), S(12, 0) },
   { S(-23, 0), S(-15, 0), S(-11, 0), S(-5, 0) }
  },
  { // Queen
   { S(0,-64), S(-4,-49), S(-3,-35), S(-1,-23) },
   { S(-4,-49), S(6,-24), S(9,-15), S(8,0) },
   { S(-2,-33), S(6,-11), S(9,-2), S(9,10) },
   { S(-1,-23), S(8,0), S(10,14), S(7,24) },
   { S(-3,-21), S(9,0), S(8,15), S(7,26) },
   { S(-2,-34), S(6,-10), S(8,-4), S(10,8) },
   { S(-2,-48), S(7,-24), S(7,-15), S(6,-1) },
   { S(-1,-67), S(-4,-48), S(-1,-36), S(0,-24) }
  },
  { // King
   { S(267,0), S(320,48), S(270,76), S(195,85) },
   { S(264,43), S(304,93), S(238,144), S(180,133) },
   { S(200,84), S(245,139), S(176,169), S(110,167) },
   { S(177,107), S(185,171), S(148,171), S(110,181) },
   { S(149,109), S(177,165), S(115,202), S(66,205) },
   { S(118,96), S(159,157), S(84,178), S(41,176) },
   { S(86,51), S(128,100), S(63,123), S(18,140) },
   { S(63,9), S(89,56), S(47,81), S(0,91) }
  }
};

#undef S

Score psq[PIECE_NB][SQUARE_NB];

// init() initializes piece-square tables: the white halves of the tables are
// copied from Bonus[] adding the piece value, then the black halves of the
// tables are initialized by flipping and changing the sign of the white scores.
void init() {

  for (Piece pc = W_PAWN; pc <= W_KING; ++pc)
  {
      PieceValue[MG][~pc] = PieceValue[MG][pc];
      PieceValue[EG][~pc] = PieceValue[EG][pc];

      Score v = make_score(PieceValue[MG][pc], PieceValue[EG][pc]);

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          File f = std::min(file_of(s), FILE_H - file_of(s));
          psq[ pc][ s] = v + Bonus[pc][rank_of(s)][f];
          psq[~pc][~s] = -psq[pc][s];
      }
  }
}

} // namespace PSQT
