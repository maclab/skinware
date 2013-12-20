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
#include "grammar.h"
#include "nonstd.h"

using namespace std;

static const char *_keywords =
"\n";

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
	"grammar_file -> @grammarStart rules @grammarEnd",
	"rules -> rule_def rules",
	"rules -> ",
	"rule_def -> @newRule ID @pushName EXPANDS or_expr END @endRule",
	"or_expr -> cat_expr more_or_expr",
	"more_or_expr -> @addRuleBody OR @appendOr cat_expr more_or_expr",
	"more_or_expr -> ",
	"cat_expr -> unary_expr more_cat_expr",
	"cat_expr -> ",
	"more_cat_expr -> unary_expr more_cat_expr",
	"more_cat_expr -> ",
	"unary_expr -> primary_expr unary_operators",
	"unary_expr -> handler",
	"unary_operators -> unary_operator unary_operators",
	"unary_operators -> ",
	"unary_operator -> KLEENE_STAR @star",
	"unary_operator -> AT_LEAST_ONCE @atLeastOnce",
	"unary_operator -> MAYBE @maybe",
	"unary_operator -> NUMBER @times",
	"primary_expr -> ID @push",
	"primary_expr -> PAREN_OPEN @parenStart or_expr @addRuleBody @parenEnd PAREN_CLOSE",
	"handler -> ACTION_ROUTINE @push",
	"handler -> EPR_HANDLER @push"
};

static const char *_grammarTable =
"cat_expr 7 ACTION_ROUTINE\n"
"cat_expr 8 END\n"
"cat_expr 7 EPR_HANDLER\n"
"cat_expr 7 ID\n"
"cat_expr 8 OR\n"
"cat_expr 8 PAREN_CLOSE\n"
"cat_expr 7 PAREN_OPEN\n"
"grammar_file 0\n"
"grammar_file 0 ID\n"
"handler 21 ACTION_ROUTINE\n"
"handler 22 EPR_HANDLER\n"
"more_cat_expr 9 ACTION_ROUTINE\n"
"more_cat_expr 10 END\n"
"more_cat_expr 9 EPR_HANDLER\n"
"more_cat_expr 9 ID\n"
"more_cat_expr 10 OR\n"
"more_cat_expr 10 PAREN_CLOSE\n"
"more_cat_expr 9 PAREN_OPEN\n"
"more_or_expr 6 END\n"
"more_or_expr 5 OR\n"
"more_or_expr 6 PAREN_CLOSE\n"
"or_expr 4 ACTION_ROUTINE\n"
"or_expr 4 END\n"
"or_expr 4 EPR_HANDLER\n"
"or_expr 4 ID\n"
"or_expr 4 OR\n"
"or_expr 4 PAREN_CLOSE\n"
"or_expr 4 PAREN_OPEN\n"
"primary_expr 19 ID\n"
"primary_expr 20 PAREN_OPEN\n"
"rule_def 3 ID\n"
"rules 2\n"
"rules 1 ID\n"
"unary_expr 12 ACTION_ROUTINE\n"
"unary_expr 12 EPR_HANDLER\n"
"unary_expr 11 ID\n"
"unary_expr 11 PAREN_OPEN\n"
"unary_operator 16 AT_LEAST_ONCE\n"
"unary_operator 15 KLEENE_STAR\n"
"unary_operator 17 MAYBE\n"
"unary_operator 18 NUMBER \n"
"unary_operators 14 ACTION_ROUTINE\n"
"unary_operators 13 AT_LEAST_ONCE\n"
"unary_operators 14 END\n"
"unary_operators 14 EPR_HANDLER\n"
"unary_operators 14 ID\n"
"unary_operators 13 KLEENE_STAR\n"
"unary_operators 13 MAYBE\n"
"unary_operators 13 NUMBER \n"
"unary_operators 14 OR\n"
"unary_operators 14 PAREN_CLOSE\n"
"unary_operators 14 PAREN_OPEN\n";

static void _noFunctionForLLKErrorProductionRule(stack<shParser::shLLKStackElement> &, shLexer &, string &, shParser::shRule &){}
static void _noTokenMatchFunction(char *fc, int *lb, int *f, const char *file, int *l, int *c);
static bool _phraseLevelErrorHandler(stack<shParser::shParser::shLLKStackElement> &s, shLexer &l, string &em,
		string &cur, shKLimitedList<string> &la, shKLimitedList<string> &lat);

