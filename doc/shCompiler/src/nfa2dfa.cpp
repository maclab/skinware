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

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <queue>
#include <dfa.h>
#include <nfa2dfa.h>

using namespace std;

//#define SH_DEBUG

#ifdef SH_DEBUG
void shN2DConverter::p_shDumpSet(const set<int> &s)
{
	for (set<int>::const_iterator i = s.begin(), end = s.end(); i != end; ++i)
		cout << *i << " ";
	cout << endl;
}
#endif

set<int> shN2DConverter::p_shUnion(const set<int> &s1, const set<int> &s2)
{
	set<int> res = s1;
	for (set<int>::const_iterator i = s2.begin(), end = s2.end(); i != end; ++i)
		res.insert(*i);
	return res;
}

set<int> shN2DConverter::p_shDFSOnLambda(shNFA &nfa, int current)
{
	set<int> states;
	if (p_shDFSMark[current])
		return states;
	p_shDFSMark[current] = true;
	states.insert(current);
	for (list<int>::const_iterator i = nfa.states[current].lambda.begin(), end = nfa.states[current].lambda.end(); i != end; ++i)
		states = p_shUnion(states, p_shDFSOnLambda(nfa, *i));
	return states;
}

shDFA shN2DConverter::shN2DConvert(shNFA &nfa, string name)
{
	map<set<int>, int> newStates;			// maps a set of states of NFA into a state in DFA
	vector<vector<int> > dfa;
	vector<bool> final;
	p_shDFSMark = new bool[nfa.states.size()];
	if (!p_shDFSMark)
	{
		shDFA d;
		cout << "NFA to DFA converter out of memory!" << endl;
		d.start = -1;				// indiciation of failure
		return d;
	}
	memset(p_shDFSMark, 0, nfa.states.size()*sizeof(bool));
	set<int> start = p_shDFSOnLambda(nfa, nfa.start);
	int stateCount = 1;
	newStates[start] = 0;
	dfa.push_back(vector<int>(nfa.alphabetSize));
	int trapState = -1;				// from trap state you can go nowhere
	queue<set<int> > Q;
	Q.push(start);
	while (!Q.empty())
	{
		set<int> current = Q.front();		// a set of reachable nfa states (reachable with the same sequence,
							// hence mapped to one state in the dfa)
		Q.pop();
#ifdef SH_DEBUG
		cout << "Picked from queue:" << endl;
		p_shDumpSet(current);
		cout << endl;
#endif
		bool is_final = false;
		for (set<int>::const_iterator i = current.begin(), end = current.end(); i != end; ++i)
			if (nfa.states[*i].final)
			{
				is_final = true;
				break;
			}
		final.push_back(is_final);
		for (int letter = 0; letter < nfa.alphabetSize; ++letter)
		{
			set<int> next;
			for (set<int>::const_iterator iter = current.begin(), end = current.end(); iter != end; ++iter)
				for (list<int>::const_iterator to = nfa.states[*iter].next[letter].begin(), end2 = nfa.states[*iter].next[letter].end();
						to != end2; ++to)
					if (next.find(*to) == next.end())
					{
						next.insert(*to);
						memset(p_shDFSMark, 0, nfa.states.size()*sizeof(bool));
						next = p_shUnion(next, p_shDFSOnLambda(nfa, *to));
					}
			if (newStates.find(next) == newStates.end())
			{
				if (next.size() == 0)
					trapState = stateCount;
				newStates[next] = stateCount++;
				dfa.push_back(vector<int>(nfa.alphabetSize));
				Q.push(next);
			}
			dfa[newStates[current]][letter] = newStates[next];
		}
	}
	delete[] p_shDFSMark;
	shDFA res;
	res.trapState = trapState;
	res.final = final;
	res.to = dfa;
	res.alphabetSize = nfa.alphabetSize;
	res.start = 0;
	memset(res.inLetter, -1, sizeof(res.inLetter));
	for (int i = 0; i < res.alphabetSize; ++i)
	{
		res.alphabet[i].name = nfa.alphabet[i].name;
		res.alphabet[i].letter = nfa.alphabet[i].letter;
		for (set<char>::const_iterator iter = res.alphabet[i].letter.begin(), end = res.alphabet[i].letter.end(); iter != end; ++iter)
		{
			if (res.inLetter[(int)*iter] != -1)
			{
				cout << "Letters " << res.alphabet[i].name << " and " <<
				res.alphabet[res.inLetter[(int)*iter]].name << " overlap in character " << *iter << endl;
				res.start = -1;		// indication of failure
				return res;
			}
			else
				res.inLetter[(int)*iter] = i;
		}
	}
	res.name = name;
	return res;
}

