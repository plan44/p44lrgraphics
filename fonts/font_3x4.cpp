// SPDX-License-Identifier: CC0-1.0
//
// This is a font file to be included from textview.cpp to define a font

// Original Font Author's notice
//   Font Glyph Data is Copyleft. That is, it is 100% free, as in speech and in beer.
//   github: https://github.com/Michaelangel007/nanofont3x4

#if 0

// MARK: original font data array from nanofont3x4.cpp by Michael "Code Poet" Pohoreski Michaelangel007

// This entire font glyph data is Copyleft 2015 since virtual "ownership" of bits (numbers) is retarded.
//
// 3x4 Monochrome Font Glyphs within 4x4 cell
// Each nibble is one row, with the bits stored in reverse order: msb = left column, lsb = right column
// The bit layout for the 4x4 cell format is uppercase A is:
//   abcd    Row0  _X__  0100
//   efgh    Row1  XXX_  1110
//   ijkl    Row2  X_X_  1010
//   mnop    Row3  ____  0000
// If you are curious: There are 64K permutations of a 4x4 monochrome cell.
// You will want to examine the output of: all_4x4_bitmaps.cpp
// ========================================================================
uint16_t aGlyphs3x4[] =
{           // Hex    [1] [2] [3] [4] Scanlines
    0x0000, // 0x20
    0x4400, // 0x21 ! 010 010 000
    0xAA00, // 0x22 " 101 101 000
    0xAEE0, // 0x23 # 101 111 111
    0x64C4, // 0x24 $ 011 010 110 010
    0xCE60, // 0x25 % 110 111 011
    0x4C60, // 0x26 & 010 110 011
    0x4000, // 0x27 ' 010 000 000
    0x4840, // 0x28 ( 010 100 010
    0x4240, // 0x29 ) 010 001 010
    0x6600, // 0x2A * 011 011 000
    0x4E40, // 0x2B + 010 111 010
    0x0088, // 0x2C , 000 000 100 100
    0x0E00, // 0x2D - 000 111 000
    0x0080, // 0x2E . 000 000 100
    0x2480, // 0x2F / 001 010 100

    0x4A40, // 0x30 0 010 101 010
    0x8440, // 0x31 1 100 010 010
    0xC460, // 0x32 2 110 010 011
    0xE6E0, // 0x33 3 111 011 111
    0xAE20, // 0x34 4 101 111 001
    0x64C0, // 0x35 5 011 010 110
    0x8EE0, // 0x36 6 100 111 111
    0xE220, // 0x37 7 111 001 001
    0x6EC0, // 0x38 8 011 111 110
    0xEE20, // 0x39 9 111 111 001
    0x4040, // 0x3A : 010 000 010
    0x0448, // 0x3B ; 000 010 010 100
    0x4840, // 0x3C < 010 100 010
    0xE0E0, // 0x3D = 111 000 111
    0x4240, // 0x3E > 010 001 010
    0x6240, // 0x3F ? 011 001 010

    0xCC20, // 0x40 @ 110 110 001 // 0 = 000_
    0x4EA0, // 0x41 A 010 111 101 // 2 = 001_
    0xCEE0, // 0x42 B 110 111 111 // 4 = 010_
    0x6860, // 0x43 C 011 100 011 // 6 = 011_
    0xCAC0, // 0x44 D 110 101 110 // 8 = 100_
    0xECE0, // 0x45 E 111 110 111 // A = 101_
    0xEC80, // 0x46 F 111 110 100 // C = 110_
    0xCAE0, // 0x47 G 110 101 111 // E = 111_
    0xAEA0, // 0x48 H 101 111 101
    0x4440, // 0x49 I 010 010 010
    0x22C0, // 0x4A J 001 001 110
    0xACA0, // 0x4B K 101 110 101
    0x88E0, // 0x4C L 100 100 111
    0xEEA0, // 0x4D M 111 111 101
    0xEAA0, // 0x4E N 111 101 101
    0xEAE0, // 0x4F O 111 101 111

    0xEE80, // 0x50 P 111 111 100
    0xEAC0, // 0x51 Q 111 101 110
    0xCEA0, // 0x52 R 110 111 101
    0x64C0, // 0x53 S 011 010 110
    0xE440, // 0x54 T 111 010 010
    0xAAE0, // 0x55 U 101 101 111
    0xAA40, // 0x56 V 101 101 010
    0xAEE0, // 0x57 W 101 111 111
    0xA4A0, // 0x58 X 101 010 101
    0xA440, // 0x59 Y 101 010 010
    0xE4E0, // 0x5A Z 111 010 111
    0xC8C0, // 0x5B [ 110 100 110
    0x8420, // 0x5C \ 100 010 001
    0x6260, // 0x5D ] 011 001 011
    0x4A00, // 0x5E ^ 010 101 000
    0x00E0, // 0x5F _ 000 000 111

    0x8400, // 0x60 ` 100 010 000
    0x04C0, // 0x61 a 000 010 110
    0x8CC0, // 0x62 b 100 110 110
    0x0CC0, // 0x63 c 000 110 110
    0x4CC0, // 0x64 d 010 110 110
    0x08C0, // 0x65 e 000 100 110
    0x4880, // 0x66 f 010 100 100
    0x0CCC, // 0x67 g 000 110 110 110
    0x8CC0, // 0x68 h 100 110 110
    0x0880, // 0x69 i 000 100 100
    0x0448, // 0x6A j 000 010 010 100
    0x8CA0, // 0x6B k 100 110 101
    0x8840, // 0x6C l 100 100 010
    0x0CE0, // 0x6D m 000 110 111
    0x0CA0, // 0x6E n 000 110 101
    0x0CC0, // 0x6F o 000 110 110

    0x0CC8, // 0x70 p 000 110 110 100
    0x0CC4, // 0x71 q 000 110 110 010
    0x0C80, // 0x72 r 000 110 100
    0x0480, // 0x73 s 000 010 100
    0x4C60, // 0x74 t 010 110 011
    0x0AE0, // 0x75 u 000 101 111
    0x0A40, // 0x76 v 000 101 010
    0x0E60, // 0x77 w 000 111 011
    0x0CC0, // 0x78 x 000 110 110
    0x0AE2, // 0x79 y 000 101 111 001
    0x0840, // 0x7A z 000 100 010
    0x6C60, // 0x7B { 011 110 011
    0x4444, // 0x7C | 010 010 010 010
    0xC6C0, // 0x7D } 110 011 110
    0x6C00, // 0x7E ~ 011 110 000
    0xA4A4  // 0x7F   101 010 101 010 // Alternative: Could even have a "full" 4x4 checkerboard
};


