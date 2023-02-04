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

const int rowsPerGlyph = 7;

#define TEXT_FONT_GEN 0
#if TEXT_FONT_GEN

static const char * fontBin[] = {

    " " /* 0x20 - Glyph 0
     76543210 */ "\n"

    "........"   "\n" // 0x00
    "........"   "\n" // 0x00
    "........"   "\n" // 0x00
    "........"   "\n" // 0x00
    "........"   ,    // 0x00

    "!" // UTF8: 0x21
  /* 84218421 */ "\n" // #1
    ".X.XXXXX"   ,    // 0x5F

    "\"" // UTF8: 0x22
  /* 84218421 */ "\n" // #2
    "......XX"   "\n" // 0x03
    "........"   "\n" // 0x00
    "......XX"   ,    // 0x03

    "#" // UTF8: 0x23
  /* 84218421 */ "\n" // #3
    "..X.X..."   "\n" // 0x28
    ".XXXXX.."   "\n" // 0x7C
    "..X.X..."   "\n" // 0x28
    ".XXXXX.."   "\n" // 0x7C
    "..X.X..."   ,    // 0x28

  "$" /* 0x24 - Glyph 4 */

  "\n"   "..X..X.."   // 0x24
  "\n"   "..X.X.X."   // 0x2A
  "\n"   ".XXXXXXX"   // 0x7F
  "\n"   "..X.X.X."   // 0x2A
  "\n"   "...X..X." , // 0x12

    "%" // UTF8: 0x25
  /* 84218421 */ "\n" // #5
    ".X..XX.."   "\n" // 0x4C
    "..X.XX.."   "\n" // 0x2C
    "...X...."   "\n" // 0x10
    ".XX.X..."   "\n" // 0x68
    ".XX..X.."   ,    // 0x64

    "&" // UTF8: 0x26
  /* 84218421 */ "\n" // #6
    "..XX...."   "\n" // 0x30
    ".X..XXX."   "\n" // 0x4E
    ".X.X.X.X"   "\n" // 0x55
    "..X...X."   "\n" // 0x22
    ".X......"   ,    // 0x40

    "'" // UTF8: 0x27
  /* 84218421 */ "\n" // #7
    "......XX"   ,    // 0x03

    "(" // UTF8: 0x28
  /* 84218421 */ "\n" // #8
    "...XXX.."   "\n" // 0x1C
    "..X...X."   "\n" // 0x22
    ".X.....X"   ,    // 0x41

    ")" // UTF8: 0x29
  /* 84218421 */ "\n" // #9
    ".X.....X"   "\n" // 0x41
    "..X...X."   "\n" // 0x22
    "...XXX.."   ,    // 0x1C

    "*" // UTF8: 0x2A
  /* 84218421 */ "\n" // #10
    "...X.X.."   "\n" // 0x14
    "....X..."   "\n" // 0x08
    "..XXXXX."   "\n" // 0x3E
    "....X..."   "\n" // 0x08
    "...X.X.."   ,    // 0x14

    "+" // UTF8: 0x2B
  /* 84218421 */ "\n" // #11
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "..XXXXX."   "\n" // 0x3E
    "....X..."   "\n" // 0x08
    "....X..."   ,    // 0x08

    "," // UTF8: 0x2C
  /* 84218421 */ "\n" // #12
    ".X.X...."   "\n" // 0x50
    "..XX...."   ,    // 0x30

    "-" // UTF8: 0x2D
  /* 84218421 */ "\n" // #13
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "....X..."   ,    // 0x08

    "." // UTF8: 0x2E
  /* 84218421 */ "\n" // #14
    ".XX....."   "\n" // 0x60
    ".XX....."   ,    // 0x60

    "/" // UTF8: 0x2F
  /* 84218421 */ "\n" // #15
    ".X......"   "\n" // 0x40
    "..X....."   "\n" // 0x20
    "...X...."   "\n" // 0x10
    "....X..."   "\n" // 0x08
    ".....X.."   "\n" // 0x04
    "......X."   ,    // 0x02

    "0" // UTF8: 0x30
  /* 84218421 */ "\n" // #16
    "..XXXXX."   "\n" // 0x3E
    ".X.X...X"   "\n" // 0x51
    ".X..X..X"   "\n" // 0x49
    ".X...X.X"   "\n" // 0x45
    "..XXXXX."   ,    // 0x3E

    "1" // UTF8: 0x31
  /* 84218421 */ "\n" // #17
    ".X....X."   "\n" // 0x42
    ".XXXXXXX"   "\n" // 0x7F
    ".X......"   ,    // 0x40

    "2" // UTF8: 0x32
  /* 84218421 */ "\n" // #18
    ".XX...X."   "\n" // 0x62
    ".X.X...X"   "\n" // 0x51
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X...XX."   ,    // 0x46

    "3" // UTF8: 0x33
  /* 84218421 */ "\n" // #19
    "..X...X."   "\n" // 0x22
    ".X.....X"   "\n" // 0x41
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX.XX."   ,    // 0x36

    "4" // UTF8: 0x34
  /* 84218421 */ "\n" // #20
    "...XXX.."   "\n" // 0x1C
    "...X..X."   "\n" // 0x12
    "...X...X"   "\n" // 0x11
    ".XXXXXXX"   "\n" // 0x7F
    "...X...."   ,    // 0x10

    "5" // UTF8: 0x35
  /* 84218421 */ "\n" // #21
    ".X..XXXX"   "\n" // 0x4F
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX...X"   ,    // 0x31

    "6" // UTF8: 0x36
  /* 84218421 */ "\n" // #22
    "..XXXXX."   "\n" // 0x3E
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX..X."   ,    // 0x32

    "7" // UTF8: 0x37
  /* 84218421 */ "\n" // #23
    "......XX"   "\n" // 0x03
    ".......X"   "\n" // 0x01
    ".XXX...X"   "\n" // 0x71
    "....X..X"   "\n" // 0x09
    ".....XXX"   ,    // 0x07

    "8" // UTF8: 0x38
  /* 84218421 */ "\n" // #24
    "..XX.XX."   "\n" // 0x36
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX.XX."   ,    // 0x36

    "9" // UTF8: 0x39
  /* 84218421 */ "\n" // #25
    "..X..XX."   "\n" // 0x26
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XXXXX."   ,    // 0x3E

    ":" // UTF8: 0x3A
  /* 84218421 */ "\n" // #26
    ".XX..XX."   "\n" // 0x66
    ".XX..XX."   ,    // 0x66

    ";" // UTF8: 0x3B
  /* 84218421 */ "\n" // #27
    ".X.X.XX."   "\n" // 0x56
    "..XX.XX."   ,    // 0x36

    "<" // UTF8: 0x3C
  /* 84218421 */ "\n" // #28
    "....X..."   "\n" // 0x08
    "...X.X.."   "\n" // 0x14
    "..X...X."   "\n" // 0x22
    ".X.....X"   ,    // 0x41

    "=" // UTF8: 0x3D
  /* 84218421 */ "\n" // #29
    "...X.X.."   "\n" // 0x14
    "...X.X.."   "\n" // 0x14
    "...X.X.."   "\n" // 0x14
    "...X.X.."   ,    // 0x14

    ">" // UTF8: 0x3E
  /* 84218421 */ "\n" // #30
    ".X.....X"   "\n" // 0x41
    "..X...X."   "\n" // 0x22
    "...X.X.."   "\n" // 0x14
    "....X..."   ,    // 0x08

    "?" // UTF8: 0x3F
  /* 84218421 */ "\n" // #31
    "......X."   "\n" // 0x02
    ".......X"   "\n" // 0x01
    ".X.XX..X"   "\n" // 0x59
    "....X..X"   "\n" // 0x09
    ".....XX."   ,    // 0x06

    "@" // UTF8: 0x40
  /* 84218421 */ "\n" // #32
    "..XXXXX."   "\n" // 0x3E
    ".X.....X"   "\n" // 0x41
    ".X.XXX.X"   "\n" // 0x5D
    ".X.X.X.X"   "\n" // 0x55
    ".X.XXXX."   ,    // 0x5E

    "A" // UTF8: 0x41
  /* 84218421 */ "\n" // #33
    ".XXXXX.."   "\n" // 0x7C
    "....X.X."   "\n" // 0x0A
    "....X..X"   "\n" // 0x09
    "....X.X."   "\n" // 0x0A
    ".XXXXX.."   ,    // 0x7C

    "B" // UTF8: 0x42
  /* 84218421 */ "\n" // #34
    ".XXXXXXX"   "\n" // 0x7F
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX.XX."   ,    // 0x36

    "C" // UTF8: 0x43
  /* 84218421 */ "\n" // #35
    "..XXXXX."   "\n" // 0x3E
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    "..X...X."   ,    // 0x22

    "D" // UTF8: 0x44
  /* 84218421 */ "\n" // #36
    ".XXXXXXX"   "\n" // 0x7F
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    "..X...X."   "\n" // 0x22
    "...XXX.."   ,    // 0x1C

    "E" // UTF8: 0x45
  /* 84218421 */ "\n" // #37
    ".XXXXXXX"   "\n" // 0x7F
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X.....X"   "\n" // 0x41
    ".X.....X"   ,    // 0x41

    "F" // UTF8: 0x46
  /* 84218421 */ "\n" // #38
    ".XXXXXXX"   "\n" // 0x7F
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    ".......X"   "\n" // 0x01
    ".......X"   ,    // 0x01

    "G" // UTF8: 0x47
  /* 84218421 */ "\n" // #39
    "..XXXXX."   "\n" // 0x3E
    ".X.....X"   "\n" // 0x41
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".XXXX.X."   ,    // 0x7A

    "H" // UTF8: 0x48
  /* 84218421 */ "\n" // #40
    ".XXXXXXX"   "\n" // 0x7F
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    ".XXXXXXX"   ,    // 0x7F

    "I" // UTF8: 0x49
  /* 84218421 */ "\n" // #41
    ".X.....X"   "\n" // 0x41
    ".XXXXXXX"   "\n" // 0x7F
    ".X.....X"   ,    // 0x41

    "J" // UTF8: 0x4A
  /* 84218421 */ "\n" // #42
    "..XX...."   "\n" // 0x30
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    "..XXXXXX"   ,    // 0x3F

    "K" // UTF8: 0x4B
  /* 84218421 */ "\n" // #43
    ".XXXXXXX"   "\n" // 0x7F
    "....X..."   "\n" // 0x08
    "....XX.."   "\n" // 0x0C
    "...X..X."   "\n" // 0x12
    ".XX....X"   ,    // 0x61

    "L" // UTF8: 0x4C
  /* 84218421 */ "\n" // #44
    ".XXXXXXX"   "\n" // 0x7F
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   ,    // 0x40

    "M" // UTF8: 0x4D
  /* 84218421 */ "\n" // #45
    ".XXXXXXX"   "\n" // 0x7F
    "......X."   "\n" // 0x02
    ".....X.."   "\n" // 0x04
    "....XX.."   "\n" // 0x0C
    ".....X.."   "\n" // 0x04
    "......X."   "\n" // 0x02
    ".XXXXXXX"   ,    // 0x7F

    "N" // UTF8: 0x4E
  /* 84218421 */ "\n" // #46
    ".XXXXXXX"   "\n" // 0x7F
    "......X."   "\n" // 0x02
    ".....X.."   "\n" // 0x04
    "....X..."   "\n" // 0x08
    ".XXXXXXX"   ,    // 0x7F

    "O" // UTF8: 0x4F
  /* 84218421 */ "\n" // #47
    "..XXXXX."   "\n" // 0x3E
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    "..XXXXX."   ,    // 0x3E

    "P" // UTF8: 0x50
  /* 84218421 */ "\n" // #48
    ".XXXXXXX"   "\n" // 0x7F
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    ".....XX."   ,    // 0x06

    "Q" // UTF8: 0x51
  /* 84218421 */ "\n" // #49
    "..XXXXX."   "\n" // 0x3E
    ".X.....X"   "\n" // 0x41
    ".X.X...X"   "\n" // 0x51
    ".XX....X"   "\n" // 0x61
    ".XXXXXX."   ,    // 0x7E

    "R" // UTF8: 0x52
  /* 84218421 */ "\n" // #50
    ".XXXXXXX"   "\n" // 0x7F
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    ".XXX.XX."   ,    // 0x76

    "S" // UTF8: 0x53
  /* 84218421 */ "\n" // #51
    "..X..XX."   "\n" // 0x26
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    ".X..X..X"   "\n" // 0x49
    "..XX..X."   ,    // 0x32

    "T" // UTF8: 0x54
  /* 84218421 */ "\n" // #52
    ".......X"   "\n" // 0x01
    ".......X"   "\n" // 0x01
    ".XXXXXXX"   "\n" // 0x7F
    ".......X"   "\n" // 0x01
    ".......X"   ,    // 0x01

    "U" // UTF8: 0x55
  /* 84218421 */ "\n" // #53
    "..XXXXXX"   "\n" // 0x3F
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    "..XXXXXX"   ,    // 0x3F

    "V" // UTF8: 0x56
  /* 84218421 */ "\n" // #54
    "...XXXXX"   "\n" // 0x1F
    "..X....."   "\n" // 0x20
    ".X......"   "\n" // 0x40
    "..X....."   "\n" // 0x20
    "...XXXXX"   ,    // 0x1F

    "W" // UTF8: 0x57
  /* 84218421 */ "\n" // #55
    "....XXXX"   "\n" // 0x0F
    "..XX...."   "\n" // 0x30
    ".X......"   "\n" // 0x40
    "..XX...."   "\n" // 0x30
    "....XX.."   "\n" // 0x0C
    "..XX...."   "\n" // 0x30
    ".X......"   "\n" // 0x40
    "..XX...."   "\n" // 0x30
    "....XXXX"   ,    // 0x0F

    "X" // UTF8: 0x58
  /* 84218421 */ "\n" // #56
    ".XX...XX"   "\n" // 0x63
    "...X.X.."   "\n" // 0x14
    "....X..."   "\n" // 0x08
    "...X.X.."   "\n" // 0x14
    ".XX...XX"   ,    // 0x63

    "Y" // UTF8: 0x59
  /* 84218421 */ "\n" // #57
    "......XX"   "\n" // 0x03
    ".....X.."   "\n" // 0x04
    ".XXXX..."   "\n" // 0x78
    ".....X.."   "\n" // 0x04
    "......XX"   ,    // 0x03

    "Z" // UTF8: 0x5A
  /* 84218421 */ "\n" // #58
    ".XX....X"   "\n" // 0x61
    ".X.X...X"   "\n" // 0x51
    ".X..X..X"   "\n" // 0x49
    ".X...X.X"   "\n" // 0x45
    ".X....XX"   ,    // 0x43

    "[" // UTF8: 0x5B
  /* 84218421 */ "\n" // #59
    ".XXXXXXX"   "\n" // 0x7F
    ".X.....X"   "\n" // 0x41
    ".X.....X"   ,    // 0x41

    "\\" // UTF8: 0x5C
  /* 84218421 */ "\n" // #60
    ".....X.."   "\n" // 0x04
    "....X..."   "\n" // 0x08
    "...X...."   "\n" // 0x10
    "..X....."   "\n" // 0x20
    ".X......"   ,    // 0x40

    "]" // UTF8: 0x5D
  /* 84218421 */ "\n" // #61
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    ".XXXXXXX"   ,    // 0x7F

    "^" // UTF8: 0x5E
  /* 84218421 */ "\n" // #62
    ".....X.."   "\n" // 0x04
    "......X."   "\n" // 0x02
    ".......X"   "\n" // 0x01
    "......X."   "\n" // 0x02
    ".....X.."   ,    // 0x04

    "_" // UTF8: 0x5F
  /* 84218421 */ "\n" // #63
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   ,    // 0x40

    "`" // UTF8: 0x60
  /* 84218421 */ "\n" // #64
    ".......X"   "\n" // 0x01
    "......X."   ,    // 0x02

    "a" // UTF8: 0x61
  /* 84218421 */ "\n" // #65
    "..X....."   "\n" // 0x20
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    ".XXXX..."   ,    // 0x78

    "b" // UTF8: 0x62
  /* 84218421 */ "\n" // #66
    ".XXXXXXX"   "\n" // 0x7F
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    "..XXX..."   ,    // 0x38

    "c" // UTF8: 0x63
  /* 84218421 */ "\n" // #67
    "..XXX..."   "\n" // 0x38
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    "....X..."   ,    // 0x08

    "d" // UTF8: 0x64
  /* 84218421 */ "\n" // #68
    "..XXX..."   "\n" // 0x38
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    ".XXXXXXX"   ,    // 0x7F

    "e" // UTF8: 0x65
  /* 84218421 */ "\n" // #69
    "..XXX..."   "\n" // 0x38
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    "...XX..."   ,    // 0x18

    "f" // UTF8: 0x66
  /* 84218421 */ "\n" // #70
    "....X..."   "\n" // 0x08
    ".XXXXXX."   "\n" // 0x7E
    "....X..X"   "\n" // 0x09
    "....X..X"   "\n" // 0x09
    "......X."   ,    // 0x02

    "g" // UTF8: 0x67
  /* 84218421 */ "\n" // #71
    ".X..X..."   "\n" // 0x48
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    "..XXX..."   ,    // 0x38

    "h" // UTF8: 0x68
  /* 84218421 */ "\n" // #72
    ".XXXXXXX"   "\n" // 0x7F
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    "....X..."   "\n" // 0x08
    ".XXX...."   ,    // 0x70

    "i" // UTF8: 0x69
  /* 84218421 */ "\n" // #73
    ".X..X..."   "\n" // 0x48
    ".XXXX.X."   "\n" // 0x7A
    ".X......"   ,    // 0x40

    "j" // UTF8: 0x6A
  /* 84218421 */ "\n" // #74
    "..X....."   "\n" // 0x20
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X..X..."   "\n" // 0x48
    "..XXX.X."   ,    // 0x3A

    "k" // UTF8: 0x6B
  /* 84218421 */ "\n" // #75
    ".XXXXXXX"   "\n" // 0x7F
    "...X...."   "\n" // 0x10
    "..X.X..."   "\n" // 0x28
    ".X...X.."   ,    // 0x44

    "l" // UTF8: 0x6C
  /* 84218421 */ "\n" // #76
    "..XXXXXX"   "\n" // 0x3F
    ".X......"   "\n" // 0x40
    ".X......"   ,    // 0x40

    "m" // UTF8: 0x6D
  /* 84218421 */ "\n" // #77
    ".XXXXX.."   "\n" // 0x7C
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    "..XXX..."   "\n" // 0x38
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    ".XXXX..."   ,    // 0x78

    "n" // UTF8: 0x6E
  /* 84218421 */ "\n" // #78
    ".XXXXX.."   "\n" // 0x7C
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    ".XXXX..."   ,    // 0x78

    "o" // UTF8: 0x6F
  /* 84218421 */ "\n" // #79
    "..XXX..."   "\n" // 0x38
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    ".X...X.."   "\n" // 0x44
    "..XXX..."   ,    // 0x38

    "p" // UTF8: 0x70
  /* 84218421 */ "\n" // #80
    ".XXXXX.."   "\n" // 0x7C
    "...X.X.."   "\n" // 0x14
    "...X.X.."   "\n" // 0x14
    "...X.X.."   "\n" // 0x14
    "....X..."   ,    // 0x08

    "q" // UTF8: 0x71
  /* 84218421 */ "\n" // #81
    "....X..."   "\n" // 0x08
    "...X.X.."   "\n" // 0x14
    "...X.X.."   "\n" // 0x14
    ".XXXXX.."   "\n" // 0x7C
    ".X......"   ,    // 0x40

    "r" // UTF8: 0x72
  /* 84218421 */ "\n" // #82
    ".XXXXX.."   "\n" // 0x7C
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    "....X..."   ,    // 0x08

    "s" // UTF8: 0x73
  /* 84218421 */ "\n" // #83
    ".X..X..."   "\n" // 0x48
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.."   "\n" // 0x54
    "..X..X.."   ,    // 0x24

    "t" // UTF8: 0x74
  /* 84218421 */ "\n" // #84
    ".....X.."   "\n" // 0x04
    ".....X.."   "\n" // 0x04
    ".XXXXXXX"   "\n" // 0x7F
    ".X...X.."   "\n" // 0x44
    ".X...X.."   ,    // 0x44

    "u" // UTF8: 0x75
  /* 84218421 */ "\n" // #85
    "..XXXX.."   "\n" // 0x3C
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".XXXXX.."   ,    // 0x7C

    "v" // UTF8: 0x76
  /* 84218421 */ "\n" // #86
    "...XXX.."   "\n" // 0x1C
    "..X....."   "\n" // 0x20
    ".X......"   "\n" // 0x40
    "..X....."   "\n" // 0x20
    "...XXX.."   ,    // 0x1C

    "w" // UTF8: 0x77
  /* 84218421 */ "\n" // #87
    ".XXXXX.."   "\n" // 0x7C
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    "..XXX..."   "\n" // 0x38
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".XXXXX.."   ,    // 0x7C

    "x" // UTF8: 0x78
  /* 84218421 */ "\n" // #88
    ".X...X.."   "\n" // 0x44
    "..X.X..."   "\n" // 0x28
    "...X...."   "\n" // 0x10
    "..X.X..."   "\n" // 0x28
    ".X...X.."   ,    // 0x44

    "y" // UTF8: 0x79
  /* 84218421 */ "\n" // #89
    "....XX.."   "\n" // 0x0C
    ".X.X...."   "\n" // 0x50
    ".X.X...."   "\n" // 0x50
    ".X.X...."   "\n" // 0x50
    "..XXXX.."   ,    // 0x3C

    "z" // UTF8: 0x7A
  /* 84218421 */ "\n" // #90
    ".X...X.."   "\n" // 0x44
    ".XX..X.."   "\n" // 0x64
    ".X.X.X.."   "\n" // 0x54
    ".X..XX.."   "\n" // 0x4C
    ".X...X.."   ,    // 0x44

    "{" // UTF8: 0x7B
  /* 84218421 */ "\n" // #91
    "....X..."   "\n" // 0x08
    "..XX.XX."   "\n" // 0x36
    ".X.....X"   ,    // 0x41

    "|" // UTF8: 0x7C
  /* 84218421 */ "\n" // #92
    ".XXXXXXX"   ,    // 0x7F

    "}" // UTF8: 0x7D
  /* 84218421 */ "\n" // #93
    ".X.....X"   "\n" // 0x41
    "..XX.XX."   "\n" // 0x36
    "....X..."   ,    // 0x08

    "~" // UTF8: 0x7E
  /* 84218421 */ "\n" // #94
    ".....X.."   "\n" // 0x04
    "......X."   "\n" // 0x02
    ".....X.."   "\n" // 0x04
    "....X..."   "\n" // 0x08
    ".....X.."   ,    // 0x04

    "placeholder" // UTF8: 0x7F
  /* 84218421 */ "\n" // #95
    ".XXXXXXX"   "\n" // 0x7F
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    ".X.....X"   "\n" // 0x41
    ".XXXXXXX"   ,    // 0x7F

    "Ä" // UTF8: 0x80
  /* 84218421 */ "\n" // #96
    ".XXXXX.X"   "\n" // 0x7D
    "....X.X."   "\n" // 0x0A
    "....X..X"   "\n" // 0x09
    "....X.X."   "\n" // 0x0A
    ".XXXXX.X"   ,    // 0x7D

    "Ö" // UTF8: 0x81
  /* 84218421 */ "\n" // #97
    "..XXXX.X"   "\n" // 0x3D
    ".X....X."   "\n" // 0x42
    ".X....X."   "\n" // 0x42
    ".X....X."   "\n" // 0x42
    "..XXXX.X"   ,    // 0x3D

    "Ü" // UTF8: 0x82
  /* 84218421 */ "\n" // #98
    "..XXXX.X"   "\n" // 0x3D
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    ".X......"   "\n" // 0x40
    "..XXXX.X"   ,    // 0x3D

    "ä" // UTF8: 0x83
  /* 84218421 */ "\n" // #99
    "..X....."   "\n" // 0x20
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.X"   "\n" // 0x55
    ".XXXX..."   ,    // 0x78

    "ö" // UTF8: 0x84
  /* 84218421 */ "\n" // #100
    "..XXX..."   "\n" // 0x38
    ".X...X.X"   "\n" // 0x45
    ".X...X.."   "\n" // 0x44
    ".X...X.X"   "\n" // 0x45
    "..XXX..."   ,    // 0x38

    "ü" // UTF8: 0x85
  /* 84218421 */ "\n" // #101
    "..XXXX.."   "\n" // 0x3C
    ".X.....X"   "\n" // 0x41
    ".X......"   "\n" // 0x40
    ".X.....X"   "\n" // 0x41
    ".XXXXX.."   ,    // 0x7C

    "è" // UTF8: 0x86
  /* 84218421 */ "\n" // #102
    "..XXX..."   "\n" // 0x38
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.."   "\n" // 0x54
    "...XX..."   ,    // 0x18

    "é" // UTF8: 0x87
  /* 84218421 */ "\n" // #103
    "..XXX..."   "\n" // 0x38
    ".X.X.X.."   "\n" // 0x54
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.X"   "\n" // 0x55
    "...XX..."   ,    // 0x18

    "ê" // UTF8: 0x88
  /* 84218421 */ "\n" // #104
    "..XXX..."   "\n" // 0x38
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.XX."   "\n" // 0x56
    "...XX..."   ,    // 0x18

    "ë" // UTF8: 0x89
  /* 84218421 */ "\n" // #105
    "..XXX..."   "\n" // 0x38
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.X.."   "\n" // 0x54
    ".X.X.X.X"   "\n" // 0x55
    "...XX..."   ,    // 0x18

    "à" // UTF8: 0x8A
  /* 84218421 */ "\n" // #106
    "..X....."   "\n" // 0x20
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.."   "\n" // 0x54
    ".XXXX..."   ,    // 0x78

    "á" // UTF8: 0x8B
  /* 84218421 */ "\n" // #107
    "..X....."   "\n" // 0x20
    ".X.X.X.."   "\n" // 0x54
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.X"   "\n" // 0x55
    ".XXXX..."   ,    // 0x78

    "â" // UTF8: 0x8C
  /* 84218421 */ "\n" // #108
    "..X....."   "\n" // 0x20
    ".X.X.XX."   "\n" // 0x56
    ".X.X.X.X"   "\n" // 0x55
    ".X.X.XX."   "\n" // 0x56
    ".XXXX..."   ,    // 0x78

    "ì" // UTF8: 0x8D
  /* 84218421 */ "\n" // #109
    ".X..X.X."   "\n" // 0x4A
    ".XXXX..."   "\n" // 0x78
    ".X....X."   ,    // 0x42

    "í" // UTF8: 0x8E
  /* 84218421 */ "\n" // #110
    ".X..X.X."   "\n" // 0x4A
    ".XXXX..."   "\n" // 0x78
    ".X....X."   ,    // 0x42

    "î" // UTF8: 0x8F
  /* 84218421 */ "\n" // #111
    ".X..X.X."   "\n" // 0x4A
    ".XXXX..X"   "\n" // 0x79
    ".X....X."   ,    // 0x42

    "ï" // UTF8: 0x90
  /* 84218421 */ "\n" // #112
    ".X..X.X."   "\n" // 0x4A
    ".XXXX..."   "\n" // 0x78
    ".X....X."   ,    // 0x42

    "ç" // UTF8: 0x91
  /* 84218421 */ "\n" // #113
    "..XXX..."   "\n" // 0x38
    ".X...X.."   "\n" // 0x44
    "XXX..X.."   "\n" // 0x64
    ".X...X.."   "\n" // 0x44
    "....X..."   ,    // 0x08

};

