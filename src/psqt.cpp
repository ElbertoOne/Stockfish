/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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
{ VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg } };

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
   { S(-16, 7), S(  1,-4), S(  7, 8), S( 3,-2) },
   { S(-23,-4), S( -7,-5), S( 20, 5), S(23, 4) },
   { S(-22, 3), S(-15, 3), S( 20,-8), S(34,-3) },
   { S(-11, 8), S(  0, 9), S(  3, 7), S(20,-6) },
   { S(-11, 8), S(-13,-5), S( -6, 2), S(-2, 4) },
   { S( -9, 3), S( 15,-9), S( -8, 1), S(-4,18) }
  },
  { // Knight
   { S(-140, -92), S(-91,-81), S(-82,-47), S(-76,-14) },
   { S( -87, -68), S(-42,-57), S(-21,-17), S(-10,  9) },
   { S( -68, -49), S(-21,-37), S(  0, -8), S(  9, 28) },
   { S( -24, -43), S( 17,-26), S( 40,  7), S( 48, 36) },
   { S( -26, -46), S( 16,-25), S( 35,  2), S( 50, 40) },
   { S( -11, -56), S( 36,-37), S( 57, -8), S( 69, 26) },
   { S( -62, -63), S(-18,-51), S(  5,-23), S( 13, 13) },
   { S(-192,-102), S(-67,-97), S(-41,-48), S(-29,-13) }
  },
  { // Bishop
   { S(-56,-67), S(-23,-39), S(-34,-45), S(-44,-28) },
   { S(-29,-44), S( 10,-16), S(  2,-23), S( -9, -5) },
   { S(-19,-32), S( 17, -9), S( 11,-13), S(  1,  8) },
   { S(-21,-37), S( 17,-13), S( 11,-15), S(  0,  7) },
   { S(-21,-34), S( 14,-13), S(  6,-17), S( -1,  3) },
   { S(-27,-35), S(  6,-14), S(  2,-10), S( -8,  1) },
   { S(-33,-44), S(  7,-22), S( -4,-22), S(-12, -4) },
   { S(-45,-64), S(-21,-42), S(-29,-44), S(-39,-28) }
  },
  { // Rook
   { S(-25, 0), S(-15, 0), S(-15, 0), S(-9, 0) },
   { S(-23, 0), S( -8, 0), S( -3, 0), S( 0, 0) },
   { S(-21, 0), S( -9, 0), S( -4, 0), S( 2, 0) },
   { S(-22, 0), S( -6, 0), S( -1, 0), S( 2, 0) },
   { S(-22, 0), S( -7, 0), S(  0, 0), S( 1, 0) },
   { S(-21, 0), S( -7, 0), S(  0, 0), S( 2, 0) },
   { S(-12, 0), S(  4, 0), S(  8, 0), S(12, 0) },
   { S(-25, 0), S(-15, 0), S(-11, 0), S(-5, 0) }
  },
  { // Queen
   { S( 0,-69), S(-3,-57), S(-4,-44), S(-1,-28) },
   { S(-4,-54), S( 6,-29), S( 9,-21), S( 8, -4) },
   { S(-2,-38), S( 6,-16), S( 9, -7), S( 9,  5) },
   { S(-1,-29), S( 8, -5), S( 9,  9), S( 7, 18) },
   { S(-3,-27), S( 9, -5), S( 8, 10), S( 7, 24) },
   { S(-2,-39), S( 5,-16), S( 7,-11), S(10,  3) },
   { S(-2,-56), S( 7,-29), S( 7,-22), S( 6, -7) },
   { S(-1,-75), S(-4,-53), S(-1,-44), S( 0,-30) }
  },
  { // King
   { S(282, 28), S(346, 67), S(292,102), S(227,108) },
   { S(293, 61), S(326,118), S(255,162), S(210,156) },
   { S(224,104), S(269,164), S(204,178), S(135,189) },
   { S(203,118), S(215,190), S(180,196), S(134,196) },
   { S(169,123), S(210,177), S(144,222), S( 92,221) },
   { S(141,112), S(198,175), S(114,193), S( 68,190) },
   { S(111, 66), S(165,116), S( 93,142), S( 47,162) },
   { S( 91, 22), S(112, 73), S( 78, 97), S( 30,106) }
  }
};

#undef S

Score psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];

// init() initializes piece-square tables: the white halves of the tables are
// copied from Bonus[] adding the piece value, then the black halves of the
// tables are initialized by flipping and changing the sign of the white scores.
void init() {

  for (PieceType pt = PAWN; pt <= KING; ++pt)
  {
      PieceValue[MG][make_piece(BLACK, pt)] = PieceValue[MG][pt];
      PieceValue[EG][make_piece(BLACK, pt)] = PieceValue[EG][pt];

      Score v = make_score(PieceValue[MG][pt], PieceValue[EG][pt]);

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          File f = std::min(file_of(s), FILE_H - file_of(s));
          psq[WHITE][pt][ s] = v + Bonus[pt][rank_of(s)][f];
          psq[BLACK][pt][~s] = -psq[WHITE][pt][s];
      }
  }
}

} // namespace PSQT
