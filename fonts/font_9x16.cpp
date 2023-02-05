//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Autogenerated by p44lrgraphics font generator
//
//  You should have received a copy of the GNU General Public License
//  along with p44lrgraphics. If not, see <http://www.gnu.org/licenses/>.
//

// This is a file to be included from textview.cpp to define a font

#ifdef GENERATE_FONT_SOURCE

// MARK: - 'font_9x16' generated font verification data

static const char * font_9x16_glyphstrings[] = {
  "placeholder" /* 0x00 - Glyph 0 */

  "\n"   "..XXXXXXXXXXXXXX"   // 3F FF
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..X............X"   // 20 01
  "\n"   "..XXXXXXXXXXXXXX" , // 3F FF

  "A" /* UTF-8 41 - Glyph 1 */

  "\n"   "..XXXXXXX......."   // 3F 84
  "\n"   "......X..XX....."   // 02 60
  "\n"   "......X....XX..."   // 02 18
  "\n"   "......X......XX."   // 02 06
  "\n"   "......X........X"   // 02 01
  "\n"   "......X......XX."   // 02 06
  "\n"   "......X....XX..."   // 02 18
  "\n"   "......X..XX....."   // 02 60
  "\n"   "..XXXXXXX......." , // 3F 80

  nullptr // terminator
};

// MARK: - end of generated font verification data


#endif // GENERATE_FONT_SOURCE


// MARK: - 'font_9x16' generated font data

static const glyph_t font_9x16_glyphs[] = {
  {  9, "\x3f\xff\x20\x01\x20\x01\x20\x01\x20\x01\x20\x01\x20\x01\x20\x01\x3f\xff" },  // placeholder          (input # 0 -> glyph # 0)
  {  9, "\x3f\x80\x02\x60\x02\x18\x02\x06\x02\x01\x02\x06\x02\x18\x02\x60\x3f\x80" },  // 'A' UTF-8 41         (input # 1 -> glyph # 1)
};

static const GlyphRange font_9x16_ranges[] = {
  { "", 0x41, 0x41, 1 }, // A
  { NULL, 0, 0}
};

static const font_t font_9x16 = {
  .fontName = "font_9x16",
  .glyphHeight = 16,
  .numGlyphs = 2,
  .glyphRanges = font_9x16_ranges,
  .glyphs = font_9x16_glyphs
};

// MARK: - end of generated font data
