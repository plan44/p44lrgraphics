//
//  Copyright (c) 2016-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of p44lrgraphics.
//
//  p44lrgraphics is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  p44lrgraphics is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with p44lrgraphics. If not, see <http://www.gnu.org/licenses/>.
//

#include "textview.hpp"

using namespace p44;



// MARK ====== Simple 7-pixel height dot matrix font
// Note: the font is derived from a monospaced 7*5 pixel font, but has been adjusted a bit
//       to get rendered proportionally (variable character width, e.g. "!" has width 1, whereas "m" has 7)
//       In the fontGlyphs table below, every char has a number of pixel colums it consists of, and then the
//       actual column values encoded as a string.

typedef struct {
  uint8_t width;
  const char *cols;
} glyph_t;

const int numGlyphs = 114; // 96 ASCII 0x20..0x7F plus 6 ÄÖÜäöü plus 12 èéêëàáâìíîïç
const int rowsPerGlyph = 7;


static const glyph_t fontGlyphs[numGlyphs] = {
  {  5, "\x00\x00\x00\x00\x00" },  // ' ' 0x20 (0)
  {  1, "\x5f" },  // '!' 0x21 (1)
  {  3, "\x03\x00\x03" },  // '"' 0x22 (2)
  {  5, "\x28\x7c\x28\x7c\x28" },  // '#' 0x23 (3)
  {  5, "\x24\x2a\x7f\x2a\x12" },  // '$' 0x24 (4)
  {  5, "\x4c\x2c\x10\x68\x64" },  // '%' 0x25 (5)
  {  5, "\x30\x4e\x55\x22\x40" },  // '&' 0x26 (6)
  {  1, "\x03" },  // ''' 0x27 (7)
  {  3, "\x1c\x22\x41" },  // '(' 0x28 (8)
  {  3, "\x41\x22\x1c" },  // ')' 0x29 (9)
  {  5, "\x14\x08\x3e\x08\x14" },  // '*' 0x2A (10)
  {  5, "\x08\x08\x3e\x08\x08" },  // '+' 0x2B (11)
  {  2, "\x50\x30" },  // ',' 0x2C (12)
  {  5, "\x08\x08\x08\x08\x08" },  // '-' 0x2D (13)
  {  2, "\x60\x60" },  // '.' 0x2E (14)
  {  6, "\x40\x20\x10\x08\x04\x02" },  // '/' 0x2F (15)
  {  5, "\x3e\x51\x49\x45\x3e" },  // '0' 0x30 (16)
  {  3, "\x42\x7f\x40" },  // '1' 0x31 (17)
  {  5, "\x62\x51\x49\x49\x46" },  // '2' 0x32 (18)
  {  5, "\x22\x41\x49\x49\x36" },  // '3' 0x33 (19)
  {  5, "\x1c\x12\x11\x7f\x10" },  // '4' 0x34 (20)
  {  5, "\x4f\x49\x49\x49\x31" },  // '5' 0x35 (21)
  {  5, "\x3e\x49\x49\x49\x32" },  // '6' 0x36 (22)
  {  5, "\x03\x01\x71\x09\x07" },  // '7' 0x37 (23)
  {  5, "\x36\x49\x49\x49\x36" },  // '8' 0x38 (24)
  {  5, "\x26\x49\x49\x49\x3e" },  // '9' 0x39 (25)
  {  2, "\x66\x66" },  // ':' 0x3A (26)
  {  2, "\x56\x36" },  // ';' 0x3B (27)
  {  4, "\x08\x14\x22\x41" },  // '<' 0x3C (28)
  {  4, "\x14\x14\x14\x14" },  // '=' 0x3D (29)
  {  4, "\x41\x22\x14\x08" },  // '>' 0x3E (30)
  {  5, "\x02\x01\x59\x09\x06" },  // '?' 0x3F (31)
  {  5, "\x3e\x41\x5d\x55\x5e" },  // '@' 0x40 (32)
  {  5, "\x7c\x0a\x09\x0a\x7c" },  // 'A' 0x41 (33)
  {  5, "\x7f\x49\x49\x49\x36" },  // 'B' 0x42 (34)
  {  5, "\x3e\x41\x41\x41\x22" },  // 'C' 0x43 (35)
  {  5, "\x7f\x41\x41\x22\x1c" },  // 'D' 0x44 (36)
  {  5, "\x7f\x49\x49\x41\x41" },  // 'E' 0x45 (37)
  {  5, "\x7f\x09\x09\x01\x01" },  // 'F' 0x46 (38)
  {  5, "\x3e\x41\x49\x49\x7a" },  // 'G' 0x47 (39)
  {  5, "\x7f\x08\x08\x08\x7f" },  // 'H' 0x48 (40)
  {  3, "\x41\x7f\x41" },  // 'I' 0x49 (41)
  {  5, "\x30\x40\x40\x40\x3f" },  // 'J' 0x4A (42)
  {  5, "\x7f\x08\x0c\x12\x61" },  // 'K' 0x4B (43)
  {  5, "\x7f\x40\x40\x40\x40" },  // 'L' 0x4C (44)
  {  7, "\x7f\x02\x04\x0c\x04\x02\x7f" },  // 'M' 0x4D (45)
  {  5, "\x7f\x02\x04\x08\x7f" },  // 'N' 0x4E (46)
  {  5, "\x3e\x41\x41\x41\x3e" },  // 'O' 0x4F (47)
  {  5, "\x7f\x09\x09\x09\x06" },  // 'P' 0x50 (48)
  {  5, "\x3e\x41\x51\x61\x7e" },  // 'Q' 0x51 (49)
  {  5, "\x7f\x09\x09\x09\x76" },  // 'R' 0x52 (50)
  {  5, "\x26\x49\x49\x49\x32" },  // 'S' 0x53 (51)
  {  5, "\x01\x01\x7f\x01\x01" },  // 'T' 0x54 (52)
  {  5, "\x3f\x40\x40\x40\x3f" },  // 'U' 0x55 (53)
  {  5, "\x1f\x20\x40\x20\x1f" },  // 'V' 0x56 (54)
  {  9, "\x0f\x30\x40\x30\x0c\x30\x40\x30\x0f" },  // 'W' 0x57 (55)
  {  5, "\x63\x14\x08\x14\x63" },  // 'X' 0x58 (56)
  {  5, "\x03\x04\x78\x04\x03" },  // 'Y' 0x59 (57)
  {  5, "\x61\x51\x49\x45\x43" },  // 'Z' 0x5A (58)
  {  3, "\x7f\x41\x41" },  // '[' 0x5B (59)
  {  5, "\x04\x08\x10\x20\x40" },  // '\' 0x5C (60)
  {  3, "\x41\x41\x7f" },  // ']' 0x5D (61)
  {  5, "\x04\x02\x01\x02\x04" },  // '^' 0x5E (62)
  {  5, "\x40\x40\x40\x40\x40" },  // '_' 0x5F (63)
  {  2, "\x01\x02" },  // '`' 0x60 (64)
  {  5, "\x20\x54\x54\x54\x78" },  // 'a' 0x61 (65)
  {  5, "\x7f\x44\x44\x44\x38" },  // 'b' 0x62 (66)
  {  5, "\x38\x44\x44\x44\x08" },  // 'c' 0x63 (67)
  {  5, "\x38\x44\x44\x44\x7f" },  // 'd' 0x64 (68)
  {  5, "\x38\x54\x54\x54\x18" },  // 'e' 0x65 (69)
  {  5, "\x08\x7e\x09\x09\x02" },  // 'f' 0x66 (70)
  {  5, "\x48\x54\x54\x54\x38" },  // 'g' 0x67 (71)
  {  5, "\x7f\x08\x08\x08\x70" },  // 'h' 0x68 (72)
  {  3, "\x48\x7a\x40" },  // 'i' 0x69 (73)
  {  5, "\x20\x40\x40\x48\x3a" },  // 'j' 0x6A (74)
  {  4, "\x7f\x10\x28\x44" },  // 'k' 0x6B (75)
  {  3, "\x3f\x40\x40" },  // 'l' 0x6C (76)
  {  7, "\x7c\x04\x04\x38\x04\x04\x78" },  // 'm' 0x6D (77)
  {  5, "\x7c\x04\x04\x04\x78" },  // 'n' 0x6E (78)
  {  5, "\x38\x44\x44\x44\x38" },  // 'o' 0x6F (79)
  {  5, "\x7c\x14\x14\x14\x08" },  // 'p' 0x70 (80)
  {  5, "\x08\x14\x14\x7c\x40" },  // 'q' 0x71 (81)
  {  5, "\x7c\x04\x04\x04\x08" },  // 'r' 0x72 (82)
  {  5, "\x48\x54\x54\x54\x24" },  // 's' 0x73 (83)
  {  5, "\x04\x04\x7f\x44\x44" },  // 't' 0x74 (84)
  {  5, "\x3c\x40\x40\x40\x7c" },  // 'u' 0x75 (85)
  {  5, "\x1c\x20\x40\x20\x1c" },  // 'v' 0x76 (86)
  {  7, "\x7c\x40\x40\x38\x40\x40\x7c" },  // 'w' 0x77 (87)
  {  5, "\x44\x28\x10\x28\x44" },  // 'x' 0x78 (88)
  {  5, "\x0c\x50\x50\x50\x3c" },  // 'y' 0x79 (89)
  {  5, "\x44\x64\x54\x4c\x44" },  // 'z' 0x7A (90)
  {  3, "\x08\x36\x41" },  // '{' 0x7B (91)
  {  1, "\x7f" },  // '|' 0x7C (92)
  {  3, "\x41\x36\x08" },  // '}' 0x7D (93)
  {  5, "\x04\x02\x04\x08\x04" },  // '~' 0x7E (94)
  {  5, "\x7f\x41\x41\x41\x7f" },  // '' 0x7F (95)
  {  5, "\x7d\x0a\x09\x0a\x7d" },  // '\200' 0x80 (96)
  {  5, "\x3d\x42\x42\x42\x3d" },  // '\201' 0x81 (97)
  {  5, "\x3d\x40\x40\x40\x3d" },  // '\202' 0x82 (98)
  {  5, "\x20\x55\x54\x55\x78" },  // '\203' 0x83 (99)
  {  5, "\x38\x45\x44\x45\x38" },  // '\204' 0x84 (100)
  {  5, "\x3c\x41\x40\x41\x7c" },  // '\205' 0x85 (101)
  {  5, "\x38\x55\x56\x54\x18" },  // '\206' 0x86 (102)
  {  5, "\x38\x54\x56\x55\x18" },  // '\207' 0x87 (103)
  {  5, "\x38\x56\x55\x56\x18" },  // '\210' 0x88 (104)
  {  5, "\x38\x55\x54\x55\x18" },  // '\211' 0x89 (105)
  {  5, "\x20\x55\x56\x54\x78" },  // '\212' 0x8A (106)
  {  5, "\x20\x54\x56\x55\x78" },  // '\213' 0x8B (107)
  {  5, "\x20\x56\x55\x56\x78" },  // '\214' 0x8C (108)
  {  3, "\x4a\x78\x42" },  // '\215' 0x8D (109)
  {  3, "\x4a\x78\x42" },  // '\216' 0x8E (110)
  {  3, "\x4a\x79\x42" },  // '\217' 0x8F (111)
  {  3, "\x4a\x78\x42" },  // '\220' 0x90 (112)
  {  5, "\x38\x44\x64\x44\x08" },  // '\221' 0x91 (113)
};


