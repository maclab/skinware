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

#ifndef NFA_H_BY_CYCLOPS
#define NFA_H_BY_CYCLOPS

#include <list>
#include <set>
#include <string>
#include <vector>

using namespace std;

#define SH_NFA_MAX_LETTERS 128

struct shLetter
{
	string name;
	set<char> letter;
};

class shNFAState
{
public:
	bool final;
	list<int> lambda;					// All states it goes with a lambda!
	list<int> next[SH_NFA_MAX_LETTERS];
	shNFAState()
	{
		final = false;
	}
};

class shNFA
{
public:
	int start;
	int final;						// Always keep the final unique!
	vector<shNFAState> states;
	shLetter alphabet[SH_NFA_MAX_LETTERS];
	// alphabetSize must be set!
	int alphabetSize;
	// An NFA here shall always have at least two states, start goes to final by a letter
	shNFA(int letter);					// If given letter is -1, an NFA with lambda transition is made
	shNFA();
	~shNFA();
	shNFA(const shNFA &orig);				// The copy constructor!
	shNFA &operator =(const shNFA &second);
	shNFA &operator +=(const shNFA &second);		// Builds another NFA which is the + of both NFAs
	shNFA &operator *=(const shNFA &second);		// Builds another NFA which is the . of both NFAs
	shNFA &operator *=(const int count);			// Builds another NFA which is the same NFA "."ed count times
								// count == 0 means (NFA)+
								// count == -1 means (NFA)?
								// count < -1 means (NFA)*
	void shNFADump();
};

#endif
