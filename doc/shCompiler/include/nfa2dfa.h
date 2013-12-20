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

#ifndef NFA2DFA_H_BY_CYCLOPS
#define NFA2DFA_H_BY_CYCLOPS

#include <dfa.h>

#define SH_NFA2DFA_SUCCESS 0
#define SH_NFA2DFA_FAIL -1

class shN2DConverter
{
protected:
	bool *p_shDFSMark;
	// A disjoint set, with past-compression
	int *p_shReductionSet;
#ifdef SH_DEBUG
	void p_shDumpSet(const set<int> &s);
#endif
	set<int> p_shUnion(const set<int> &s1, const set<int> &s2);
	set<int> p_shDFSOnLambda(shNFA &nfa, int current);
	int p_shGetParent(int node);
	bool p_shIfExistsEqualStatesThenUnion(shDFA &dfa);
public:
	shDFA shN2DConvert(shNFA &nfa, string name);
	shDFA shDFASimplify(shDFA &dfa);
};

#endif