#define TEXT_FONT_GEN 0
#if TEXT_FONT_GEN

static const char * fontBin[numGlyphs] = {
  // ' ' 0x20 (0)
  "........\n" // 0x00
  "........\n" // 0x00
  "........\n" // 0x00
  "........\n" // 0x00
  "........",  // 0x00
  // '!' 0x21 (1)
  ".X.XXXXX",  // 0x5F
  // '"' 0x22 (2)
  "......XX\n" // 0x03
  "........\n" // 0x00
  "......XX",  // 0x03
  // '#' 0x23 (3)
  "..X.X...\n" // 0x28
  ".XXXXX..\n" // 0x7C
  "..X.X...\n" // 0x28
  ".XXXXX..\n" // 0x7C
  "..X.X...",  // 0x28
  // '$' 0x24 (4)
  "..X..X..\n" // 0x24
  "..X.X.X.\n" // 0x2A
  ".XXXXXXX\n" // 0x7F
  "..X.X.X.\n" // 0x2A
  "...X..X.",  // 0x12
  // '%' 0x25 (5)
  ".X..XX..\n" // 0x4C
  "..X.XX..\n" // 0x2C
  "...X....\n" // 0x10
  ".XX.X...\n" // 0x68
  ".XX..X..",  // 0x64
  // '&' 0x26 (6)
  "..XX....\n" // 0x30
  ".X..XXX.\n" // 0x4E
  ".X.X.X.X\n" // 0x55
  "..X...X.\n" // 0x22
  ".X......",  // 0x40
  // ''' 0x27 (7)
  "......XX",  // 0x03
  // '(' 0x28 (8)
  "...XXX..\n" // 0x1C
  "..X...X.\n" // 0x22
  ".X.....X",  // 0x41
  // ')' 0x29 (9)
  ".X.....X\n" // 0x41
  "..X...X.\n" // 0x22
  "...XXX..",  // 0x1C
  // '*' 0x2A (10)
  "...X.X..\n" // 0x14
  "....X...\n" // 0x08
  "..XXXXX.\n" // 0x3E
  "....X...\n" // 0x08
  "...X.X..",  // 0x14
  // '+' 0x2B (11)
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "..XXXXX.\n" // 0x3E
  "....X...\n" // 0x08
  "....X...",  // 0x08
  // ',' 0x2C (12)
  ".X.X....\n" // 0x50
  "..XX....",  // 0x30
  // '-' 0x2D (13)
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "....X...",  // 0x08
  // '.' 0x2E (14)
  ".XX.....\n" // 0x60
  ".XX.....",  // 0x60
  // '/' 0x2F (15)
  ".X......\n" // 0x40
  "..X.....\n" // 0x20
  "...X....\n" // 0x10
  "....X...\n" // 0x08
  ".....X..\n" // 0x04
  "......X.",  // 0x02
  // '0' 0x30 (16)
  "..XXXXX.\n" // 0x3E
  ".X.X...X\n" // 0x51
  ".X..X..X\n" // 0x49
  ".X...X.X\n" // 0x45
  "..XXXXX.",  // 0x3E
  // '1' 0x31 (17)
  ".X....X.\n" // 0x42
  ".XXXXXXX\n" // 0x7F
  ".X......",  // 0x40
  // '2' 0x32 (18)
  ".XX...X.\n" // 0x62
  ".X.X...X\n" // 0x51
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X...XX.",  // 0x46
  // '3' 0x33 (19)
  "..X...X.\n" // 0x22
  ".X.....X\n" // 0x41
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX.XX.",  // 0x36
  // '4' 0x34 (20)
  "...XXX..\n" // 0x1C
  "...X..X.\n" // 0x12
  "...X...X\n" // 0x11
  ".XXXXXXX\n" // 0x7F
  "...X....",  // 0x10
  // '5' 0x35 (21)
  ".X..XXXX\n" // 0x4F
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX...X",  // 0x31
  // '6' 0x36 (22)
  "..XXXXX.\n" // 0x3E
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX..X.",  // 0x32
  // '7' 0x37 (23)
  "......XX\n" // 0x03
  ".......X\n" // 0x01
  ".XXX...X\n" // 0x71
  "....X..X\n" // 0x09
  ".....XXX",  // 0x07
  // '8' 0x38 (24)
  "..XX.XX.\n" // 0x36
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX.XX.",  // 0x36
  // '9' 0x39 (25)
  "..X..XX.\n" // 0x26
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XXXXX.",  // 0x3E
  // ':' 0x3A (26)
  ".XX..XX.\n" // 0x66
  ".XX..XX.",  // 0x66
  // ';' 0x3B (27)
  ".X.X.XX.\n" // 0x56
  "..XX.XX.",  // 0x36
  // '<' 0x3C (28)
  "....X...\n" // 0x08
  "...X.X..\n" // 0x14
  "..X...X.\n" // 0x22
  ".X.....X",  // 0x41
  // '=' 0x3D (29)
  "...X.X..\n" // 0x14
  "...X.X..\n" // 0x14
  "...X.X..\n" // 0x14
  "...X.X..",  // 0x14
  // '>' 0x3E (30)
  ".X.....X\n" // 0x41
  "..X...X.\n" // 0x22
  "...X.X..\n" // 0x14
  "....X...",  // 0x08
  // '?' 0x3F (31)
  "......X.\n" // 0x02
  ".......X\n" // 0x01
  ".X.XX..X\n" // 0x59
  "....X..X\n" // 0x09
  ".....XX.",  // 0x06
  // '@' 0x40 (32)
  "..XXXXX.\n" // 0x3E
  ".X.....X\n" // 0x41
  ".X.XXX.X\n" // 0x5D
  ".X.X.X.X\n" // 0x55
  ".X.XXXX.",  // 0x5E
  // 'A' 0x41 (33)
  ".XXXXX..\n" // 0x7C
  "....X.X.\n" // 0x0A
  "....X..X\n" // 0x09
  "....X.X.\n" // 0x0A
  ".XXXXX..",  // 0x7C
  // 'B' 0x42 (34)
  ".XXXXXXX\n" // 0x7F
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX.XX.",  // 0x36
  // 'C' 0x43 (35)
  "..XXXXX.\n" // 0x3E
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  "..X...X.",  // 0x22
  // 'D' 0x44 (36)
  ".XXXXXXX\n" // 0x7F
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  "..X...X.\n" // 0x22
  "...XXX..",  // 0x1C
  // 'E' 0x45 (37)
  ".XXXXXXX\n" // 0x7F
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X.....X\n" // 0x41
  ".X.....X",  // 0x41
  // 'F' 0x46 (38)
  ".XXXXXXX\n" // 0x7F
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  ".......X\n" // 0x01
  ".......X",  // 0x01
  // 'G' 0x47 (39)
  "..XXXXX.\n" // 0x3E
  ".X.....X\n" // 0x41
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".XXXX.X.",  // 0x7A
  // 'H' 0x48 (40)
  ".XXXXXXX\n" // 0x7F
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  ".XXXXXXX",  // 0x7F
  // 'I' 0x49 (41)
  ".X.....X\n" // 0x41
  ".XXXXXXX\n" // 0x7F
  ".X.....X",  // 0x41
  // 'J' 0x4A (42)
  "..XX....\n" // 0x30
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  "..XXXXXX",  // 0x3F
  // 'K' 0x4B (43)
  ".XXXXXXX\n" // 0x7F
  "....X...\n" // 0x08
  "....XX..\n" // 0x0C
  "...X..X.\n" // 0x12
  ".XX....X",  // 0x61
  // 'L' 0x4C (44)
  ".XXXXXXX\n" // 0x7F
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......",  // 0x40
  // 'M' 0x4D (45)
  ".XXXXXXX\n" // 0x7F
  "......X.\n" // 0x02
  ".....X..\n" // 0x04
  "....XX..\n" // 0x0C
  ".....X..\n" // 0x04
  "......X.\n" // 0x02
  ".XXXXXXX",  // 0x7F
  // 'N' 0x4E (46)
  ".XXXXXXX\n" // 0x7F
  "......X.\n" // 0x02
  ".....X..\n" // 0x04
  "....X...\n" // 0x08
  ".XXXXXXX",  // 0x7F
  // 'O' 0x4F (47)
  "..XXXXX.\n" // 0x3E
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  "..XXXXX.",  // 0x3E
  // 'P' 0x50 (48)
  ".XXXXXXX\n" // 0x7F
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  ".....XX.",  // 0x06
  // 'Q' 0x51 (49)
  "..XXXXX.\n" // 0x3E
  ".X.....X\n" // 0x41
  ".X.X...X\n" // 0x51
  ".XX....X\n" // 0x61
  ".XXXXXX.",  // 0x7E
  // 'R' 0x52 (50)
  ".XXXXXXX\n" // 0x7F
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  ".XXX.XX.",  // 0x76
  // 'S' 0x53 (51)
  "..X..XX.\n" // 0x26
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  ".X..X..X\n" // 0x49
  "..XX..X.",  // 0x32
  // 'T' 0x54 (52)
  ".......X\n" // 0x01
  ".......X\n" // 0x01
  ".XXXXXXX\n" // 0x7F
  ".......X\n" // 0x01
  ".......X",  // 0x01
  // 'U' 0x55 (53)
  "..XXXXXX\n" // 0x3F
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  "..XXXXXX",  // 0x3F
  // 'V' 0x56 (54)
  "...XXXXX\n" // 0x1F
  "..X.....\n" // 0x20
  ".X......\n" // 0x40
  "..X.....\n" // 0x20
  "...XXXXX",  // 0x1F
  // 'W' 0x57 (55)
  "....XXXX\n" // 0x0F
  "..XX....\n" // 0x30
  ".X......\n" // 0x40
  "..XX....\n" // 0x30
  "....XX..\n" // 0x0C
  "..XX....\n" // 0x30
  ".X......\n" // 0x40
  "..XX....\n" // 0x30
  "....XXXX",  // 0x0F
  // 'X' 0x58 (56)
  ".XX...XX\n" // 0x63
  "...X.X..\n" // 0x14
  "....X...\n" // 0x08
  "...X.X..\n" // 0x14
  ".XX...XX",  // 0x63
  // 'Y' 0x59 (57)
  "......XX\n" // 0x03
  ".....X..\n" // 0x04
  ".XXXX...\n" // 0x78
  ".....X..\n" // 0x04
  "......XX",  // 0x03
  // 'Z' 0x5A (58)
  ".XX....X\n" // 0x61
  ".X.X...X\n" // 0x51
  ".X..X..X\n" // 0x49
  ".X...X.X\n" // 0x45
  ".X....XX",  // 0x43
  // '[' 0x5B (59)
  ".XXXXXXX\n" // 0x7F
  ".X.....X\n" // 0x41
  ".X.....X",  // 0x41
  // '\' 0x5C (60)
  ".....X..\n" // 0x04
  "....X...\n" // 0x08
  "...X....\n" // 0x10
  "..X.....\n" // 0x20
  ".X......",  // 0x40
  // ']' 0x5D (61)
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  ".XXXXXXX",  // 0x7F
  // '^' 0x5E (62)
  ".....X..\n" // 0x04
  "......X.\n" // 0x02
  ".......X\n" // 0x01
  "......X.\n" // 0x02
  ".....X..",  // 0x04
  // '_' 0x5F (63)
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......",  // 0x40
  // '`' 0x60 (64)
  ".......X\n" // 0x01
  "......X.",  // 0x02
  // 'a' 0x61 (65)
  "..X.....\n" // 0x20
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  ".XXXX...",  // 0x78
  // 'b' 0x62 (66)
  ".XXXXXXX\n" // 0x7F
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  "..XXX...",  // 0x38
  // 'c' 0x63 (67)
  "..XXX...\n" // 0x38
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  "....X...",  // 0x08
  // 'd' 0x64 (68)
  "..XXX...\n" // 0x38
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  ".XXXXXXX",  // 0x7F
  // 'e' 0x65 (69)
  "..XXX...\n" // 0x38
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  "...XX...",  // 0x18
  // 'f' 0x66 (70)
  "....X...\n" // 0x08
  ".XXXXXX.\n" // 0x7E
  "....X..X\n" // 0x09
  "....X..X\n" // 0x09
  "......X.",  // 0x02
  // 'g' 0x67 (71)
  ".X..X...\n" // 0x48
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  "..XXX...",  // 0x38
  // 'h' 0x68 (72)
  ".XXXXXXX\n" // 0x7F
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  "....X...\n" // 0x08
  ".XXX....",  // 0x70
  // 'i' 0x69 (73)
  ".X..X...\n" // 0x48
  ".XXXX.X.\n" // 0x7A
  ".X......",  // 0x40
  // 'j' 0x6A (74)
  "..X.....\n" // 0x20
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X..X...\n" // 0x48
  "..XXX.X.",  // 0x3A
  // 'k' 0x6B (75)
  ".XXXXXXX\n" // 0x7F
  "...X....\n" // 0x10
  "..X.X...\n" // 0x28
  ".X...X..",  // 0x44
  // 'l' 0x6C (76)
  "..XXXXXX\n" // 0x3F
  ".X......\n" // 0x40
  ".X......",  // 0x40
  // 'm' 0x6D (77)
  ".XXXXX..\n" // 0x7C
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  "..XXX...\n" // 0x38
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  ".XXXX...",  // 0x78
  // 'n' 0x6E (78)
  ".XXXXX..\n" // 0x7C
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  ".XXXX...",  // 0x78
  // 'o' 0x6F (79)
  "..XXX...\n" // 0x38
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  ".X...X..\n" // 0x44
  "..XXX...",  // 0x38
  // 'p' 0x70 (80)
  ".XXXXX..\n" // 0x7C
  "...X.X..\n" // 0x14
  "...X.X..\n" // 0x14
  "...X.X..\n" // 0x14
  "....X...",  // 0x08
  // 'q' 0x71 (81)
  "....X...\n" // 0x08
  "...X.X..\n" // 0x14
  "...X.X..\n" // 0x14
  ".XXXXX..\n" // 0x7C
  ".X......",  // 0x40
  // 'r' 0x72 (82)
  ".XXXXX..\n" // 0x7C
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  "....X...",  // 0x08
  // 's' 0x73 (83)
  ".X..X...\n" // 0x48
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  ".X.X.X..\n" // 0x54
  "..X..X..",  // 0x24
  // 't' 0x74 (84)
  ".....X..\n" // 0x04
  ".....X..\n" // 0x04
  ".XXXXXXX\n" // 0x7F
  ".X...X..\n" // 0x44
  ".X...X..",  // 0x44
  // 'u' 0x75 (85)
  "..XXXX..\n" // 0x3C
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".XXXXX..",  // 0x7C
  // 'v' 0x76 (86)
  "...XXX..\n" // 0x1C
  "..X.....\n" // 0x20
  ".X......\n" // 0x40
  "..X.....\n" // 0x20
  "...XXX..",  // 0x1C
  // 'w' 0x77 (87)
  ".XXXXX..\n" // 0x7C
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  "..XXX...\n" // 0x38
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".XXXXX..",  // 0x7C
  // 'x' 0x78 (88)
  ".X...X..\n" // 0x44
  "..X.X...\n" // 0x28
  "...X....\n" // 0x10
  "..X.X...\n" // 0x28
  ".X...X..",  // 0x44
  // 'y' 0x79 (89)
  "....XX..\n" // 0x0C
  ".X.X....\n" // 0x50
  ".X.X....\n" // 0x50
  ".X.X....\n" // 0x50
  "..XXXX..",  // 0x3C
  // 'z' 0x7A (90)
  ".X...X..\n" // 0x44
  ".XX..X..\n" // 0x64
  ".X.X.X..\n" // 0x54
  ".X..XX..\n" // 0x4C
  ".X...X..",  // 0x44
  // '{' 0x7B (91)
  "....X...\n" // 0x08
  "..XX.XX.\n" // 0x36
  ".X.....X",  // 0x41
  // '|' 0x7C (92)
  ".XXXXXXX",  // 0x7F
  // '}' 0x7D (93)
  ".X.....X\n" // 0x41
  "..XX.XX.\n" // 0x36
  "....X...",  // 0x08
  // '~' 0x7E (94)
  ".....X..\n" // 0x04
  "......X.\n" // 0x02
  ".....X..\n" // 0x04
  "....X...\n" // 0x08
  ".....X..",  // 0x04
  // '' 0x7F (95)
  ".XXXXXXX\n" // 0x7F
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  ".X.....X\n" // 0x41
  ".XXXXXXX",  // 0x7F
  // '\200' 0x80 (96)
  ".XXXXX.X\n" // 0x7D
  "....X.X.\n" // 0x0A
  "....X..X\n" // 0x09
  "....X.X.\n" // 0x0A
  ".XXXXX.X",  // 0x7D
  // '\201' 0x81 (97)
  "..XXXX.X\n" // 0x3D
  ".X....X.\n" // 0x42
  ".X....X.\n" // 0x42
  ".X....X.\n" // 0x42
  "..XXXX.X",  // 0x3D
  // '\202' 0x82 (98)
  "..XXXX.X\n" // 0x3D
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  ".X......\n" // 0x40
  "..XXXX.X",  // 0x3D
  // '\203' 0x83 (99)
  "..X.....\n" // 0x20
  ".X.X.X.X\n" // 0x55
  ".X.X.X..\n" // 0x54
  ".X.X.X.X\n" // 0x55
  ".XXXX...",  // 0x78
  // '\204' 0x84 (100)
  "..XXX...\n" // 0x38
  ".X...X.X\n" // 0x45
  ".X...X..\n" // 0x44
  ".X...X.X\n" // 0x45
  "..XXX...",  // 0x38
  // '\205' 0x85 (101)
  "..XXXX..\n" // 0x3C
  ".X.....X\n" // 0x41
  ".X......\n" // 0x40
  ".X.....X\n" // 0x41
  ".XXXXX..",  // 0x7C
  // 'è' 0x86 (102)
  "..XXX...\n" // 0x38
  ".X.X.X.X\n" // 0x55
  ".X.X.XX.\n" // 0x56
  ".X.X.X..\n" // 0x54
  "...XX...",  // 0x18
  // 'é' 0x87 (103)
  "..XXX...\n" // 0x38
  ".X.X.X..\n" // 0x54
  ".X.X.XX.\n" // 0x56
  ".X.X.X.X\n" // 0x55
  "...XX...",  // 0x18
  // 'ê' 0x88 (104)
  "..XXX...\n" // 0x38
  ".X.X.XX.\n" // 0x56
  ".X.X.X.X\n" // 0x55
  ".X.X.XX.\n" // 0x56
  "...XX...",  // 0x18
  // 'ë' 0x89 (105)
  "..XXX...\n" // 0x38
  ".X.X.X.X\n" // 0x55
  ".X.X.X..\n" // 0x54
  ".X.X.X.X\n" // 0x55
  "...XX...",  // 0x18
  // 'à' 0x8A (106)
  "..X.....\n" // 0x20
  ".X.X.X.X\n" // 0x55
  ".X.X.XX.\n" // 0x56
  ".X.X.X..\n" // 0x54
  ".XXXX...",  // 0x78
  // 'á' 0x8B (107)
  "..X.....\n" // 0x20
  ".X.X.X..\n" // 0x54
  ".X.X.XX.\n" // 0x56
  ".X.X.X.X\n" // 0x55
  ".XXXX...",  // 0x78
  // 'â' 0x8C (108)
  "..X.....\n" // 0x20
  ".X.X.XX.\n" // 0x56
  ".X.X.X.X\n" // 0x55
  ".X.X.XX.\n" // 0x56
  ".XXXX...",  // 0x78
  // 'ì' 0x8D (109)
  ".X..X.X.\n" // 0x4A
  ".XXXX...\n" // 0x78
  ".X....X.",  // 0x42
  // 'í' 0x8E (110)
  ".X..X.X.\n" // 0x4A
  ".XXXX...\n" // 0x78
  ".X....X.",  // 0x42
  // 'î' 0x8F (111)
  ".X..X.X.\n" // 0x4A
  ".XXXX..X\n" // 0x79
  ".X....X.",  // 0x42
  // 'ï' 0x90 (112)
  ".X..X.X.\n" // 0x4A
  ".XXXX...\n" // 0x78
  ".X....X.",  // 0x42
  // 'ç' 0x91 (113)
  "..XXX...\n" // 0x38
  ".X...X..\n" // 0x44
  "XXX..X..\n" // 0x64
  ".X...X..\n" // 0x44
  "....X...",  // 0x08
};