#endif // TEXT_FONT_GEN

typedef struct {
  const char* prefix;
  uint8_t first;
  uint8_t last;
  uint8_t glyphOffset;
} GlyphRange;

// MARK: - generated glyph data

const int numGlyphs = 114;

static const glyph_t fontGlyphs[numGlyphs] = {
  {  5, "\x7f\x41\x41\x41\x7f"                     },  // placeholder          (input # 95 -> glyph # 0)
  {  5, "\x00\x00\x00\x00\x00"                     },  // ' ' UTF-8 20         (input # 0 -> glyph # 1)
  {  1, "\x5f"                                     },  // '!' UTF-8 21         (input # 1 -> glyph # 2)
  {  3, "\x03\x00\x03"                             },  // '"' UTF-8 22         (input # 2 -> glyph # 3)
  {  5, "\x28\x7c\x28\x7c\x28"                     },  // '#' UTF-8 23         (input # 3 -> glyph # 4)
  {  5, "\x24\x2a\x7f\x2a\x12"                     },  // '$' UTF-8 24         (input # 4 -> glyph # 5)
  {  5, "\x4c\x2c\x10\x68\x64"                     },  // '%' UTF-8 25         (input # 5 -> glyph # 6)
  {  5, "\x30\x4e\x55\x22\x40"                     },  // '&' UTF-8 26         (input # 6 -> glyph # 7)
  {  1, "\x03"                                     },  // ''' UTF-8 27         (input # 7 -> glyph # 8)
  {  3, "\x1c\x22\x41"                             },  // '(' UTF-8 28         (input # 8 -> glyph # 9)
  {  3, "\x41\x22\x1c"                             },  // ')' UTF-8 29         (input # 9 -> glyph # 10)
  {  5, "\x14\x08\x3e\x08\x14"                     },  // '*' UTF-8 2A         (input # 10 -> glyph # 11)
  {  5, "\x08\x08\x3e\x08\x08"                     },  // '+' UTF-8 2B         (input # 11 -> glyph # 12)
  {  2, "\x50\x30"                                 },  // ',' UTF-8 2C         (input # 12 -> glyph # 13)
  {  5, "\x08\x08\x08\x08\x08"                     },  // '-' UTF-8 2D         (input # 13 -> glyph # 14)
  {  2, "\x60\x60"                                 },  // '.' UTF-8 2E         (input # 14 -> glyph # 15)
  {  6, "\x40\x20\x10\x08\x04\x02"                 },  // '/' UTF-8 2F         (input # 15 -> glyph # 16)
  {  5, "\x3e\x51\x49\x45\x3e"                     },  // '0' UTF-8 30         (input # 16 -> glyph # 17)
  {  3, "\x42\x7f\x40"                             },  // '1' UTF-8 31         (input # 17 -> glyph # 18)
  {  5, "\x62\x51\x49\x49\x46"                     },  // '2' UTF-8 32         (input # 18 -> glyph # 19)
  {  5, "\x22\x41\x49\x49\x36"                     },  // '3' UTF-8 33         (input # 19 -> glyph # 20)
  {  5, "\x1c\x12\x11\x7f\x10"                     },  // '4' UTF-8 34         (input # 20 -> glyph # 21)
  {  5, "\x4f\x49\x49\x49\x31"                     },  // '5' UTF-8 35         (input # 21 -> glyph # 22)
  {  5, "\x3e\x49\x49\x49\x32"                     },  // '6' UTF-8 36         (input # 22 -> glyph # 23)
  {  5, "\x03\x01\x71\x09\x07"                     },  // '7' UTF-8 37         (input # 23 -> glyph # 24)
  {  5, "\x36\x49\x49\x49\x36"                     },  // '8' UTF-8 38         (input # 24 -> glyph # 25)
  {  5, "\x26\x49\x49\x49\x3e"                     },  // '9' UTF-8 39         (input # 25 -> glyph # 26)
  {  2, "\x66\x66"                                 },  // ':' UTF-8 3A         (input # 26 -> glyph # 27)
  {  2, "\x56\x36"                                 },  // ';' UTF-8 3B         (input # 27 -> glyph # 28)
  {  4, "\x08\x14\x22\x41"                         },  // '<' UTF-8 3C         (input # 28 -> glyph # 29)
  {  4, "\x14\x14\x14\x14"                         },  // '=' UTF-8 3D         (input # 29 -> glyph # 30)
  {  4, "\x41\x22\x14\x08"                         },  // '>' UTF-8 3E         (input # 30 -> glyph # 31)
  {  5, "\x02\x01\x59\x09\x06"                     },  // '?' UTF-8 3F         (input # 31 -> glyph # 32)
  {  5, "\x3e\x41\x5d\x55\x5e"                     },  // '@' UTF-8 40         (input # 32 -> glyph # 33)
  {  5, "\x7c\x0a\x09\x0a\x7c"                     },  // 'A' UTF-8 41         (input # 33 -> glyph # 34)
  {  5, "\x7f\x49\x49\x49\x36"                     },  // 'B' UTF-8 42         (input # 34 -> glyph # 35)
  {  5, "\x3e\x41\x41\x41\x22"                     },  // 'C' UTF-8 43         (input # 35 -> glyph # 36)
  {  5, "\x7f\x41\x41\x22\x1c"                     },  // 'D' UTF-8 44         (input # 36 -> glyph # 37)
  {  5, "\x7f\x49\x49\x41\x41"                     },  // 'E' UTF-8 45         (input # 37 -> glyph # 38)
  {  5, "\x7f\x09\x09\x01\x01"                     },  // 'F' UTF-8 46         (input # 38 -> glyph # 39)
  {  5, "\x3e\x41\x49\x49\x7a"                     },  // 'G' UTF-8 47         (input # 39 -> glyph # 40)
  {  5, "\x7f\x08\x08\x08\x7f"                     },  // 'H' UTF-8 48         (input # 40 -> glyph # 41)
  {  3, "\x41\x7f\x41"                             },  // 'I' UTF-8 49         (input # 41 -> glyph # 42)
  {  5, "\x30\x40\x40\x40\x3f"                     },  // 'J' UTF-8 4A         (input # 42 -> glyph # 43)
  {  5, "\x7f\x08\x0c\x12\x61"                     },  // 'K' UTF-8 4B         (input # 43 -> glyph # 44)
  {  5, "\x7f\x40\x40\x40\x40"                     },  // 'L' UTF-8 4C         (input # 44 -> glyph # 45)
  {  7, "\x7f\x02\x04\x0c\x04\x02\x7f"             },  // 'M' UTF-8 4D         (input # 45 -> glyph # 46)
  {  5, "\x7f\x02\x04\x08\x7f"                     },  // 'N' UTF-8 4E         (input # 46 -> glyph # 47)
  {  5, "\x3e\x41\x41\x41\x3e"                     },  // 'O' UTF-8 4F         (input # 47 -> glyph # 48)
  {  5, "\x7f\x09\x09\x09\x06"                     },  // 'P' UTF-8 50         (input # 48 -> glyph # 49)
  {  5, "\x3e\x41\x51\x61\x7e"                     },  // 'Q' UTF-8 51         (input # 49 -> glyph # 50)
  {  5, "\x7f\x09\x09\x09\x76"                     },  // 'R' UTF-8 52         (input # 50 -> glyph # 51)
  {  5, "\x26\x49\x49\x49\x32"                     },  // 'S' UTF-8 53         (input # 51 -> glyph # 52)
  {  5, "\x01\x01\x7f\x01\x01"                     },  // 'T' UTF-8 54         (input # 52 -> glyph # 53)
  {  5, "\x3f\x40\x40\x40\x3f"                     },  // 'U' UTF-8 55         (input # 53 -> glyph # 54)
  {  5, "\x1f\x20\x40\x20\x1f"                     },  // 'V' UTF-8 56         (input # 54 -> glyph # 55)
  {  9, "\x0f\x30\x40\x30\x0c\x30\x40\x30\x0f"     },  // 'W' UTF-8 57         (input # 55 -> glyph # 56)
  {  5, "\x63\x14\x08\x14\x63"                     },  // 'X' UTF-8 58         (input # 56 -> glyph # 57)
  {  5, "\x03\x04\x78\x04\x03"                     },  // 'Y' UTF-8 59         (input # 57 -> glyph # 58)
  {  5, "\x61\x51\x49\x45\x43"                     },  // 'Z' UTF-8 5A         (input # 58 -> glyph # 59)
  {  3, "\x7f\x41\x41"                             },  // '[' UTF-8 5B         (input # 59 -> glyph # 60)
  {  5, "\x04\x08\x10\x20\x40"                     },  // '\' UTF-8 5C         (input # 60 -> glyph # 61)
  {  3, "\x41\x41\x7f"                             },  // ']' UTF-8 5D         (input # 61 -> glyph # 62)
  {  5, "\x04\x02\x01\x02\x04"                     },  // '^' UTF-8 5E         (input # 62 -> glyph # 63)
  {  5, "\x40\x40\x40\x40\x40"                     },  // '_' UTF-8 5F         (input # 63 -> glyph # 64)
  {  2, "\x01\x02"                                 },  // '`' UTF-8 60         (input # 64 -> glyph # 65)
  {  5, "\x20\x54\x54\x54\x78"                     },  // 'a' UTF-8 61         (input # 65 -> glyph # 66)
  {  5, "\x7f\x44\x44\x44\x38"                     },  // 'b' UTF-8 62         (input # 66 -> glyph # 67)
  {  5, "\x38\x44\x44\x44\x08"                     },  // 'c' UTF-8 63         (input # 67 -> glyph # 68)
  {  5, "\x38\x44\x44\x44\x7f"                     },  // 'd' UTF-8 64         (input # 68 -> glyph # 69)
  {  5, "\x38\x54\x54\x54\x18"                     },  // 'e' UTF-8 65         (input # 69 -> glyph # 70)
  {  5, "\x08\x7e\x09\x09\x02"                     },  // 'f' UTF-8 66         (input # 70 -> glyph # 71)
  {  5, "\x48\x54\x54\x54\x38"                     },  // 'g' UTF-8 67         (input # 71 -> glyph # 72)
  {  5, "\x7f\x08\x08\x08\x70"                     },  // 'h' UTF-8 68         (input # 72 -> glyph # 73)
  {  3, "\x48\x7a\x40"                             },  // 'i' UTF-8 69         (input # 73 -> glyph # 74)
  {  5, "\x20\x40\x40\x48\x3a"                     },  // 'j' UTF-8 6A         (input # 74 -> glyph # 75)
  {  4, "\x7f\x10\x28\x44"                         },  // 'k' UTF-8 6B         (input # 75 -> glyph # 76)
  {  3, "\x3f\x40\x40"                             },  // 'l' UTF-8 6C         (input # 76 -> glyph # 77)
  {  7, "\x7c\x04\x04\x38\x04\x04\x78"             },  // 'm' UTF-8 6D         (input # 77 -> glyph # 78)
  {  5, "\x7c\x04\x04\x04\x78"                     },  // 'n' UTF-8 6E         (input # 78 -> glyph # 79)
  {  5, "\x38\x44\x44\x44\x38"                     },  // 'o' UTF-8 6F         (input # 79 -> glyph # 80)
  {  5, "\x7c\x14\x14\x14\x08"                     },  // 'p' UTF-8 70         (input # 80 -> glyph # 81)
  {  5, "\x08\x14\x14\x7c\x40"                     },  // 'q' UTF-8 71         (input # 81 -> glyph # 82)
  {  5, "\x7c\x04\x04\x04\x08"                     },  // 'r' UTF-8 72         (input # 82 -> glyph # 83)
  {  5, "\x48\x54\x54\x54\x24"                     },  // 's' UTF-8 73         (input # 83 -> glyph # 84)
  {  5, "\x04\x04\x7f\x44\x44"                     },  // 't' UTF-8 74         (input # 84 -> glyph # 85)
  {  5, "\x3c\x40\x40\x40\x7c"                     },  // 'u' UTF-8 75         (input # 85 -> glyph # 86)
  {  5, "\x1c\x20\x40\x20\x1c"                     },  // 'v' UTF-8 76         (input # 86 -> glyph # 87)
  {  7, "\x7c\x40\x40\x38\x40\x40\x7c"             },  // 'w' UTF-8 77         (input # 87 -> glyph # 88)
  {  5, "\x44\x28\x10\x28\x44"                     },  // 'x' UTF-8 78         (input # 88 -> glyph # 89)
  {  5, "\x0c\x50\x50\x50\x3c"                     },  // 'y' UTF-8 79         (input # 89 -> glyph # 90)
  {  5, "\x44\x64\x54\x4c\x44"                     },  // 'z' UTF-8 7A         (input # 90 -> glyph # 91)
  {  3, "\x08\x36\x41"                             },  // '{' UTF-8 7B         (input # 91 -> glyph # 92)
  {  1, "\x7f"                                     },  // '|' UTF-8 7C         (input # 92 -> glyph # 93)
  {  3, "\x41\x36\x08"                             },  // '}' UTF-8 7D         (input # 93 -> glyph # 94)
  {  5, "\x04\x02\x04\x08\x04"                     },  // '~' UTF-8 7E         (input # 94 -> glyph # 95)
  {  5, "\x7d\x0a\x09\x0a\x7d"                     },  // 'Ä' UTF-8 C3 84     (input # 96 -> glyph # 96)
  {  5, "\x3d\x42\x42\x42\x3d"                     },  // 'Ö' UTF-8 C3 96     (input # 97 -> glyph # 97)
  {  5, "\x3d\x40\x40\x40\x3d"                     },  // 'Ü' UTF-8 C3 9C     (input # 98 -> glyph # 98)
  {  5, "\x20\x55\x56\x54\x78"                     },  // 'à' UTF-8 C3 A0     (input # 106 -> glyph # 99)
  {  5, "\x20\x54\x56\x55\x78"                     },  // 'á' UTF-8 C3 A1     (input # 107 -> glyph # 100)
  {  5, "\x20\x56\x55\x56\x78"                     },  // 'â' UTF-8 C3 A2     (input # 108 -> glyph # 101)
  {  5, "\x20\x55\x54\x55\x78"                     },  // 'ä' UTF-8 C3 A4     (input # 99 -> glyph # 102)
  {  5, "\x38\x44\xe4\x44\x08"                     },  // 'ç' UTF-8 C3 A7     (input # 113 -> glyph # 103)
  {  5, "\x38\x55\x56\x54\x18"                     },  // 'è' UTF-8 C3 A8     (input # 102 -> glyph # 104)
  {  5, "\x38\x54\x56\x55\x18"                     },  // 'é' UTF-8 C3 A9     (input # 103 -> glyph # 105)
  {  5, "\x38\x56\x55\x56\x18"                     },  // 'ê' UTF-8 C3 AA     (input # 104 -> glyph # 106)
  {  5, "\x38\x55\x54\x55\x18"                     },  // 'ë' UTF-8 C3 AB     (input # 105 -> glyph # 107)
  {  3, "\x4a\x78\x42"                             },  // 'ì' UTF-8 C3 AC     (input # 109 -> glyph # 108)
  {  3, "\x4a\x78\x42"                             },  // 'í' UTF-8 C3 AD     (input # 110 -> glyph # 109)
  {  3, "\x4a\x79\x42"                             },  // 'î' UTF-8 C3 AE     (input # 111 -> glyph # 110)
  {  3, "\x4a\x78\x42"                             },  // 'ï' UTF-8 C3 AF     (input # 112 -> glyph # 111)
  {  5, "\x38\x45\x44\x45\x38"                     },  // 'ö' UTF-8 C3 B6     (input # 100 -> glyph # 112)
  {  5, "\x3c\x41\x40\x41\x7c"                     },  // 'ü' UTF-8 C3 BC     (input # 101 -> glyph # 113)
};