// action routines
static void _grammarStart(shLexer &lexer, shParser &parser);
static void _grammarEnd(shLexer &lexer, shParser &parser);
static void _newRule(shLexer &lexer, shParser &parser);
static void _endRule(shLexer &lexer, shParser &parser);
static void _pushName(shLexer &lexer, shParser &parser);
static void _addRuleBody(shLexer &lexer, shParser &parser);
static void _appendOr(shLexer &lexer, shParser &parser);
static void _star(shLexer &lexer, shParser &parser);
static void _atLeastOnce(shLexer &lexer, shParser &parser);
static void _maybe(shLexer &lexer, shParser &parser);
static void _times(shLexer &lexer, shParser &parser);
static void _push(shLexer &lexer, shParser &parser);
static void _parenStart(shLexer &lexer, shParser &parser);
static void _parenEnd(shLexer &lexer, shParser &parser);

void shLexer::shLInitToGrammarFile()
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

void shParser::shPInitToGrammarFile()
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
	p_nonterminals.push_back("grammar_file");
	p_nonterminals.push_back("rules");
	p_nonterminals.push_back("rule_def");
	p_nonterminals.push_back("or_expr");
	p_nonterminals.push_back("cat_expr");
	p_nonterminals.push_back("more_or_expr");
	p_nonterminals.push_back("more_cat_expr");
	p_nonterminals.push_back("unary_expr");
	p_nonterminals.push_back("unary_operators");
	p_nonterminals.push_back("unary_operator");
	p_nonterminals.push_back("primary_expr");
	p_nonterminals.push_back("handler");
	p_nonterminalCodes["frammar_file"] = 0;
	p_nonterminalCodes["rules"] = 1;
	p_nonterminalCodes["rule_def"] = 2;
	p_nonterminalCodes["or_expr"] = 3;
	p_nonterminalCodes["cat_expr"] = 4;
	p_nonterminalCodes["more_or_expr"] = 5;
	p_nonterminalCodes["more_cat_expr"] = 6;
	p_nonterminalCodes["unary_expr"] = 7;
	p_nonterminalCodes["unary_operators"] = 8;
	p_nonterminalCodes["unary_operator"] = 9;
	p_nonterminalCodes["primary_expr"] = 10;
	p_nonterminalCodes["handler"] = 11;
	p_nonterminalCount = 12;
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
	shPAddActionRoutine("@grammarStart", _grammarStart);
	shPAddActionRoutine("@grammarEnd", _grammarEnd);
	shPAddActionRoutine("@newRule", _newRule);
	shPAddActionRoutine("@endRule", _endRule);
	shPAddActionRoutine("@pushName", _pushName);
	shPAddActionRoutine("@addRuleBody", _addRuleBody);
	shPAddActionRoutine("@appendOr", _appendOr);
	shPAddActionRoutine("@star", _star);
	shPAddActionRoutine("@atLeastOnce", _atLeastOnce);
	shPAddActionRoutine("@maybe", _maybe);
	shPAddActionRoutine("@times", _times);
	shPAddActionRoutine("@push", _push);
	shPAddActionRoutine("@parenStart", _parenStart);
	shPAddActionRoutine("@parenEnd", _parenEnd);
}

struct _nameElement
{
	bool is_compound;
	string name;
	list<list<string> > ruleBodies;		// Usually rules are generated immediately after | or ; however, in case of (x|..., the nonterminal of rule is not yet known so the rule bodies are
						// added here so that when ) is met, they would all be added together
};

struct _bodyElement
{
	bool start;				// marks start of rule body
	string component;			// if !start, then this could be a terminal, non-terminal, action routing or EPR handler
};

static list<shParser::shRule> _grammarRules;
static list<shParser::shRule> _auxRules;	// rules generated by operators (such as *, ? and ())
static stack<_nameElement> _ruleNamesStack;
static stack<_bodyElement> _ruleBody;		// body of current rule being read.  Note that if in the middle of rule there was parenthesis, its body would be added to the same stack, and later retrieved
						// back until start is true
static bool _evaluate;

static int _error;

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
	_error = SH_P_GRAMMAR_ERROR;
}

#define stack_clear(s) while (!s.empty()) s.pop()

void shcompiler_GF_init()
{
	_error = SH_P_SUCCESS;
}

void shcompiler_GF_cleanup()
{
	_grammarRules.clear();
	_auxRules.clear();
	stack_clear(_ruleNamesStack);
	stack_clear(_ruleBody);
}