static void fontAsBinString()
{
  printf("\n\nstatic const char * fontBin[numGlyphs] = {\n");
  for (int g=0; g<numGlyphs; ++g) {
    const glyph_t &glyph = fontGlyphs[g];
    printf("  // '%c' 0x%02X (%d)\n", g+0x20, g+0x20, g);
    for (int i=0; i<glyph.width; i++) {
      char col = glyph.cols[i];
      string colstr;
      for (int bit=7; bit>=0; --bit) {
        colstr += col & (1<<bit) ? "X" : ".";
      }
      printf("  \"%s%s\"%s // 0x%02X\n", colstr.c_str(), i==glyph.width-1 ? "" : "\\n", i==glyph.width-1 ? ", " : "", col);
    }
  }
  printf("};\n\n");
}


static void binStringAsFont()
{
  printf("\n\nstatic const glyph_t fontGlyphs[numGlyphs] = {\n");
  for (int g=0; g<numGlyphs; ++g) {
    const char *bs = fontBin[g];
    int w = 0;
    string chr;
    while (*bs) {
      w++;
      uint8_t by = 0;
      while (*bs && *bs!='\n') {
        by = (by<<1) | (*bs!='.' && *bs!=' ' ? 0x01 : 0x00);
        bs++;
      }
      string_format_append(chr, "\\x%02x", by);
      if (*bs=='\n') bs++;
    }
    printf("  { %2d, \"%s\" },  // '%c' 0x%02X (%d)\n", w, chr.c_str(), g+0x20, g+0x20, g);
  }
  printf("};\n\n");
}