// MARK: function to generate p44lrgraphic glyphs from original data

void p44lrgraphics_out()
{
  int n = sizeof(aGlyphs3x4)/sizeof(uint16_t);
  uint8_t col[3];
  for(int i=0; i<n; i++) {
    // we need to extract 3 columns, from left to right
    uint16_t g = aGlyphs3x4[i];
    for(int c=0; c<3; c++) {
      // of 4 rows (including descenders), LSBit=top row
      col[c] = 0;
      for (int r=0; r<4; r++) {
        col[c] = col[c] |
          ( ( ( (g>>4*(3-r)) & (1<<(3-c)) )!=0 ? 1 : 0) <<r );
      }
    }
    // print
    printf(" {  3, \"\\x%02x\\x%02x\\x%02x\"                             },  // '%c' UTF8 %02X\n", col[0], col[1], col[2], i+' ', i+0x20);
  }
}


#endif


#ifdef GENERATE_FONT_SOURCE

// MARK: - '3x4' generated font verification data

static const char * font_3x4_glyphstrings[] = {
  "placeholder" /* 0x00 - Glyph 0 */

  "\n"   "XXXX"   // 0F
  "\n"   "X..X"   // 09
  "\n"   "XXXX" , // 0F

  " " /* UTF-8 20 - Glyph 1 */

  "\n"   "...."   // 00
  "\n"   "...."   // 00
  "\n"   "...." , // 00

  "!" /* UTF-8 21 - Glyph 2 */

  "\n"   "...."   // 00
  "\n"   "..XX"   // 03
  "\n"   "...." , // 00

  "\"" /* UTF-8 22 - Glyph 3 */

  "\n"   "..XX"   // 03
  "\n"   "...."   // 00
  "\n"   "..XX" , // 03

  "#" /* UTF-8 23 - Glyph 4 */

  "\n"   ".XXX"   // 07
  "\n"   ".XX."   // 06
  "\n"   ".XXX" , // 07

  "$" /* UTF-8 24 - Glyph 5 */

  "\n"   ".X.."   // 04
  "\n"   "XXXX"   // 0F
  "\n"   "...X" , // 01

  "%" /* UTF-8 25 - Glyph 6 */

  "\n"   "..XX"   // 03
  "\n"   ".XXX"   // 07
  "\n"   ".XX." , // 06

  "&" /* UTF-8 26 - Glyph 7 */

  "\n"   "..X."   // 02
  "\n"   ".XXX"   // 07
  "\n"   ".X.." , // 04

  "'" /* UTF-8 27 - Glyph 8 */

  "\n"   "...."   // 00
  "\n"   "...X"   // 01
  "\n"   "...." , // 00

  "(" /* UTF-8 28 - Glyph 9 */

  "\n"   "..X."   // 02
  "\n"   ".X.X"   // 05
  "\n"   "...." , // 00

  ")" /* UTF-8 29 - Glyph 10 */

  "\n"   "...."   // 00
  "\n"   ".X.X"   // 05
  "\n"   "..X." , // 02

  "*" /* UTF-8 2A - Glyph 11 */

  "\n"   "...."   // 00
  "\n"   "..XX"   // 03
  "\n"   "..XX" , // 03

  "+" /* UTF-8 2B - Glyph 12 */

  "\n"   "..X."   // 02
  "\n"   ".XXX"   // 07
  "\n"   "..X." , // 02

  "," /* UTF-8 2C - Glyph 13 */

  "\n"   "XX.."   // 0C
  "\n"   "...."   // 00
  "\n"   "...." , // 00

  "-" /* UTF-8 2D - Glyph 14 */

  "\n"   "..X."   // 02
  "\n"   "..X."   // 02
  "\n"   "..X." , // 02

  "." /* UTF-8 2E - Glyph 15 */

  "\n"   ".X.."   // 04
  "\n"   "...."   // 00
  "\n"   "...." , // 00

  "/" /* UTF-8 2F - Glyph 16 */

  "\n"   ".X.."   // 04
  "\n"   "..X."   // 02
  "\n"   "...X" , // 01

  "0" /* UTF-8 30 - Glyph 17 */

  "\n"   "..X."   // 02
  "\n"   ".X.X"   // 05
  "\n"   "..X." , // 02

  "1" /* UTF-8 31 - Glyph 18 */

  "\n"   "...X"   // 01
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "2" /* UTF-8 32 - Glyph 19 */

  "\n"   "...X"   // 01
  "\n"   ".XXX"   // 07
  "\n"   ".X.." , // 04

  "3" /* UTF-8 33 - Glyph 20 */

  "\n"   ".X.X"   // 05
  "\n"   ".XXX"   // 07
  "\n"   ".XXX" , // 07

  "4" /* UTF-8 34 - Glyph 21 */

  "\n"   "..XX"   // 03
  "\n"   "..X."   // 02
  "\n"   ".XXX" , // 07

  "5" /* UTF-8 35 - Glyph 22 */

  "\n"   ".X.."   // 04
  "\n"   ".XXX"   // 07
  "\n"   "...X" , // 01

  "6" /* UTF-8 36 - Glyph 23 */

  "\n"   ".XXX"   // 07
  "\n"   ".XX."   // 06
  "\n"   ".XX." , // 06

  "7" /* UTF-8 37 - Glyph 24 */

  "\n"   "...X"   // 01
  "\n"   "...X"   // 01
  "\n"   ".XXX" , // 07

  "8" /* UTF-8 38 - Glyph 25 */

  "\n"   ".XX."   // 06
  "\n"   ".XXX"   // 07
  "\n"   "..XX" , // 03

  "9" /* UTF-8 39 - Glyph 26 */

  "\n"   "..XX"   // 03
  "\n"   "..XX"   // 03
  "\n"   ".XXX" , // 07

  ":" /* UTF-8 3A - Glyph 27 */

  "\n"   "...."   // 00
  "\n"   ".X.X"   // 05
  "\n"   "...." , // 00

  ";" /* UTF-8 3B - Glyph 28 */

  "\n"   "X..."   // 08
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "<" /* UTF-8 3C - Glyph 29 */

  "\n"   "..X."   // 02
  "\n"   ".X.X"   // 05
  "\n"   "...." , // 00

  "=" /* UTF-8 3D - Glyph 30 */

  "\n"   ".X.X"   // 05
  "\n"   ".X.X"   // 05
  "\n"   ".X.X" , // 05

  ">" /* UTF-8 3E - Glyph 31 */

  "\n"   "...."   // 00
  "\n"   ".X.X"   // 05
  "\n"   "..X." , // 02

  "?" /* UTF-8 3F - Glyph 32 */

  "\n"   "...."   // 00
  "\n"   ".X.X"   // 05
  "\n"   "..XX" , // 03

  "@" /* UTF-8 40 - Glyph 33 */

  "\n"   "..XX"   // 03
  "\n"   "..XX"   // 03
  "\n"   ".X.." , // 04

  "A" /* UTF-8 41 - Glyph 34 */

  "\n"   ".XX."   // 06
  "\n"   "..XX"   // 03
  "\n"   ".XX." , // 06

  "B" /* UTF-8 42 - Glyph 35 */

  "\n"   ".XXX"   // 07
  "\n"   ".XXX"   // 07
  "\n"   ".XX." , // 06

  "C" /* UTF-8 43 - Glyph 36 */

  "\n"   "..X."   // 02
  "\n"   ".X.X"   // 05
  "\n"   ".X.X" , // 05

  "D" /* UTF-8 44 - Glyph 37 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.X"   // 05
  "\n"   "..X." , // 02

  "E" /* UTF-8 45 - Glyph 38 */

  "\n"   ".XXX"   // 07
  "\n"   ".XXX"   // 07
  "\n"   ".X.X" , // 05

  "F" /* UTF-8 46 - Glyph 39 */

  "\n"   ".XXX"   // 07
  "\n"   "..XX"   // 03
  "\n"   "...X" , // 01

  "G" /* UTF-8 47 - Glyph 40 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.X"   // 05
  "\n"   ".XX." , // 06

  "H" /* UTF-8 48 - Glyph 41 */

  "\n"   ".XXX"   // 07
  "\n"   "..X."   // 02
  "\n"   ".XXX" , // 07

  "I" /* UTF-8 49 - Glyph 42 */

  "\n"   "...."   // 00
  "\n"   ".XXX"   // 07
  "\n"   "...." , // 00

  "J" /* UTF-8 4A - Glyph 43 */

  "\n"   ".X.."   // 04
  "\n"   ".X.."   // 04
  "\n"   "..XX" , // 03

  "K" /* UTF-8 4B - Glyph 44 */

  "\n"   ".XXX"   // 07
  "\n"   "..X."   // 02
  "\n"   ".X.X" , // 05

  "L" /* UTF-8 4C - Glyph 45 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.."   // 04
  "\n"   ".X.." , // 04

  "M" /* UTF-8 4D - Glyph 46 */

  "\n"   ".XXX"   // 07
  "\n"   "..XX"   // 03
  "\n"   ".XXX" , // 07

  "N" /* UTF-8 4E - Glyph 47 */

  "\n"   ".XXX"   // 07
  "\n"   "...X"   // 01
  "\n"   ".XXX" , // 07

  "O" /* UTF-8 4F - Glyph 48 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.X"   // 05
  "\n"   ".XXX" , // 07

  "P" /* UTF-8 50 - Glyph 49 */

  "\n"   ".XXX"   // 07
  "\n"   "..XX"   // 03
  "\n"   "..XX" , // 03

  "Q" /* UTF-8 51 - Glyph 50 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.X"   // 05
  "\n"   "..XX" , // 03

  "R" /* UTF-8 52 - Glyph 51 */

  "\n"   ".XXX"   // 07
  "\n"   "..XX"   // 03
  "\n"   ".XX." , // 06

  "S" /* UTF-8 53 - Glyph 52 */

  "\n"   ".X.."   // 04
  "\n"   ".XXX"   // 07
  "\n"   "...X" , // 01

  "T" /* UTF-8 54 - Glyph 53 */

  "\n"   "...X"   // 01
  "\n"   ".XXX"   // 07
  "\n"   "...X" , // 01

  "U" /* UTF-8 55 - Glyph 54 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.."   // 04
  "\n"   ".XXX" , // 07

  "V" /* UTF-8 56 - Glyph 55 */

  "\n"   "..XX"   // 03
  "\n"   ".X.."   // 04
  "\n"   "..XX" , // 03

  "W" /* UTF-8 57 - Glyph 56 */

  "\n"   ".XXX"   // 07
  "\n"   ".XX."   // 06
  "\n"   ".XXX" , // 07

  "X" /* UTF-8 58 - Glyph 57 */

  "\n"   ".X.X"   // 05
  "\n"   "..X."   // 02
  "\n"   ".X.X" , // 05

  "Y" /* UTF-8 59 - Glyph 58 */

  "\n"   "...X"   // 01
  "\n"   ".XX."   // 06
  "\n"   "...X" , // 01

  "Z" /* UTF-8 5A - Glyph 59 */

  "\n"   ".X.X"   // 05
  "\n"   ".XXX"   // 07
  "\n"   ".X.X" , // 05

  "[" /* UTF-8 5B - Glyph 60 */

  "\n"   ".XXX"   // 07
  "\n"   ".X.X"   // 05
  "\n"   "...." , // 00

  "\\" /* UTF-8 5C - Glyph 61 */

  "\n"   "...X"   // 01
  "\n"   "..X."   // 02
  "\n"   ".X.." , // 04

  "]" /* UTF-8 5D - Glyph 62 */

  "\n"   "...."   // 00
  "\n"   ".X.X"   // 05
  "\n"   ".XXX" , // 07

  "^" /* UTF-8 5E - Glyph 63 */

  "\n"   "..X."   // 02
  "\n"   "...X"   // 01
  "\n"   "..X." , // 02

  "_" /* UTF-8 5F - Glyph 64 */

  "\n"   ".X.."   // 04
  "\n"   ".X.."   // 04
  "\n"   ".X.." , // 04

  "`" /* UTF-8 60 - Glyph 65 */

  "\n"   "...X"   // 01
  "\n"   "..X."   // 02
  "\n"   "...." , // 00

  "a" /* UTF-8 61 - Glyph 66 */

  "\n"   ".X.."   // 04
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "b" /* UTF-8 62 - Glyph 67 */

  "\n"   ".XXX"   // 07
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "c" /* UTF-8 63 - Glyph 68 */

  "\n"   ".XX."   // 06
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "d" /* UTF-8 64 - Glyph 69 */

  "\n"   ".XX."   // 06
  "\n"   ".XXX"   // 07
  "\n"   "...." , // 00

  "e" /* UTF-8 65 - Glyph 70 */

  "\n"   ".XX."   // 06
  "\n"   ".X.."   // 04
  "\n"   "...." , // 00

  "f" /* UTF-8 66 - Glyph 71 */

  "\n"   ".XX."   // 06
  "\n"   "...X"   // 01
  "\n"   "...." , // 00

  "g" /* UTF-8 67 - Glyph 72 */

  "\n"   "XXX."   // 0E
  "\n"   "XXX."   // 0E
  "\n"   "...." , // 00

  "h" /* UTF-8 68 - Glyph 73 */

  "\n"   ".XXX"   // 07
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "i" /* UTF-8 69 - Glyph 74 */

  "\n"   ".XX."   // 06
  "\n"   "...."   // 00
  "\n"   "...." , // 00

  "j" /* UTF-8 6A - Glyph 75 */

  "\n"   "X..."   // 08
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "k" /* UTF-8 6B - Glyph 76 */

  "\n"   ".XXX"   // 07
  "\n"   "..X."   // 02
  "\n"   ".X.." , // 04

  "l" /* UTF-8 6C - Glyph 77 */

  "\n"   "..XX"   // 03
  "\n"   ".X.."   // 04
  "\n"   "...." , // 00

  "m" /* UTF-8 6D - Glyph 78 */

  "\n"   ".XX."   // 06
  "\n"   ".XX."   // 06
  "\n"   ".X.." , // 04

  "n" /* UTF-8 6E - Glyph 79 */

  "\n"   ".XX."   // 06
  "\n"   "..X."   // 02
  "\n"   ".X.." , // 04

  "o" /* UTF-8 6F - Glyph 80 */

  "\n"   ".XX."   // 06
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "p" /* UTF-8 70 - Glyph 81 */

  "\n"   "XXX."   // 0E
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "q" /* UTF-8 71 - Glyph 82 */

  "\n"   ".XX."   // 06
  "\n"   "XXX."   // 0E
  "\n"   "...." , // 00

  "r" /* UTF-8 72 - Glyph 83 */

  "\n"   ".XX."   // 06
  "\n"   "..X."   // 02
  "\n"   "...." , // 00

  "s" /* UTF-8 73 - Glyph 84 */

  "\n"   ".X.."   // 04
  "\n"   "..X."   // 02
  "\n"   "...." , // 00

  "t" /* UTF-8 74 - Glyph 85 */

  "\n"   "..X."   // 02
  "\n"   ".XXX"   // 07
  "\n"   ".X.." , // 04

  "u" /* UTF-8 75 - Glyph 86 */

  "\n"   ".XX."   // 06
  "\n"   ".X.."   // 04
  "\n"   ".XX." , // 06

  "v" /* UTF-8 76 - Glyph 87 */

  "\n"   "..X."   // 02
  "\n"   ".X.."   // 04
  "\n"   "..X." , // 02

  "w" /* UTF-8 77 - Glyph 88 */

  "\n"   "..X."   // 02
  "\n"   ".XX."   // 06
  "\n"   ".XX." , // 06

  "x" /* UTF-8 78 - Glyph 89 */

  "\n"   ".XX."   // 06
  "\n"   ".XX."   // 06
  "\n"   "...." , // 00

  "y" /* UTF-8 79 - Glyph 90 */

  "\n"   ".XX."   // 06
  "\n"   ".X.."   // 04
  "\n"   "XXX." , // 0E

  "z" /* UTF-8 7A - Glyph 91 */

  "\n"   "..X."   // 02
  "\n"   ".X.."   // 04
  "\n"   "...." , // 00

  "{" /* UTF-8 7B - Glyph 92 */

  "\n"   "..X."   // 02
  "\n"   ".XXX"   // 07
  "\n"   ".X.X" , // 05

  "|" /* UTF-8 7C - Glyph 93 */

  "\n"   "...."   // 00
  "\n"   "XXXX"   // 0F
  "\n"   "...." , // 00

  "}" /* UTF-8 7D - Glyph 94 */

  "\n"   ".X.X"   // 05
  "\n"   ".XXX"   // 07
  "\n"   "..X." , // 02

  "~" /* UTF-8 7E - Glyph 95 */

  "\n"   "..X."   // 02
  "\n"   "..XX"   // 03
  "\n"   "...X" , // 01

  "\x7f" /* UTF-8 7F - Glyph 96 */

  "\n"   ".X.X"   // 05
  "\n"   "X.X."   // 0A
  "\n"   ".X.X" , // 05

  nullptr // terminator
};