list<shParser::shRule> shcompiler_GF_getRules()
{
	return _grammarRules;
}

int shcompiler_GF_error()
{
	return _error;
}

static void _grammarStart(shLexer &lexer, shParser &parser)
{
	_evaluate = true;
	stack_clear(_ruleNamesStack);
	_grammarRules.clear();
	_auxRules.clear();
}

static void _grammarEnd(shLexer &lexer, shParser &parser)
{
	stack_clear(_ruleNamesStack);
	stack_clear(_ruleBody);
	_grammarRules.splice(_grammarRules.end(), _auxRules);		// append auxiliary rules to the end of rules
}

static void _markNewRule()
{
	_bodyElement b;
	b.start = true;
	_ruleBody.push(b);
}

static void _addRule(const shParser::shRule &rule)
{
	bool duplicate = false;
	for (list<shParser::shRule>::const_iterator i = _grammarRules.begin(), end = _grammarRules.end(); i != end; ++i)
		if (i->head != rule.head)
			continue;
		else if (i->body != rule.body)
			continue;
		else
		{
			duplicate = true;
			break;
		}
	if (!duplicate)
		_grammarRules.push_back(rule);
}

static void _addAuxRule(const shParser::shRule &rule)
{
	bool duplicate = false;
	for (list<shParser::shRule>::const_iterator i = _auxRules.begin(), end = _auxRules.end(); i != end; ++i)
		if (i->head != rule.head)
			continue;
		else if (i->body != rule.body)
			continue;
		else
		{
			duplicate = true;
			break;
		}
	if (!duplicate)
		_auxRules.push_back(rule);
}

static void _appendToName(const string &str)
{
/*	_nameElement ruleName = _ruleNamesStack.top();
	_ruleNamesStack.pop();
	ruleName.name += str;
	_ruleNamesStack.push(ruleName);*/
	_ruleNamesStack.top().name += str;
}

static void _newRule(shLexer &lexer, shParser &parser)
{
	stack_clear(_ruleBody);		// they should be
	stack_clear(_ruleNamesStack);	// empty anyway!
	_markNewRule();
}

static void _endRule(shLexer &lexer, shParser &parser)
{
	shParser::shRule r;
	r.head = _ruleNamesStack.top().name;
	_ruleNamesStack.pop();
	while (_ruleBody.size() > 0 && !_ruleBody.top().start)
	{
		r.body.push_front(_ruleBody.top().component);
		_ruleBody.pop();
	}
	if (_ruleBody.size() == 0)
		cout << lexer.shLFileName() << ":" << lexer.shLLine() << ":" << lexer.shLColumn() << ": internal error: no start mark when retrieving rule body" << endl;
	else
		_ruleBody.pop();
	_addRule(r);
}

static void _pushName(shLexer &lexer, shParser &parser)
{
	_nameElement n;
	n.is_compound = false;
	n.name = lexer.shLCurrentToken();
	_ruleNamesStack.push(n);
}

static void _addRuleBody(shLexer &lexer, shParser &parser)
{
	list<string> body;
	while (_ruleBody.size() > 0 && !_ruleBody.top().start)
	{
		body.push_front(_ruleBody.top().component);
		_ruleBody.pop();
	}
	if (_ruleBody.size() == 0)
	{
		cout << lexer.shLFileName() << ":" << lexer.shLLine() << ":" << lexer.shLColumn() << ": internal error: no start mark when retrieving rule body" << endl;
		_markNewRule();
	}
	if (_ruleNamesStack.top().is_compound)
		_ruleNamesStack.top().ruleBodies.push_back(body);
	else
	{
		shParser::shRule r;
		r.head = _ruleNamesStack.top().name;		// don't pop the name
		r.body = body;
		_addRule(r);
	}
}

static void _appendOr(shLexer &lexer, shParser &parser)
{
	if (_ruleNamesStack.top().is_compound)
		_appendToName("|");
}

static void _addStar(const string &component)
{
	string newName = component+'*';
	shParser::shRule r1, r2;
	r1.head = newName;
	r1.body.push_back(component);
	r1.body.push_back(r1.head);
	r2.head = newName;
	_addAuxRule(r1);
	_addAuxRule(r2);
}

static void _star(shLexer &lexer, shParser &parser)
{
	string lastComponent = _ruleBody.top().component;
	_ruleBody.pop();
	_bodyElement b;
	b.start = false;
	b.component = lastComponent+'*';
	_ruleBody.push(b);
	_addStar(lastComponent);
	if (_ruleNamesStack.top().is_compound)
		_appendToName("*");
}