#endif // TEXT_FONT_GEN


// MARK: ===== TextView


TextView::TextView()
{
  #if TEXT_FONT_GEN
  fontAsBinString();
  binStringAsFont();
  exit(1);
  #endif
  mTextSpacing = 2;
  mVisible = true;
}


void TextView::clear()
{
  stopAnimations();
  setText("");
  mVisible = true;
}


TextView::~TextView()
{
}


void TextView::setText(const string aText)
{
  mText = aText;
  renderText();
}


void TextView::setVisible(bool aVisible)
{
  mVisible = aVisible;
  renderText();
}



void TextView::renderText()
{
  mTextPixelCols.clear();
  if (mVisible) {
    // convert to glyph indices
    string glyphs;
    size_t i = 0;
    while (i<mText.size()) {
      uint8_t textbyte = mText[i++];
      unsigned char c = 0x7F; // placeholder for unknown
      // Ä = C3 84
      // Ö = C3 96
      // Ü = C3 9C
      // ä = C3 A4
      // ö = C3 B6
      // ü = C3 BC
      if (textbyte==0xC3) {
        if (i>=mText.size()) break; // end of text
        switch ((uint8_t)mText[i++]) {
          case 0x84: c = 0x80; break; // Ä
          case 0x96: c = 0x81; break; // Ö
          case 0x9C: c = 0x82; break; // Ü
          case 0xA4: c = 0x83; break; // ä
          case 0xB6: c = 0x84; break; // ö
          case 0xBC: c = 0x85; break; // ü
          case 0xA8: c = 0x86; break; // è
          case 0xA9: c = 0x87; break; // é
          case 0xAA: c = 0x88; break; // ê
          case 0xAB: c = 0x89; break; // ë
          case 0xA0: c = 0x8A; break; // à
          case 0xA1: c = 0x8B; break; // á
          case 0xA2: c = 0x8C; break; // â
          case 0xAC: c = 0x8D; break; // ì
          case 0xAD: c = 0x8E; break; // í
          case 0xAE: c = 0x8F; break; // î
          case 0xAF: c = 0x90; break; // ï
          case 0xA7: c = 0x91; break; // ç
        }
      }
      else {
        c = textbyte;
      }
      // convert to glyph number
      if (c<0x20 || c>=0x20+numGlyphs) {
        c = 0x7F; // placeholder for unknown
      }
      c -= 0x20;
      glyphs += c;
    }
    // now render glyphs
    for (size_t i = 0; i<glyphs.size(); ++i) {
      const glyph_t &g = fontGlyphs[(unsigned int)glyphs[i]];
      for (int j = 0; j<g.width; ++j) {
        mTextPixelCols.append(1, g.cols[j]);
      }
      for (int j = 0; j<mTextSpacing; ++j) {
        mTextPixelCols.append(1, 0);
      }
    }
  }
  // set content size
  setContentSize({(int)mTextPixelCols.size(), rowsPerGlyph});
  makeDirty();
}


