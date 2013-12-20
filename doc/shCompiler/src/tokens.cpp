/*
 * Copyright (C) 2007-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shCompiler.
 *
 * shCompiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shCompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shCompiler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <string>
#include <list>
#include <lexer.h>
#include <parser.h>
#include <nfa2dfa.h>
#include "tokens.h"
#include "nonstd.h"

using namespace std;

static const char *_keywords =
"lambda\n"
"new_line\n"
"space\n"
"tab\n";

static const char *_token_ID =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 4\n"
"states =\n"
"0 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 2 2 1 1 1 1 1 1 1 1 3 1 1 1 1 1 1 1\n"
"0 1 1 1 2 2 1 1 1 1 1 1 1 1 3 1 1 1 1 1 1 1\n"
"name = ID\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_EQUAL =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = EQUAL\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_END =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = END\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_WHITE_SPACE =
"start = 0\n"
"trapState = 2\n"
"alphabetSize = 21\n"
"size = 7\n"
"states =\n"
"0 1 1 1 2 2 2 2 2 2 2 2 2 2 2 2 2 3 2 2 2 2\n"
"1 1 1 1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2\n"
"0 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2\n"
"0 3 4 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 5 3 3 3\n"
"1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2\n"
"0 3 6 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 5 3 3 3\n"
"1 3 4 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 5 3 3 3\n"
"name = WHITE_SPACE\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_PAREN_OPEN =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = PAREN_OPEN\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_PAREN_CLOSE =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = PAREN_CLOSE\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_NUMBER =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = NUMBER\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_MAYBE =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = MAYBE\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_KLEENE_STAR =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = KLEENE_STAR\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_OR =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = OR\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_AT_LEAST_ONCE =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 3\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 2 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = AT_LEAST_ONCE\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_ACTION_ROUTINE =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 5\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 3 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 3 3 1 1 1 1 1 1 1 1 4 1 1 1 1 1 1 1\n"
"0 1 1 1 3 3 1 1 1 1 1 1 1 1 4 1 1 1 1 1 1 1\n"
"name = ACTION_ROUTINE\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_EPR_HANDLER =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 5\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 3 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 3 3 1 1 1 1 1 1 1 1 4 1 1 1 1 1 1 1\n"
"0 1 1 1 3 3 1 1 1 1 1 1 1 1 4 1 1 1 1 1 1 1\n"
"name = EPR_HANDLER\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

static const char *_token_EXPANDS =
"start = 0\n"
"trapState = 1\n"
"alphabetSize = 21\n"
"size = 6\n"
"states =\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 2 3 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 4 1 1 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 5 1 1 1 1 1 1\n"
"0 1 1 1 1 1 1 1 1 1 1 1 5 1 1 1 1 1 1 1 1 1\n"
"1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n"
"name = EXPANDS\n"
"alphabet =\n"
"space 1 32\n"
"new_line 1 10\n"
"tab 1 9\n"
"digit 10 48 49 50 51 52 53 54 55 56 57\n"
"letter 55 46 47 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 95 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122\n"
"left_paren 1 40\n"
"right_paren 1 41\n"
"bar 1 124\n"
"star 1 42\n"
"plus 1 43\n"
"question 1 63\n"
"equal 1 61\n"
"colon 1 58\n"
"dash 1 45\n"
"angle_right 1 62\n"
"semicolon 1 59\n"
"hash 1 35\n"
"backslash 1 92\n"
"at 1 64\n"
"exclamation 1 33\n"
"illegal 14 34 36 37 38 39 44 60 91 93 94 96 123 125 126\n";

#define RULE_COUNT 23

static const char *_rules[RULE_COUNT] =
{
	"tokens_file -> @tokensStart tokens @tokensEnd",
	"tokens -> token_def tokens",
	"tokens -> ",
	"token_def -> @newNFA ID @setName EQUAL or_expr END @endNFA",
	"or_expr -> cat_expr more_or_expr",
	"more_or_expr -> OR cat_expr @or more_or_expr",
	"more_or_expr -> ",
	"cat_expr -> unary_expr more_cat_expr",
	"more_cat_expr -> unary_expr @cat more_cat_expr",
	"more_cat_expr -> ",
	"unary_expr -> primary_expr unary_operators",
	"unary_operators -> unary_operator unary_operators",
	"unary_operators -> ",
	"unary_operator -> KLEENE_STAR @star",
	"unary_operator -> AT_LEAST_ONCE @atLeastOnce",
	"unary_operator -> MAYBE @maybe",
	"unary_operator -> NUMBER @times",
	"primary_expr -> ID @push",
	"primary_expr -> lambda @push",
	"primary_expr -> space @push",
	"primary_expr -> tab @push",
	"primary_expr -> new_line @push",
	"primary_expr -> PAREN_OPEN or_expr PAREN_CLOSE"
};

static const char *_grammarTable =
"cat_expr 7 ID\n"
"cat_expr 7 PAREN_OPEN\n"
"cat_expr 7 lambda\n"
"cat_expr 7 new_line\n"
"cat_expr 7 space\n"
"cat_expr 7 tab\n"
"more_cat_expr 9 END\n"
"more_cat_expr 8 ID\n"
"more_cat_expr 9 OR\n"
"more_cat_expr 9 PAREN_CLOSE\n"
"more_cat_expr 8 PAREN_OPEN\n"
"more_cat_expr 8 lambda\n"
"more_cat_expr 8 new_line\n"
"more_cat_expr 8 space\n"
"more_cat_expr 8 tab\n"
"more_or_expr 6 END\n"
"more_or_expr 5 OR\n"
"more_or_expr 6 PAREN_CLOSE\n"
"or_expr 4 ID\n"
"or_expr 4 PAREN_OPEN\n"
"or_expr 4 lambda\n"
"or_expr 4 new_line\n"
"or_expr 4 space\n"
"or_expr 4 tab\n"
"primary_expr 17 ID\n"
"primary_expr 22 PAREN_OPEN\n"
"primary_expr 18 lambda\n"
"primary_expr 21 new_line\n"
"primary_expr 19 space\n"
"primary_expr 20 tab\n"
"token_def 3 ID\n"
"tokens 2\n"
"tokens 1 ID\n"
"tokens_file 0\n"
"tokens_file 0 ID\n"
"unary_expr 10 ID\n"
"unary_expr 10 PAREN_OPEN\n"
"unary_expr 10 lambda\n"
"unary_expr 10 new_line\n"
"unary_expr 10 space\n"
"unary_expr 10 tab\n"
"unary_operator 14 AT_LEAST_ONCE\n"
"unary_operator 13 KLEENE_STAR\n"
"unary_operator 15 MAYBE\n"
"unary_operator 16 NUMBER\n"
"unary_operators 11 AT_LEAST_ONCE\n"
"unary_operators 12 END\n"
"unary_operators 12 ID\n"
"unary_operators 11 KLEENE_STAR\n"
"unary_operators 11 MAYBE\n"
"unary_operators 11 NUMBER\n"
"unary_operators 12 OR\n"
"unary_operators 12 PAREN_CLOSE\n"
"unary_operators 12 PAREN_OPEN\n"
"unary_operators 12 lambda\n"
"unary_operators 12 new_line\n"
"unary_operators 12 space\n"
"unary_operators 12 tab\n";

static void _noFunctionForLLKErrorProductionRule(stack<shParser::shLLKStackElement> &, shLexer &, string &, shParser::shRule &){}
static void _noTokenMatchFunction(char *fc, int *lb, int *f, const char *file, int *l, int *c);
static bool _phraseLevelErrorHandler(stack<shParser::shParser::shLLKStackElement> &s, shLexer &l, string &em,
		string &cur, shKLimitedList<string> &la, shKLimitedList<string> &lat);

// action routines
static void _tokensStart(shLexer &lexer, shParser &parser);
static void _tokensEnd(shLexer &lexer, shParser &parser);
static void _setName(shLexer &lexer, shParser &parser);
static void _newNFA(shLexer &lexer, shParser &parser);
static void _endNFA(shLexer &lexer, shParser &parser);
static void _or(shLexer &lexer, shParser &parser);
static void _cat(shLexer &lexer, shParser &parser);
static void _star(shLexer &lexer, shParser &parser);
static void _atLeastOnce(shLexer &lexer, shParser &parser);
static void _maybe(shLexer &lexer, shParser &parser);
static void _times(shLexer &lexer, shParser &parser);
static void _push(shLexer &lexer, shParser &parser);

void shLexer::shLInitToTokensFile()
{
	shLDiscardKeywords();
	shLFreeDFAs();
	istringstream kin(_keywords);
	shLReadKeywords(kin);
	shDFA dfa;
	istringstream IDin(_token_ID); 				dfa.shDFALoad(IDin);			shLAddDFA(dfa);
	istringstream EQUALin(_token_EQUAL);			dfa.shDFALoad(EQUALin);			shLAddDFA(dfa);
	istringstream ENDin(_token_END);			dfa.shDFALoad(ENDin);			shLAddDFA(dfa);
	istringstream WHITE_SPACEin(_token_WHITE_SPACE);	dfa.shDFALoad(WHITE_SPACEin);		shLAddDFA(dfa);
	istringstream PAREN_OPENin(_token_PAREN_OPEN);		dfa.shDFALoad(PAREN_OPENin);		shLAddDFA(dfa);
	istringstream PAREN_CLOSEin(_token_PAREN_CLOSE);	dfa.shDFALoad(PAREN_CLOSEin);		shLAddDFA(dfa);
	istringstream NUMBERin(_token_NUMBER);			dfa.shDFALoad(NUMBERin);		shLAddDFA(dfa);
	istringstream MAYBEin(_token_MAYBE);			dfa.shDFALoad(MAYBEin);			shLAddDFA(dfa);
	istringstream KLEENE_STARin(_token_KLEENE_STAR);	dfa.shDFALoad(KLEENE_STARin);		shLAddDFA(dfa);
	istringstream ORin(_token_OR);				dfa.shDFALoad(ORin);			shLAddDFA(dfa);
	istringstream AT_LEAST_ONCEin(_token_AT_LEAST_ONCE);	dfa.shDFALoad(AT_LEAST_ONCEin);		shLAddDFA(dfa);
	istringstream ACTION_ROUTINEin(_token_ACTION_ROUTINE);	dfa.shDFALoad(ACTION_ROUTINEin);	shLAddDFA(dfa);
	istringstream EPR_HANDLERin(_token_EPR_HANDLER);	dfa.shDFALoad(EPR_HANDLERin);		shLAddDFA(dfa);
	istringstream EXPANDSin(_token_EXPANDS);		dfa.shDFALoad(EXPANDSin);		shLAddDFA(dfa);
	shLAddNoMatchErrorFunction(_noTokenMatchFunction);
	p_initialized = true;
}

static void _generateRule(const char *str, shParser::shRule *rule, shParser::shRule *ruleWithActionRoutine)
{
	istringstream r(str);
	string s;
	r >> s;
	rule->head = s;
	ruleWithActionRoutine->head = s;
	rule->body.clear();
	ruleWithActionRoutine->body.clear();
	r >> s;		// ignore ->
	while (r >> s)
	{
		if (s[0] != '@')
			rule->body.push_back(s);
		ruleWithActionRoutine->body.push_back(s);
	}
}

void shParser::shPInitToTokensFile()
{
	p_tokenTypes.clear();
	p_tokenTypes.insert("ID");
	p_tokenTypes.insert("EQUAL");
	p_tokenTypes.insert("END");
	p_tokenTypes.insert("WHITE_SPACE");
	p_tokenTypes.insert("PAREN_OPEN");
	p_tokenTypes.insert("PAREN_CLOSE");
	p_tokenTypes.insert("NUMBER");
	p_tokenTypes.insert("MAYBE");
	p_tokenTypes.insert("KLEENE_STAR");
	p_tokenTypes.insert("OR");
	p_tokenTypes.insert("AT_LEAST_ONCE");
	p_tokenTypes.insert("ACTION_ROUTINE");
	p_tokenTypes.insert("EPR_HANDLER");
	p_tokenTypes.insert("EXPANDS");
	istringstream kin(_keywords);
	string keyword;
	while (kin >> keyword)
		p_tokenTypes.insert(keyword);
	p_k_in_grammar = 1;
	p_ruleCount = 0;
	p_LLK_EPRHandleFunction.clear();
	p_LLK_pleh = _phraseLevelErrorHandler;
	p_ruleByCode.clear();
	p_ruleByCodeWithActionRoutine.clear();
	p_rulesByNonterminalCode.clear();
	p_nonterminalCodes.clear();
	p_nonterminals.clear();
	p_nonterminals.push_back("tokens_file");
	p_nonterminals.push_back("tokens");
	p_nonterminals.push_back("token_def");
	p_nonterminals.push_back("or_expr");
	p_nonterminals.push_back("cat_expr");
	p_nonterminals.push_back("more_or_expr");
	p_nonterminals.push_back("more_cat_expr");
	p_nonterminals.push_back("unary_expr");
	p_nonterminals.push_back("unary_operators");
	p_nonterminals.push_back("unary_operator");
	p_nonterminals.push_back("primary_expr");
	p_nonterminalCodes["tokens_file"] = 0;
	p_nonterminalCodes["tokens"] = 1;
	p_nonterminalCodes["token_def"] = 2;
	p_nonterminalCodes["or_expr"] = 3;
	p_nonterminalCodes["cat_expr"] = 4;
	p_nonterminalCodes["more_or_expr"] = 5;
	p_nonterminalCodes["more_cat_expr"] = 6;
	p_nonterminalCodes["unary_expr"] = 7;
	p_nonterminalCodes["unary_operators"] = 8;
	p_nonterminalCodes["unary_operator"] = 9;
	p_nonterminalCodes["primary_expr"] = 10;
	p_nonterminalCount = 11;
	for (int i = 0; i < RULE_COUNT; ++i)
	{
		shRule rule;
		shRule ruleWithActionRoutine;
		_generateRule(_rules[i], &rule, &ruleWithActionRoutine);
		p_ruleByCode[p_ruleCount] = rule;
		p_ruleByCodeWithActionRoutine[p_ruleCount] = ruleWithActionRoutine;
		p_rulesByNonterminalCode[p_nonterminalCodes[rule.head]].insert(p_ruleCount);
		p_LLK_EPRHandleFunction[p_ruleCount] = _noFunctionForLLKErrorProductionRule;
		++p_ruleCount;
		rule.body.clear();
		ruleWithActionRoutine.body.clear();
	}
	istringstream tin(_grammarTable);
	string line;
	string nonterminal;
	p_LLK_table_data tdata;
	while (tin >> nonterminal >> tdata.rule)
	{
		getline(tin, line);
		istringstream sin(line);
		list<string> tokens;
		for (int i = 0; i < p_k_in_grammar; ++i)
		{
			string tmp;
			sin >> tmp;                                     // If it was $, then "" would be read ;)
			tokens.push_back(tmp);
		}
		p_LLK_table[p_LLK_table_lookup(nonterminal, tokens)] = tdata;
	}
	p_LLK_tableBuilt = true;
	shPAddActionRoutine("@tokensStart", _tokensStart);
	shPAddActionRoutine("@tokensEnd", _tokensEnd);
	shPAddActionRoutine("@setName", _setName);
	shPAddActionRoutine("@newNFA", _newNFA);
	shPAddActionRoutine("@endNFA", _endNFA);
	shPAddActionRoutine("@or", _or);
	shPAddActionRoutine("@cat", _cat);
	shPAddActionRoutine("@star", _star);
	shPAddActionRoutine("@atLeastOnce", _atLeastOnce);
	shPAddActionRoutine("@maybe", _maybe);
	shPAddActionRoutine("@times", _times);
	shPAddActionRoutine("@push", _push);
}

static list<shDFA> _dfas;
static stack<shNFA> _nfaStack;
static bool _evaluate;
static bool _rebuild;
static int _error;

static shLetter _alphabet[128];
static int _alphabetCount;
static map<string, int> _alphabetMap;
static string _cachePath;

static bool _reloaded;
static string _tokenName;
static int _missingToken = 0;
static bool _eofErrorGiven = false;

static void _noTokenMatchFunction(char *fc, int *lb, int *f, const char *file, int *l, int *c)
{
	cout << file << ":" << *l << ":" << *c << ": error: no token can be matched, probably because of an invalid character (is it '";
	if (fc[*f] == '\n')
		cout << "\\n";
	else if (fc[*f] == '\r')
		cout << "\\r";
	else if (fc[*f] == '\t')
		cout << "\\t";
	else if (fc[*f] >= ' ' && fc[*f] < 127)
		cout << fc[*f];
	else
		cout << "\\" << (int)fc[*f];
	cout << "'?)" << endl;
	for (; *lb < *f; ++*lb, ++*c)
		if (*lb == '\n')
		{
			++*l;
			*c = 0;
		}
	if (fc[*f] == '\n')
	{
		++*l;
		*c = 0;
	}
	_error = SH_L_FAIL;
}

#define stack_clear(s) while (!s.empty()) s.pop()

void shcompiler_TF_init(bool rebuild, istream &lin, const char *cachePath)
{
	_rebuild = rebuild;
	_error = SH_L_SUCCESS;
	_dfas.clear();
	_missingToken = 0;
	_eofErrorGiven = false;
	_cachePath = cachePath;
	_alphabetCount = 0;
	_alphabetMap.clear();
	_alphabet[0].name = "space";
	_alphabet[1].name = "new_line";
	_alphabet[2].name = "tab";
	_alphabet[0].letter.insert(' ');
	_alphabet[1].letter.insert('\n');
	_alphabet[2].letter.insert('\t');
	_alphabetMap[_alphabet[0].name] = (_alphabetCount)++;
	_alphabetMap[_alphabet[1].name] = (_alphabetCount)++;
	_alphabetMap[_alphabet[2].name] = (_alphabetCount)++;
	while (lin >> _alphabet[_alphabetCount].name)
	{
		bool ignore_this = false;
		if (_alphabet[_alphabetCount].name == "lambda" || _alphabetMap.find(_alphabet[_alphabetCount].name) != _alphabetMap.end())
		{
			cout << "Error: Letter redifinition (letter " << _alphabet[_alphabetCount].name << "). Ignored" << endl;
			ignore_this = true;
		}
		else if (_alphabet[_alphabetCount].name.find('+') != string::npos || _alphabet[_alphabetCount].name.find('|') != string::npos || _alphabet[_alphabetCount].name.find('*') != string::npos ||
			_alphabet[_alphabetCount].name.find('?') != string::npos || _alphabet[_alphabetCount].name.find('(') != string::npos || _alphabet[_alphabetCount].name.find(')') != string::npos ||
			_alphabet[_alphabetCount].name.find('=') != string::npos || _alphabet[_alphabetCount].name.find('@') != string::npos || _alphabet[_alphabetCount].name.find(';') != string::npos ||
			_alphabet[_alphabetCount].name.find('#') != string::npos || _alphabet[_alphabetCount].name.find('\\') != string::npos ||
			(_alphabet[_alphabetCount].name[0] >= '0' && _alphabet[_alphabetCount].name[0] <= '9'))
		{
			cout << "Error: Letter name cannot contain +, *, ?, |, (, ), =, @, ;, # or \\ and cannot start with a digit (letter " <<
				_alphabet[_alphabetCount].name << "). Ignored" << endl;
			ignore_this = true;
		}
		if (!ignore_this)
			_alphabetMap[_alphabet[_alphabetCount].name] = _alphabetCount;
		int n;
		lin >> n;
		for (int i = 0; i < n; ++i)
		{
			char c;
			lin >> c;
			if (!ignore_this)
				_alphabet[_alphabetCount].letter.insert(c);
		}
		if (!ignore_this)
			++_alphabetCount;
	}
}

void shcompiler_TF_cleanup()
{
	_alphabetMap.clear();
	_dfas.clear();
	stack_clear(_nfaStack);
}

list<shDFA> shcompiler_TF_getDFAs()
{
	return _dfas;
}

int shcompiler_TF_error()
{
	return _error;
}

static void _tokensStart(shLexer &lexer, shParser &parser)
{
	_dfas.clear();
	stack_clear(_nfaStack);
}

static void _tokensEnd(shLexer &lexer, shParser &parser)
{
	stack_clear(_nfaStack);
}

static void _newNFA(shLexer &lexer, shParser &parser)
{
	_reloaded = false;
	_evaluate = true;
}

static void _endNFA(shLexer &lexer, shParser &parser)
{
	if (_reloaded || !_evaluate)
		return;
	shN2DConverter n2dc;
	if (_nfaStack.size() > 0)
	{
		shNFA nfa = _nfaStack.top();
		_nfaStack.pop();
		nfa.alphabetSize = _alphabetCount;
		for (int i = 0; i < _alphabetCount; ++i)
		{
			nfa.alphabet[i].name = _alphabet[i].name;
			nfa.alphabet[i].letter = _alphabet[i].letter;
		}
		shDFA dfa = n2dc.shN2DConvert(nfa, _tokenName);
		if (dfa.start == -1)
			return;		// There was a failure
		shDFA simpdfa = n2dc.shDFASimplify(dfa);
		ofstream dfaCache(FIX_PATH(string(_cachePath)+"/"+_tokenName+".dfa").c_str());
		simpdfa.shDFAStore(dfaCache);
		_dfas.push_back(dfa);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _setName(shLexer &lexer, shParser &parser)
{
	_tokenName = lexer.shLCurrentToken();
	ifstream dfaFile(FIX_PATH(_cachePath+"/"+_tokenName+".dfa").c_str());
	if (!_rebuild && dfaFile.is_open())
	{
		shDFA dfa;
		dfa.shDFALoad(dfaFile);
		_reloaded = true;
		_dfas.push_back(dfa);
	}
}

static void _or(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() >= 2)
	{
		shNFA nfa2 = _nfaStack.top();
		_nfaStack.pop();
		shNFA nfa1 = _nfaStack.top();
		_nfaStack.pop();
		nfa1 += nfa2;
		_nfaStack.push(nfa1);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _cat(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() >= 2)
	{
		shNFA nfa2 = _nfaStack.top();
		_nfaStack.pop();
		shNFA nfa1 = _nfaStack.top();
		_nfaStack.pop();
		nfa1 *= nfa2;
		_nfaStack.push(nfa1);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _star(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() > 0)
	{
		shNFA nfa = _nfaStack.top();
		_nfaStack.pop();
		nfa *= -2;
		_nfaStack.push(nfa);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _atLeastOnce(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() > 0)
	{
		shNFA nfa = _nfaStack.top();
		_nfaStack.pop();
		nfa *= 0;
		_nfaStack.push(nfa);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _maybe(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() > 0)
	{
		shNFA nfa = _nfaStack.top();
		_nfaStack.pop();
		nfa *= -1;
		_nfaStack.push(nfa);
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _times(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	if (_nfaStack.size() > 0)
	{
		shNFA nfa = _nfaStack.top();
		_nfaStack.pop();
		int count = 0;
		sscanf(lexer.shLCurrentToken().c_str(), "%d", &count);
		if (count != 0)		// if count is 0, just make a lambda NFA
		{
			nfa *= count;
			_nfaStack.push(nfa);
		}
		else
			_nfaStack.push(shNFA(-1));
	}
	else
	{
		_evaluate = false;
		_error = SH_L_FAIL;
	}
}

static void _push(shLexer &lexer, shParser &parser)
{
	if (_reloaded)
		return;
	string letter = lexer.shLCurrentToken();
	if (letter == "lambda")
		_nfaStack.push(shNFA(-1));
	else
		if (_alphabetMap.find(letter) == _alphabetMap.end())
		{
			cout << lexer.shLFileName() << ":" << lexer.shLLine() << ":" << lexer.shLColumn() << ": error: unknown letter name " << letter << endl;
			_evaluate = false;
			_error = SH_L_FAIL;
			_nfaStack.push(shNFA(-1));	// push a lambda instead of this unknown letter
		}
		else
			_nfaStack.push(_alphabetMap[letter]);
}

#define MAX_MESSAGE_SIZE 1000

static bool _phraseLevelErrorHandler(stack<shParser::shParser::shLLKStackElement> &s, shLexer &l, string &em,
		string &cur, shKLimitedList<string> &la, shKLimitedList<string> &lat)
{
	char temp[MAX_MESSAGE_SIZE+1];
	bool recovered = true;
	if (cur == "END")
	{
		l.shLInsertFakeToken(cur);
		snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon, inserted ';'\n",
				l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
		em += temp;
	}
	else if (cur == "EQUAL")
	{
		l.shLInsertFakeToken(cur);
		snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing assignment operator, inserted '='\n",
				l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
		em += temp;
	}
	else if (cur == "ID")
	{
		char id[100];
		sprintf(id, "__missing_%d", _missingToken++);
		l.shLInsertFakeToken(cur, id);
		snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing identifier, inserted \"%s\"\n",
				l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), l.shLCurrentToken().c_str());
		em += temp;
	}
	else if (cur == "PAREN_CLOSE")
	{
		l.shLInsertFakeToken(cur);
		snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing closing parenthesis, inserted ')'\n",
				l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
		em += temp;
	}
	else
	{
		list<string>::const_iterator token0 = la.k_elements().begin();
		string la_str;
		for (list<string>::const_iterator i = la.k_elements().begin(), end = la.k_elements().end(); i != end; ++i)
			la_str += (i == la.k_elements().begin()?"":" ")+((*i == "")?string("$"):*i);
		if (*token0 == "EXPANDS")
		{
			// Since lookaheads are not yet read, then to get the currect line and column, I shall peek one token ahead
			int line, column;
			string ignore;
			l.shLPeekNextToken(ignore, 0, &line, &column);
			snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: \"%s\" cannot appear in this file\n",
					l.shLFileName().c_str(), line, column, lat.k_elements().begin()->c_str());
			em += temp;
			s.push(cur);
			s.push(*token0);
		}
		else if (*token0 == "ACTION_ROUTINE")
		{
			int line, column;
			string ignore;
			l.shLPeekNextToken(ignore, 0, &line, &column);
			snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: names starting with '@' are reserved for action routines\n",
					l.shLFileName().c_str(), line, column);
			em += temp;
			s.push(cur);
			s.push(*token0);
		}
		else if (*token0 == "EPR_HANDLER")
		{
			int line, column;
			string ignore;
			l.shLPeekNextToken(ignore, 0, &line, &column);
			snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: names starting with '!' are reserved for error production rule handlers\n",
					l.shLFileName().c_str(), line, column);
			em += temp;
			s.push(cur);
			s.push(*token0);
		}
		else if (*token0 == "")
		{
			if (!_eofErrorGiven)
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected end of file before end of token defintion\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());		// Note: this shouldn't happen
				em += temp;
			}
			_eofErrorGiven = true;
			s.push("@tokensEnd");
			s.push("@endNFA");
			if (cur == "or_expr" || cur == "cat_expr" || cur == "unary_expr" || cur == "primary_expr")
			{
				l.shLInsertFakeToken("lambda");
				s.push("@push");
			}
		}
		else if (cur == "tokens_file" || cur == "tokens" || cur == "token_def")
		{
			if (*token0 == "AT_LEAST_ONCE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '+' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "END")		// just a single semicolon, ignore it
			{
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "EQUAL")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected token name before '='\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				char id[100];
				sprintf(id, "__missing_%d", _missingToken++);
				l.shLInsertFakeToken("ID", id);
				if (cur == "tokens_file")
					s.push("@tokensEnd");
				if (cur != "token_def")
					s.push("tokens");
				s.push("@endNFA");
				s.push("END");
				s.push("or_expr");
				s.push("EQUAL");
				s.push("@setName");
				// s.push("ID");	// missing ID faked
				s.push("@newNFA");
				if (cur == "tokens_file")
					s.push("@tokensStart");
			}
			else if (*token0 == "KLEENE_STAR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '*' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "MAYBE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '?' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "NUMBER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected number at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: note:  identifiers cannot start with a digit\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "OR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '|' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_CLOSE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected ')' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_OPEN")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '(' at start of token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "lambda" || *token0 == "new_line" || *token0 == "space" || *token0 == "tab")
			{
				char id[100];
				sprintf(id, "__missing_%d", _missingToken++);
				l.shLInsertFakeToken("ID", id);
				*la.k_elements().begin() = l.shLCurrentTokenType();
				*lat.k_elements().begin() = l.shLCurrentToken();
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: \"%s\" is a reserved word and cannot be used as a token name, replaced by %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), token0->c_str(), l.shLCurrentToken().c_str());
				em += temp;
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "or_expr" || cur == "cat_expr" || cur == "unary_expr" || cur == "primary_expr")
		{
			if (*token0 == "AT_LEAST_ONCE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '+'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "END")
			{
				l.shLInsertFakeToken("lambda");
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing expression in token definition or unclosed '('\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push("@push");
			}
			else if (*token0 == "EQUAL")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
			}
			else if (*token0 == "KLEENE_STAR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '*'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "MAYBE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '?'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "NUMBER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before repetition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "OR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '|'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_CLOSE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing expression before ')'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "more_or_expr")
		{
			if (*token0 == "AT_LEAST_ONCE" || *token0 == "ID" || *token0 == "KLEENE_STAR" || *token0 == "MAYBE" || *token0 == "NUMBER" || *token0 == "PAREN_OPEN" ||
					*token0 == "lambda" || *token0 == "new_line" || *token0 == "space" || *token0 == "tab")		// Note: this shouldn't happen
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected '|'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push("more_or_expr");
				s.push("@or");
				s.push("cat_expr");
				// s.push("OR");	// missing '|' ignored
			}
			else if (*token0 == "EQUAL")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "more_cat_expr")
		{
			if (*token0 == "AT_LEAST_ONCE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '+'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());		// Note: this shouldn't happen
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "EQUAL")
			{
				// error will be given with more_or_expr
/*				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;*/
			}
			else if (*token0 == "KLEENE_STAR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '*'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());		// Note: this shouldn't happen
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "MAYBE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before '?'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());		// Note: this shouldn't happen
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "NUMBER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected expression before repetition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());		// Note: this shouldn't happen
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "unary_operators")
		{
			if (*token0 == "EQUAL")
			{
				// error will be given with more_or_expr
/*				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous token definition\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;*/
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "unary_operator")
		{
			if (*token0 == "END" || *token0 == "EQUAL" || *token0 == "ID" || *token0 == "OR" || *token0 == "PAREN_CLOSE" || *token0 == "PAREN_OPEN" ||
					*token0 == "lambda" || *token0 == "new_line" || *token0 == "space" || *token0 == "tab")		// Note: this shouldn't happen
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected unary operator: '*', '+', '?' or a number\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
			}
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else
		{
			snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
					l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
			em += temp;
		}
	}
	_evaluate = false;
	_error = SH_L_FAIL;
	return recovered;
}