const int placeholderGlyphNo = 0;

static const GlyphRange glyphRanges[] = {
  { "", 0x20, 0x7E, 1 }, //  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
  { "\xc3", 0x84, 0x84, 96 }, // Ä
  { "\xc3", 0x96, 0x96, 97 }, // Ö
  { "\xc3", 0x9C, 0x9C, 98 }, // Ü
  { "\xc3", 0xA0, 0xA2, 99 }, // àáâ
  { "\xc3", 0xA4, 0xA4, 102 }, // ä
  { "\xc3", 0xA7, 0xAF, 103 }, // çèéêëìíîï
  { "\xc3", 0xB6, 0xB6, 112 }, // ö
  { "\xc3", 0xBC, 0xBC, 113 }, // ü
  { NULL, 0, 0}
};

// MARK: - end of generated glyph data

// 'Ä' UTF8 C3 84  -> glyph # 96)

// 'Ö' UTF8 C3 96  -> glyph # 97)

// 'Ü' UTF8 C3 9C  -> glyph # 98)

// 'à' UTF8 C3 A0  -> glyph # 99)
// 'á' UTF8 C3 A1  -> glyph # 100)
// 'â' UTF8 C3 A2  -> glyph # 101)

// 'ä' UTF8 C3 A4  -> glyph # 102)