// MARK: - end of generated font verification data


#endif // GENERATE_FONT_SOURCE


// MARK: - '3x4' generated font data

static const glyph_t font_3x4_glyphs[] = {
  {  3, "\x0f\x09\x0f"                             },  // placeholder          (input # 0 -> glyph # 0)
  {  3, "\x00\x00\x00"                             },  // ' ' UTF-8 20         (input # 1 -> glyph # 1)
  {  3, "\x00\x03\x00"                             },  // '!' UTF-8 21         (input # 2 -> glyph # 2)
  {  3, "\x03\x00\x03"                             },  // '"' UTF-8 22         (input # 3 -> glyph # 3)
  {  3, "\x07\x06\x07"                             },  // '#' UTF-8 23         (input # 4 -> glyph # 4)
  {  3, "\x04\x0f\x01"                             },  // '$' UTF-8 24         (input # 5 -> glyph # 5)
  {  3, "\x03\x07\x06"                             },  // '%' UTF-8 25         (input # 6 -> glyph # 6)
  {  3, "\x02\x07\x04"                             },  // '&' UTF-8 26         (input # 7 -> glyph # 7)
  {  3, "\x00\x01\x00"                             },  // ''' UTF-8 27         (input # 8 -> glyph # 8)
  {  3, "\x02\x05\x00"                             },  // '(' UTF-8 28         (input # 9 -> glyph # 9)
  {  3, "\x00\x05\x02"                             },  // ')' UTF-8 29         (input # 10 -> glyph # 10)
  {  3, "\x00\x03\x03"                             },  // '*' UTF-8 2A         (input # 11 -> glyph # 11)
  {  3, "\x02\x07\x02"                             },  // '+' UTF-8 2B         (input # 12 -> glyph # 12)
  {  3, "\x0c\x00\x00"                             },  // ',' UTF-8 2C         (input # 13 -> glyph # 13)
  {  3, "\x02\x02\x02"                             },  // '-' UTF-8 2D         (input # 14 -> glyph # 14)
  {  3, "\x04\x00\x00"                             },  // '.' UTF-8 2E         (input # 15 -> glyph # 15)
  {  3, "\x04\x02\x01"                             },  // '/' UTF-8 2F         (input # 16 -> glyph # 16)
  {  3, "\x02\x05\x02"                             },  // '0' UTF-8 30         (input # 17 -> glyph # 17)
  {  3, "\x01\x06\x00"                             },  // '1' UTF-8 31         (input # 18 -> glyph # 18)
  {  3, "\x01\x07\x04"                             },  // '2' UTF-8 32         (input # 19 -> glyph # 19)
  {  3, "\x05\x07\x07"                             },  // '3' UTF-8 33         (input # 20 -> glyph # 20)
  {  3, "\x03\x02\x07"                             },  // '4' UTF-8 34         (input # 21 -> glyph # 21)
  {  3, "\x04\x07\x01"                             },  // '5' UTF-8 35         (input # 22 -> glyph # 22)
  {  3, "\x07\x06\x06"                             },  // '6' UTF-8 36         (input # 23 -> glyph # 23)
  {  3, "\x01\x01\x07"                             },  // '7' UTF-8 37         (input # 24 -> glyph # 24)
  {  3, "\x06\x07\x03"                             },  // '8' UTF-8 38         (input # 25 -> glyph # 25)
  {  3, "\x03\x03\x07"                             },  // '9' UTF-8 39         (input # 26 -> glyph # 26)
  {  3, "\x00\x05\x00"                             },  // ':' UTF-8 3A         (input # 27 -> glyph # 27)
  {  3, "\x08\x06\x00"                             },  // ';' UTF-8 3B         (input # 28 -> glyph # 28)
  {  3, "\x02\x05\x00"                             },  // '<' UTF-8 3C         (input # 29 -> glyph # 29)
  {  3, "\x05\x05\x05"                             },  // '=' UTF-8 3D         (input # 30 -> glyph # 30)
  {  3, "\x00\x05\x02"                             },  // '>' UTF-8 3E         (input # 31 -> glyph # 31)
  {  3, "\x00\x05\x03"                             },  // '?' UTF-8 3F         (input # 32 -> glyph # 32)
  {  3, "\x03\x03\x04"                             },  // '@' UTF-8 40         (input # 33 -> glyph # 33)
  {  3, "\x06\x03\x06"                             },  // 'A' UTF-8 41         (input # 34 -> glyph # 34)
  {  3, "\x07\x07\x06"                             },  // 'B' UTF-8 42         (input # 35 -> glyph # 35)
  {  3, "\x02\x05\x05"                             },  // 'C' UTF-8 43         (input # 36 -> glyph # 36)
  {  3, "\x07\x05\x02"                             },  // 'D' UTF-8 44         (input # 37 -> glyph # 37)
  {  3, "\x07\x07\x05"                             },  // 'E' UTF-8 45         (input # 38 -> glyph # 38)
  {  3, "\x07\x03\x01"                             },  // 'F' UTF-8 46         (input # 39 -> glyph # 39)
  {  3, "\x07\x05\x06"                             },  // 'G' UTF-8 47         (input # 40 -> glyph # 40)
  {  3, "\x07\x02\x07"                             },  // 'H' UTF-8 48         (input # 41 -> glyph # 41)
  {  3, "\x00\x07\x00"                             },  // 'I' UTF-8 49         (input # 42 -> glyph # 42)
  {  3, "\x04\x04\x03"                             },  // 'J' UTF-8 4A         (input # 43 -> glyph # 43)
  {  3, "\x07\x02\x05"                             },  // 'K' UTF-8 4B         (input # 44 -> glyph # 44)
  {  3, "\x07\x04\x04"                             },  // 'L' UTF-8 4C         (input # 45 -> glyph # 45)
  {  3, "\x07\x03\x07"                             },  // 'M' UTF-8 4D         (input # 46 -> glyph # 46)
  {  3, "\x07\x01\x07"                             },  // 'N' UTF-8 4E         (input # 47 -> glyph # 47)
  {  3, "\x07\x05\x07"                             },  // 'O' UTF-8 4F         (input # 48 -> glyph # 48)
  {  3, "\x07\x03\x03"                             },  // 'P' UTF-8 50         (input # 49 -> glyph # 49)
  {  3, "\x07\x05\x03"                             },  // 'Q' UTF-8 51         (input # 50 -> glyph # 50)
  {  3, "\x07\x03\x06"                             },  // 'R' UTF-8 52         (input # 51 -> glyph # 51)
  {  3, "\x04\x07\x01"                             },  // 'S' UTF-8 53         (input # 52 -> glyph # 52)
  {  3, "\x01\x07\x01"                             },  // 'T' UTF-8 54         (input # 53 -> glyph # 53)
  {  3, "\x07\x04\x07"                             },  // 'U' UTF-8 55         (input # 54 -> glyph # 54)
  {  3, "\x03\x04\x03"                             },  // 'V' UTF-8 56         (input # 55 -> glyph # 55)
  {  3, "\x07\x06\x07"                             },  // 'W' UTF-8 57         (input # 56 -> glyph # 56)
  {  3, "\x05\x02\x05"                             },  // 'X' UTF-8 58         (input # 57 -> glyph # 57)
  {  3, "\x01\x06\x01"                             },  // 'Y' UTF-8 59         (input # 58 -> glyph # 58)
  {  3, "\x05\x07\x05"                             },  // 'Z' UTF-8 5A         (input # 59 -> glyph # 59)
  {  3, "\x07\x05\x00"                             },  // '[' UTF-8 5B         (input # 60 -> glyph # 60)
  {  3, "\x01\x02\x04"                             },  // '\' UTF-8 5C         (input # 61 -> glyph # 61)
  {  3, "\x00\x05\x07"                             },  // ']' UTF-8 5D         (input # 62 -> glyph # 62)
  {  3, "\x02\x01\x02"                             },  // '^' UTF-8 5E         (input # 63 -> glyph # 63)
  {  3, "\x04\x04\x04"                             },  // '_' UTF-8 5F         (input # 64 -> glyph # 64)
  {  3, "\x01\x02\x00"                             },  // '`' UTF-8 60         (input # 65 -> glyph # 65)
  {  3, "\x04\x06\x00"                             },  // 'a' UTF-8 61         (input # 66 -> glyph # 66)
  {  3, "\x07\x06\x00"                             },  // 'b' UTF-8 62         (input # 67 -> glyph # 67)
  {  3, "\x06\x06\x00"                             },  // 'c' UTF-8 63         (input # 68 -> glyph # 68)
  {  3, "\x06\x07\x00"                             },  // 'd' UTF-8 64         (input # 69 -> glyph # 69)
  {  3, "\x06\x04\x00"                             },  // 'e' UTF-8 65         (input # 70 -> glyph # 70)
  {  3, "\x06\x01\x00"                             },  // 'f' UTF-8 66         (input # 71 -> glyph # 71)
  {  3, "\x0e\x0e\x00"                             },  // 'g' UTF-8 67         (input # 72 -> glyph # 72)
  {  3, "\x07\x06\x00"                             },  // 'h' UTF-8 68         (input # 73 -> glyph # 73)
  {  3, "\x06\x00\x00"                             },  // 'i' UTF-8 69         (input # 74 -> glyph # 74)
  {  3, "\x08\x06\x00"                             },  // 'j' UTF-8 6A         (input # 75 -> glyph # 75)
  {  3, "\x07\x02\x04"                             },  // 'k' UTF-8 6B         (input # 76 -> glyph # 76)
  {  3, "\x03\x04\x00"                             },  // 'l' UTF-8 6C         (input # 77 -> glyph # 77)
  {  3, "\x06\x06\x04"                             },  // 'm' UTF-8 6D         (input # 78 -> glyph # 78)
  {  3, "\x06\x02\x04"                             },  // 'n' UTF-8 6E         (input # 79 -> glyph # 79)
  {  3, "\x06\x06\x00"                             },  // 'o' UTF-8 6F         (input # 80 -> glyph # 80)
  {  3, "\x0e\x06\x00"                             },  // 'p' UTF-8 70         (input # 81 -> glyph # 81)
  {  3, "\x06\x0e\x00"                             },  // 'q' UTF-8 71         (input # 82 -> glyph # 82)
  {  3, "\x06\x02\x00"                             },  // 'r' UTF-8 72         (input # 83 -> glyph # 83)
  {  3, "\x04\x02\x00"                             },  // 's' UTF-8 73         (input # 84 -> glyph # 84)
  {  3, "\x02\x07\x04"                             },  // 't' UTF-8 74         (input # 85 -> glyph # 85)
  {  3, "\x06\x04\x06"                             },  // 'u' UTF-8 75         (input # 86 -> glyph # 86)
  {  3, "\x02\x04\x02"                             },  // 'v' UTF-8 76         (input # 87 -> glyph # 87)
  {  3, "\x02\x06\x06"                             },  // 'w' UTF-8 77         (input # 88 -> glyph # 88)
  {  3, "\x06\x06\x00"                             },  // 'x' UTF-8 78         (input # 89 -> glyph # 89)
  {  3, "\x06\x04\x0e"                             },  // 'y' UTF-8 79         (input # 90 -> glyph # 90)
  {  3, "\x02\x04\x00"                             },  // 'z' UTF-8 7A         (input # 91 -> glyph # 91)
  {  3, "\x02\x07\x05"                             },  // '{' UTF-8 7B         (input # 92 -> glyph # 92)
  {  3, "\x00\x0f\x00"                             },  // '|' UTF-8 7C         (input # 93 -> glyph # 93)
  {  3, "\x05\x07\x02"                             },  // '}' UTF-8 7D         (input # 94 -> glyph # 94)
  {  3, "\x02\x03\x01"                             },  // '~' UTF-8 7E         (input # 95 -> glyph # 95)
  {  3, "\x05\x0a\x05"                             },  // '' UTF-8 7F         (input # 96 -> glyph # 96)
};

static const GlyphRange font_3x4_ranges[] = {
  { "", 0x20, 0x7F, 1 }, //  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
  { NULL, 0, 0, 0 }
};

static const font_t font_3x4 = {
  .fontName = "3x4",
  .glyphHeight = 4,
  .numGlyphs = 97,
  .glyphRanges = font_3x4_ranges,
  .glyphs = font_3x4_glyphs
};