void TextView::recalculateColoring()
{
  calculateGradient(mContent.dx, mExtent.x);
  inherited::recalculateColoring();
}


void TextView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  // coloring is dependent on geometry
  recalculateColoring();
  inherited::geometryChanged(aOldFrame, aOldContent);
}


PixelColor TextView::contentColorAt(PixelPoint aPt)
{
  if (isInContentSize(aPt)) {
    uint8_t col = mTextPixelCols[aPt.x];
    if (aPt.y<rowsPerGlyph && aPt.y>=0 && (col & (1<<(rowsPerGlyph-1-aPt.y)))) {
      if (!mGradientPixels.empty()) {
        // horizontally gradiated text
        return gradientPixel(aPt.x);
      }
      return mForegroundColor;
    }
  }
  return mBackgroundColor;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr TextView::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    if (aViewConfig->get("text", o)) {
      setText(o->stringValue());
    }
    if (aViewConfig->get("visible", o)) {
      setVisible(o->boolValue());
    }
    if (aViewConfig->get("spacing", o)) {
      setTextSpacing(o->int32Value());
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS

JsonObjectPtr TextView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("text", JsonObject::newString(mText));
  status->add("visible", JsonObject::newBool(mVisible));
  status->add("spacing", JsonObject::newInt32(mTextSpacing));
  return status;
}

#endif // ENABLE_VIEWSTATUS