static void _atLeastOnce(shLexer &lexer, shParser &parser)
{
	string lastComponent = _ruleBody.top().component;
	_ruleBody.pop();
	string newName = lastComponent+'+';
	_bodyElement b;
	b.start = false;
	b.component = newName;
	_ruleBody.push(b);
	shParser::shRule r;
	r.head = newName;
	r.body.push_back(lastComponent);
	r.body.push_back(lastComponent+'*');
	_addAuxRule(r);
	_addStar(lastComponent);		// in case lastComponent * was not previously defined
	if (_ruleNamesStack.top().is_compound)
		_appendToName("+");
}

static void _maybe(shLexer &lexer, shParser &parser)
{
	string lastComponent = _ruleBody.top().component;
	_ruleBody.pop();
	string newName = lastComponent+'?';
	_bodyElement b;
	b.start = false;
	b.component = newName;
	_ruleBody.push(b);
	shParser::shRule r1, r2;
	r1.head = newName;
	r1.body.push_back(lastComponent);
	r2.head = newName;
	_addAuxRule(r1);
	_addAuxRule(r2);
	if (_ruleNamesStack.top().is_compound)
		_appendToName("?");
}

static void _times(shLexer &lexer, shParser &parser)
{
	string lastComponent = _ruleBody.top().component;
	_ruleBody.pop();
	int count = 0;
	sscanf(lexer.shLCurrentToken().c_str(), "%d", &count);
	if (count == 0)			// if count is 0, just drop the lastComponent
		return;
	string newName = lastComponent+lexer.shLCurrentToken();
	_bodyElement b;
	b.start = false;
	b.component = newName;
	_ruleBody.push(b);
	shParser::shRule r;
	r.head = newName;
	for (int i = 0; i < count; ++i)
		r.body.push_back(lastComponent);
	_addAuxRule(r);
	if (_ruleNamesStack.top().is_compound)
		_appendToName(lexer.shLCurrentToken());
}

static void _push(shLexer &lexer, shParser &parser)
{
	_bodyElement b;
	b.start = false;
	b.component = lexer.shLCurrentToken();
	_ruleBody.push(b);
	if (_ruleNamesStack.top().is_compound && b.component[0] != '@' && b.component[0] != '!')
		_appendToName(b.component);
}

static void _parenStart(shLexer &lexer, shParser &parser)
{
	_markNewRule();
	_nameElement n;
	n.is_compound = true;
	n.name = '(';
	_ruleNamesStack.push(n);
}

static void _parenEnd(shLexer &lexer, shParser &parser)
{
	_appendToName(")");
	shParser::shRule r;
	string compoundName = _ruleNamesStack.top().name;
	r.head = compoundName;
	for (list<list<string> >::const_iterator i = _ruleNamesStack.top().ruleBodies.begin(), end = _ruleNamesStack.top().ruleBodies.end(); i != end; ++i)
	{
		r.body = *i;
		_addAuxRule(r);
	}
	_bodyElement b;
	b.start = false;
	b.component = compoundName;
	if (_ruleBody.size() == 0)
		cout << lexer.shLFileName() << ":" << lexer.shLLine() << ":" << lexer.shLColumn() << ": internal error: no start mark when closing parenthesis" << endl;
	else
		_ruleBody.pop();
	_ruleBody.push(b);
	_ruleNamesStack.pop();
	if (_ruleNamesStack.top().is_compound)	// if it was parenthesis inside parenthesis
		_appendToName(compoundName);
}

#define MAX_MESSAGE_SIZE 1000