int shN2DConverter::p_shGetParent(int node)
{
	int current = node;
	while (p_shReductionSet[current] != current)			// find parent
		current = p_shReductionSet[current];
	int parent = current;
	current = node;
	while (p_shReductionSet[current] != current)			// set parent of all in the path to the found parent
	{								// to optimize future findParent's of those nodes
		int temp = p_shReductionSet[current];
		p_shReductionSet[current] = parent;
		current = temp;
	}
	return parent;
}

bool shN2DConverter::p_shIfExistsEqualStatesThenUnion(shDFA &dfa)
{
	for (unsigned int i = 0, size = dfa.to.size(); i < size; ++i)
		for (unsigned int j = 1; j < size; ++j)
		{
			if (dfa.final[i] != dfa.final[j])
				continue;
			if (p_shGetParent(i) == p_shGetParent(j))
				continue;
			bool equal = true;
			for (int k = 0; k < dfa.alphabetSize; ++k)
			{
				if (p_shGetParent(dfa.to[i][k]) != p_shGetParent(dfa.to[j][k]) &&
					((dfa.to[i][k] != (int)i && dfa.to[i][k] != (int)j) || (dfa.to[j][k] != (int)i && dfa.to[j][k] != (int)j)))
				{
					equal = false;
					break;
				}
			}
			if (equal)
			{
#ifdef SH_DEBUG
				cout << "Unifying: " << p_shGetParent(j) << " and " << p_shGetParent(i) << endl;
#endif
				p_shReductionSet[p_shGetParent(j)] = p_shGetParent(i);
				return true;
			}
		}
	return false;
}

shDFA shN2DConverter::shDFASimplify(shDFA &dfa)
{
	shDFA res;
	p_shReductionSet = new int[dfa.to.size()];
	for (unsigned int i = 0, size = dfa.to.size(); i < size; ++i)
		p_shReductionSet[i] = i;
	set<int> remainingStates;
	map<int, int> newStateAssignment;
	while (p_shIfExistsEqualStatesThenUnion(dfa));
	for (unsigned int i = 0, size = dfa.to.size(); i < size; ++i)
		remainingStates.insert(p_shGetParent(i));
	int count = 0;
	for (set<int>::const_iterator iter = remainingStates.begin(), end = remainingStates.end(); iter != end; ++iter)
	{
#ifdef SH_DEBUG
		cout << "New assignment: " << *iter << " to " << count << endl;
#endif
		newStateAssignment[*iter] = count++;
	}
	res.start = newStateAssignment[dfa.start];
	res.alphabetSize = dfa.alphabetSize;
	res.final.resize(count);
	res.to.resize(count);
	for (int i = 0; i < count; ++i)
		res.to[i].resize(res.alphabetSize);
	for (int i = 0; i < SH_DFA_MAX_LETTERS; ++i)
	{
		res.alphabet[i].name = dfa.alphabet[i].name;
		res.alphabet[i].letter = dfa.alphabet[i].letter;
	}
	for (unsigned int i = 0, size = dfa.to.size(); i < size; ++i)
	{
		if (p_shGetParent(i) != (int)i)
			continue;
		int newI = newStateAssignment[i];
		res.final[newI] = dfa.final[i];
		for (int j = 0; j < dfa.alphabetSize; ++j)
			res.to[newI][j] = newStateAssignment[p_shGetParent(dfa.to[i][j])];
	}
	if (dfa.trapState == -1)
		res.trapState = -1;
	else
		res.trapState = newStateAssignment[p_shGetParent(dfa.trapState)];
	res.name = dfa.name;
	memcpy(res.inLetter, dfa.inLetter, sizeof(dfa.inLetter));
	delete[] p_shReductionSet;
	return res;
}