// 'ç' UTF8 C3 A7  -> glyph # 103)
// 'è' UTF8 C3 A8  -> glyph # 104)
// 'é' UTF8 C3 A9  -> glyph # 105)
// 'ê' UTF8 C3 AA  -> glyph # 106)
// 'ë' UTF8 C3 AB  -> glyph # 107)
// 'ì' UTF8 C3 AC  -> glyph # 108)
// 'í' UTF8 C3 AD  -> glyph # 109)
// 'î' UTF8 C3 AE  -> glyph # 110)
// 'ï' UTF8 C3 AF  -> glyph # 111)

// 'ö' UTF8 C3 B6  -> glyph # 112)

// 'ü' UTF8 C3 BC  -> glyph # 113)


// Glyph to codepoint mapping

/// @return glyph number or -1 if EOT
static int glyphNoFromText(size_t &aIdx, const char* aText) {
  string prefix;
  uint8_t textbyte = aText[aIdx++];
  if (!textbyte) return -1; // end of text
  if (textbyte>=0xC0) {
    // start of UTF8 sequence, get prefix
    while ((aText[aIdx]&0xC0)==0x80) {
      // next is still a continuation
      prefix += textbyte;
      textbyte = aText[aIdx++];
      if (!textbyte) return -1; // invalid sequence ends text
    }
  }
  else if (textbyte>=0x80) {
    return placeholderGlyphNo; // UTF-8 continuation without start -> invalid
  }
  // we have the prefix and the last char
  const GlyphRange* grP = glyphRanges;
  bool prefixmatched = false;
  while (grP->prefix) {
    if (prefix == grP->prefix) {
      // prefix matches
      if (textbyte>=grP->first && textbyte<=grP->last) {
        // match
        return grP->glyphOffset+(textbyte-grP->first);
      }
    }
    grP++;
  }
  // no match
  return placeholderGlyphNo;
}