static int _missingToken = 0;
static bool _eofErrorGiven = false;

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
	else if (cur == "EXPANDS")
	{
		l.shLInsertFakeToken(cur);
		snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing expand operator, inserted '->'\n",
				l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
		em += temp;
	}
	else if (cur == "ID")
	{
		l.shLInsertFakeToken(cur);
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
		if (*token0 == "EQUAL")
		{
			// Since lookaheads are not yet read, then to get the currect line and column, I shall peek one token ahead
			int line, column;
			string ignore;
			l.shLPeekNextToken(ignore, 0, &line, &column);
			snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: '=' cannot appear in this file\n",
					l.shLFileName().c_str(), line, column);
			em += temp;
			s.push(cur);
			s.push(*token0);
		}
		else if (*token0 == "")
		{
			if (!_eofErrorGiven)
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected end of file before end of rule defintion\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
			}
			_eofErrorGiven = true;
			s.push("@grammarEnd");
			s.push("@endRule");
		}
		else if (cur == "grammar_file" || cur == "rules")
		{
			if (*token0 == "ACTION_ROUTINE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected action routine on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "AT_LEAST_ONCE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '+' on the left side of rule\n",
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
			else if (*token0 == "EPR_HANDLER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected error production rule handler on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "EXPANDS")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing non-terminal on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				char id[100];
				sprintf(id, "__missing_%d", _missingToken++);
				l.shLInsertFakeToken("ID", id);
				s.push("rules");
				s.push("@endRule");
				s.push("END");
				s.push("or_expr");
				s.push("EXPANDS");
				s.push("@pushName");
				// s.push("ID");	// missing ID faked
				s.push("@newRule");
			}
			else if (*token0 == "KLEENE_STAR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '*' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "MAYBE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '?' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "NUMBER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '?' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: note:  identifiers cannot start with a digit\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "OR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '|' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_CLOSE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected ')' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_OPEN")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '(' on the left side of rule\n",
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
		else if (cur == "rule_def")
		{
			if (*token0 == "ACTION_ROUTINE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected action routine on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "AT_LEAST_ONCE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '+' on the left side of rule\n",
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
			else if (*token0 == "EPR_HANDLER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected error production rule handler on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "EXPANDS")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing non-terminal on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				char id[100];
				sprintf(id, "__missing_%d", _missingToken++);
				l.shLInsertFakeToken("ID", id);
				s.push("@endRule");
				s.push("END");
				s.push("or_expr");
				s.push("EXPANDS");
				s.push("@pushName");
				// s.push("ID");	// missing ID faked
				s.push("@newRule");
			}
			else if (*token0 == "KLEENE_STAR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '*' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "MAYBE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '?' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "NUMBER")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '?' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: note:  identifiers cannot start with a digit\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "OR")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '|' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_CLOSE")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected ')' on the left side of rule\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push(cur);
				s.push(*token0);
			}
			else if (*token0 == "PAREN_OPEN")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unexpected '(' on the left side of rule\n",
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
			else if (*token0 == "EXPANDS")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous rule definition\n",
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
			else
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: unhandled error, expected to match %s, but found %s\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());
				em += temp;
			}
		}
		else if (cur == "more_or_expr")
		{
			if (*token0 == "AT_LEAST_ONCE" || *token0 == "ID" || *token0 == "KLEENE_STAR" || *token0 == "MAYBE" || *token0 == "NUMBER" || *token0 == "PAREN_OPEN"
					|| *token0 == "ACTION_ROUTINE" || *token0 == "EPR_HANDLER")		// Note: this shouldn't happen
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected '|'\n",
						l.shLFileName().c_str(), l.shLLine(), l.shLColumn());
				em += temp;
				s.push("more_or_expr");
				s.push("cat_expr");
				s.push("@appendOr");
				// s.push("OR");	// missing '|' ignored
				s.push("@addRuleBody");
			}
			else if (*token0 == "EXPANDS")
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous rule definition\n",
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
			else if (*token0 == "EXPANDS")
			{
				// error will be given with more_or_expr
/*				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: missing semicolon at the end of previous rule definition\n",
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
			if (*token0 == "EXPANDS")
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
		else if (cur == "handler")
		{
			if (*token0 == "AT_LEAST_ONCE" || *token0 == "END" || *token0 == "EXPANDS" || *token0 == "ID" || *token0 == "KLEENE_STAR" || *token0 == "MAYBE"
				|| *token0 == "NUMBER" || *token0 == "OR" || *token0 == "PAREN_CLOSE" || *token0 == "PAREN_OPEN")		// Note: this shouldn't happen
			{
				snprintf(temp, MAX_MESSAGE_SIZE, "%s:%d:%d: error: expected to match action routine or error production rule handler\n",
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
		else if (cur == "unary_operator")
		{
			if (*token0 == "END" || *token0 == "EXPANDS" || *token0 == "ID" || *token0 == "OR" || *token0 == "PAREN_CLOSE" || *token0 == "PAREN_OPEN" ||
					*token0 == "ACTION_ROUTINE" || *token0 == "EPR_HANDLER")		// Note: this shouldn't happen
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
	_error = SH_P_GRAMMAR_ERROR;
	return recovered;
}
