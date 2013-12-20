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

#ifndef DFA_H_BY_CYCLOPS
#define DFA_H_BY_CYCLOPS

#include <iostream>
#include <nfa.h>

using namespace std;

#define SH_DFA_MAX_LETTERS SH_NFA_MAX_LETTERS

#define SH_DFA_SUCCESS 0
#define SH_DFA_FAIL -1

class shDFA
{
public:
	int start;
	int trapState;
	int alphabetSize;
	int inLetter[256];			// map of actual characters to defined letters in alphabet
	vector<bool> final;
	vector<vector<int> > to;
	shLetter alphabet[SH_DFA_MAX_LETTERS];
	string name;
	void shDFAStore(ostream &out);
	int shDFALoad(istream &in);
	void shDFADump();
};

#endif