#if TEXT_FONT_GEN

static void fontAsBinString()
{
  printf("\n\nstatic const char * fontBin[] = {\n");
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


typedef std::pair<string, string> StringPair;
static bool mapcmp(const StringPair &a, const StringPair &b)
{
  return a.first < b.first;
}

static bool binStringAsFont()
{
  int numInputGlyphs = sizeof(fontBin)/sizeof(char*);
  printf("\n\n// MARK: - generated glyph data\n\n");
  printf("const int numGlyphs = %d;", numInputGlyphs);
  printf("\n\nstatic const glyph_t fontGlyphs[numGlyphs] = {\n");
  // UTF8 mappings
  typedef std::list<StringPair> GlyphMap;
  GlyphMap gm;
  for (int g=0; g<numInputGlyphs; ++g) {
    const char* bs0 = fontBin[g];
    const char* bs = bs0;
    // first string up to newline is character, possibly UTF8
    while (*bs && *bs!='\n') bs++;
    string code;
    code.assign(bs0, bs-bs0);
    bs++; // skip LF
    // create character definition
    int w = 0;
    string chr = "\"";
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
    chr += "\"";
    string codedesc = code; // default to non-code
    if (code=="placeholder") {
      code="\0"; // make sure it comes first
    }
    else {
      codedesc = string_format("'%s' UTF-8", code.c_str());
      for (size_t i=0; i<code.size(); i++) string_format_append(codedesc, " %02X", (uint8_t)code[i]);
    }
    string chardef = string_format("  { %2d, %-42s },  // %-20s (input # %d", w, chr.c_str(), codedesc.c_str(), g);
    gm.push_back(make_pair(code,chardef));
  }
  // now sort by Code
  gm.sort(mapcmp);
  int gno = 0;
  for (GlyphMap::iterator g = gm.begin(); g!=gm.end(); ++g, ++gno) {
    printf("%s -> glyph # %d)\n", g->second.c_str(), gno);
  }
  printf("};\n\n");
  // placeholder is always the first glyph
  printf("const int placeholderGlyphNo = 0;\n\n");
  // generate glyph number lookup
  printf("static const GlyphRange glyphRanges[] = {\n");
  string prefix="NONE";
  string rangedesc;
  gno = 0;
  int glyphOffset = 0;
  string dispchars;
  for (GlyphMap::iterator g = gm.begin(); g!=gm.end(); ++gno) {
    string code = g->first;
    ++g;
    if (code[0]==0) continue; // this is the 0 codepoint which is the placeholder
    dispchars += code; // accumulate for display
    // split in prefix and lastbyte
    uint8_t lastbyte = code[code.size()-1];
    code.erase(code.size()-1);
    if (code!=prefix) {
      // start a new prefix
      prefix = code;
      glyphOffset = gno;
      rangedesc = "  { \"";
      for (size_t i=0; i<code.size(); i++) string_format_append(rangedesc, "\\x%02x", (uint8_t)code[i]);
      string_format_append(rangedesc, "\", 0x%02X, ", lastbyte);
    }
    if (g==gm.end() || (uint8_t)(g->first[g->first.size()-1])!=lastbyte+1) {
      // end of range (but maybe same prefix)
      prefix = "NONE";
      string_format_append(rangedesc, "0x%02X, %d }, // %s", lastbyte, glyphOffset, dispchars.c_str());
      printf("%s\n", rangedesc.c_str());
      dispchars.clear();
      rangedesc.clear();
    }
  }
  printf("  { NULL, 0, 0}\n");
  printf("};\n");
  printf("\n// MARK: - end of generated glyph data\n\n");
  return true;
}

/*
const int placeholderGlyphNo = 0;

static const GlyphRange glyphRanges[] = {
  { ""    , 0x20, 0x7F, 0 },
  { "\xC3", 0x84, 0x84, 96 }, // Ä
  { "\xC3", 0x96, 0x96, 97 }, // Ö
  { "\xC3", 0x9C, 0x9C, 98 }, // Ü
  { "\xC3", 0xA0, 0xA2, 99 }, // à,á,â
  { "\xC3", 0xA4, 0xA4, 102 }, // ä
  { "\xC3", 0xA7, 0xAF, 103 }, // ç,è,é,ê,ë,ì,í,î,ï
  { "\xC3", 0xB6, 0xB6, 112 }, // ö
  { "\xC3", 0xBC, 0xBC, 113 }, // ü
  { NULL, 0, 0}
};
*/


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
    // render
    string glyphs;
    size_t i = 0;
    while (i<mText.size()) {
      int glyphNo = glyphNoFromText(i, mText.c_str());
      const glyph_t &g = fontGlyphs[glyphNo];
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
