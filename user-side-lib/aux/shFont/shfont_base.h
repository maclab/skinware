/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shFont.
 *
 * shFont is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shFont is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shFont.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SH_FONT_BASE_H_BY_CYCLOPS
#define SH_FONT_BASE_H_BY_CYCLOPS

#define			SH_FONT_SUCCESS			0
#define			SH_FONT_FAIL			-1
#define			SH_FONT_FILE_NOT_FOUND		-2
#define			SH_FONT_NO_MEM			-3
#define			SH_FONT_NO_GRAPHICS_MEM		-4
#define			SH_FONT_ALREADY_LOADED		-5

#define			SH_FONT_NO_SHADOW		0.0f, 0.0f, false
#define			SH_FONT_FULL_SHADOW		2.0f, 2.0f, true	// NOTE: This means that 4 shadows are rendered so is slower. The shadows will be in (dirx, diry), (dirx, -diry), (-dirx, diry) and (-dirx, -diry)
#define			SH_FONT_SHADOW_LEFT		-2.0f, 0.0f, false
#define			SH_FONT_SHADOW_RIGHT		2.0f, 0.0f, false
#define			SH_FONT_SHADOW_BELOW		0.0f, -2.0f, false
#define			SH_FONT_SHADOW_ABOVE		0.0f, 2.0f, false
#define			SH_FONT_SHADOW_RIGHT_BELOW	2.0f, -2.0f, false
#define			SH_FONT_SHADOW_RIGHT_ABOVE	2.0f, 2.0f, false
#define			SH_FONT_SHADOW_LEFT_BELOW	-2.0f, -2.0f, false
#define			SH_FONT_SHADOW_LEFT_ABOVE	-2.0f, 2.0f, false

#define			SH_FONT_LEFT_ALIGN		0x01
#define			SH_FONT_RIGHT_ALIGN		0x02
#define			SH_FONT_CENTER_ALIGN		0x03
#define			SH_FONT_TOP_ALIGN		0x10
#define			SH_FONT_BOTTOM_ALIGN		0x20
#define			SH_FONT_VERTICAL_CENTER_ALIGN	0x30

#ifdef __GNUC__
#define SHFONT_PRINTF_STYLE(format_index, args_index) __attribute__ ((format (printf, format_index, args_index)))
#else
#define SHFONT_PRINTF_STYLE(format_index, args_index)
#endif

// File and memory operations
int shFontConvertToFont(const char *bmpFile, const char *fontFile);	// The bmp file should have red background (that is RGB = (255, 0, 0))
									// All the letters must be black over white with grayscale defining
									// transparency (white = transparent)
									// The letters should come in order of ascii code, starting from '\0'. They are
									// ordered as they appear first from top to bottom, then from left to right.
									// Each letter can have any width (> 0) and height (> 0)
									// The target binary file has format:
									// (256 times, for each letter:) left (4 bytes) bottom (4b) width (2b) height (2b)
									// texture width (4 bytes), texture height (4 bytes)
									// The texture image of the font with only one value per taxel: transparency
int shFontConvertToBMP(const char *fontFile, const char *bmpFile);
int shFontLoad(const char *fontFile);
void shFontFree();
bool shFontIsLoaded();

// Font rendering
void shFontWrite(int *windowFrame, const char *text, ...) SHFONT_PRINTF_STYLE(2, 3);
									// binds texture on every call. Note: I tested the performance, writing 20000 times
									// "No messages yet" and checking to see if already bound increased the frame rate
									// from 16~17 to 17. My decision was that it's fine rebinding.
									// Window frame is 4 values of left bottom right top saying how the
									// font should be clipped. Note that shFontWrite assumes it is writing
									// from (0, 0) (that is top left of the text). If window frame is '\0', it is ignored.
									// Note also that (0, 0) is the top left corner of the text and the text is written
									// in negative direction of y axis, therefore, you will most likely give negative values
									// for y in windowFrame

// Font manipulation
void shFontColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);	// "a" makes the whole text transparent, multiplied by
void shFontColor(unsigned char *rgb, unsigned char a = 255);					// the transparency of the font itself
void shFontSize(float sizeX, float sizeY = 0.0f);						// if sizeY was 0.0f, sizeX is used for both
void shFontShadowColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
void shFontShadowColor(unsigned char *rgb, unsigned char a = 255);
void shFontShadow(float directionX, float directionY, bool fullshadow = false);			// either directly give the shadow direction (if fullshadow
												// is selected, direction is ignored) or use
												// SH_FONT_SHADOW_* or SH_FONT_NO_SHADOW or SH_FONT_FULL_SHADOW
												// as predefined parameters
void shFontLetterSpacing(float spacing);		// extra space between characters
void shFontLineSpacing(float spacing);			// extra space between lines
void shFontAlignment(unsigned int alignment);		// "or"ed value of SH_FONT_*_ALIGN values, if both left and right, then centers (same for vertical)

// Font utilities
float shFontTextWidth(const char *text, float scaleX = 0.0f);	// predicts the width of the text to be written. If the text has mutliple lines, the maximum of line widths is returned.
float shFontTextHeight(const char *text, float scaleY = 0.0f);	// predicts the height of the text to be written
void shFontTextDimensions(const char *text, float scaleX, float scaleY, float *width, float *height);	// combination of shFontTextWidth and shFontTextHeight
float shFontLineWidth(const char *text, float scaleX = 0.0f);	// similar to shFontTextWidth, but predicts the width of only the current line, that is it stops on '\n' or '\r'
float shFontLineHeight(const char *text, float scaleY = 0.0f);	// similar to shFontTextHeight, but predicts the height of only the current line, that is it stops on '\n' or '\r'

#endif
