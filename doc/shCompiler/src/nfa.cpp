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
#include <nfa.h>

using namespace std;

shNFA::shNFA(int letter): states(2)
{
	start = 0;
	final = 1;
	states[final].final = true;
	if (letter >= 0)
		states[start].next[letter].push_back(final);
	else
		states[start].lambda.push_back(final);
}

shNFA::shNFA(): states(1)
{
	start = 0;
	final = 0;
	states[final].final = true;
}

shNFA::~shNFA()
{
}

shNFA::shNFA(const shNFA &orig)					// The copy constructor!
{
	*this = orig;
}

shNFA &shNFA::operator =(const shNFA &second)
{
	start = second.start;
	final = second.final;
	states = second.states;
	for (int i = 0; i < SH_NFA_MAX_LETTERS; ++i)
	{
		alphabet[i].letter = second.alphabet[i].letter;
		alphabet[i].name = second.alphabet[i].name;
	}
	alphabetSize = second.alphabetSize;
	return *this;
}

shNFA &shNFA::operator +=(const shNFA &second)			// Builds another NFA which is the + of both NFAs
{
	int offsetSecond = states.size();
	vector<shNFAState> combStates(states.size()+second.states.size()+2);
	// Copy seconds data in this new array
	for (unsigned int i = 0, size = states.size(); i < size; ++i)
		combStates[i] = states[i];
	for (unsigned int i = 0, size = second.states.size(); i < size; ++i)
	{
		combStates[i+offsetSecond] = second.states[i];
		// Correct all indices:
		for (list<int>::iterator iter = combStates[i+offsetSecond].lambda.begin(), end = combStates[i+offsetSecond].lambda.end();
				iter != end; ++iter)
			*iter += offsetSecond;
		for (int j = 0; j < SH_NFA_MAX_LETTERS; ++j)
			for (list<int>::iterator iter = combStates[i+offsetSecond].next[j].begin(), end = combStates[i+offsetSecond].next[j].end();
					iter != end; ++iter)
				*iter += offsetSecond;
	}
	int p_start = states.size()+second.states.size();
	int p_final = states.size()+second.states.size()+1;
	combStates[p_final].final = true;
	combStates[final].final = false;
	combStates[final].lambda.push_back(p_final);
	combStates[second.final+offsetSecond].final = false;
	combStates[second.final+offsetSecond].lambda.push_back(p_final);
	combStates[p_start].lambda.push_back(start);
	combStates[p_start].lambda.push_back(second.start+offsetSecond);
	states = combStates;
	start = p_start;
	final = p_final;
	return *this;
}

shNFA &shNFA::operator *=(const shNFA &second)			// Builds another NFA which is the . of both NFAs
{
	int offsetSecond = states.size();
	vector<shNFAState> combStates(states.size()+second.states.size());
	// Copy seconds data in this new array
	for (unsigned int i = 0, size = states.size(); i < size; ++i)
		combStates[i] = states[i];
	for (unsigned int i = 0, size = second.states.size(); i < size; ++i)
	{
		combStates[i+offsetSecond] = second.states[i];
		// Correct all indices:
		for (list<int>::iterator iter = combStates[i+offsetSecond].lambda.begin(), end = combStates[i+offsetSecond].lambda.end();
				iter != end; ++iter)
			*iter += offsetSecond;
		for (int j = 0; j < SH_NFA_MAX_LETTERS; ++j)
			for (list<int>::iterator iter = combStates[i+offsetSecond].next[j].begin(), end = combStates[i+offsetSecond].next[j].end();
					iter != end; ++iter)
				*iter += offsetSecond;
	}
	combStates[final].final = false;
	combStates[final].lambda.push_back(second.start+offsetSecond);
	states = combStates;
	final = second.final+offsetSecond;
	return *this;
}

shNFA &shNFA::operator *=(const int count)			// Builds another NFA which is the same NFA .ed count times
										// count == 0 means (NFA)+
										// count == -1 means (NFA)?
										// count < -1 means (NFA)*
{
	if (count == 1)
		return *this;
	else if (count > 1)
	{
		shNFA orig(*this);						// Take a copy first ;)
		for (int i = 1; i < count; ++i)
			*this *= orig;
		return *this;
	}
	else if (count == 0)							// compute (NFA)+
	{
		vector<shNFAState> combStates(states.size()+2);
		// Copy data in this new array
		for (unsigned int i = 0, size = states.size(); i < size; ++i)
			combStates[i] = states[i];
		int p_start = states.size();
		int p_final = states.size()+1;
		combStates[p_final].final = true;
		combStates[final].final = false;
		combStates[p_start].lambda.push_back(start);
		// The only difference with (NFA)* is that there is no lamda transition from p_start to p_final
		combStates[p_final].lambda.push_back(p_start);
		combStates[final].lambda.push_back(p_final);
		states = combStates;
		start = p_start;
		final = p_final;
		return *this;
	}
	else if (count == -1)							// compute (NFA)?
	{
		vector<shNFAState> combStates(states.size()+2);
		// Copy data in this new array
		for (unsigned int i = 0, size = states.size(); i < size; ++i)
			combStates[i] = states[i];
		int p_start = states.size();
		int p_final = states.size()+1;
		combStates[p_final].final = true;
		combStates[final].final = false;
		combStates[p_start].lambda.push_back(start);
		combStates[p_start].lambda.push_back(p_final);
		// The only difference with (NFA)* is that there is no lamda transition from p_final to p_start
		combStates[final].lambda.push_back(p_final);
		states = combStates;
		start = p_start;
		final = p_final;
		return *this;
	}
	else									// compute (NFA)*
	{
		vector<shNFAState> combStates(states.size()+2);
		// Copy data in this new array
		for (unsigned int i = 0, size = states.size(); i < size; ++i)
			combStates[i] = states[i];
		int p_start = states.size();
		int p_final = states.size()+1;
		combStates[p_final].final = true;
		combStates[final].final = false;
		combStates[p_start].lambda.push_back(start);
		combStates[p_start].lambda.push_back(p_final);
		combStates[p_final].lambda.push_back(p_start);
		combStates[final].lambda.push_back(p_final);
		states = combStates;
		start = p_start;
		final = p_final;
		return *this;
	}
}

void shNFA::shNFADump()
{
	cout << "Start: " << start << endl;
	cout << "End: " << final << endl;
	for (unsigned int i = 0, size = states.size(); i < size; ++i)
	{
		cout << i << " ";
		if (states[i].final)
			cout << "Final";
		else
			cout << "Not final";
		cout << endl;
		cout << "Lambda:";
		for (list<int>::const_iterator iter = states[i].lambda.begin(), end = states[i].lambda.end(); iter != end; ++iter)
			cout << " " << *iter;
		cout << endl;
		for (int j = 0; j < alphabetSize; ++j)
		{
			cout << alphabet[j].name << ":";
			for (list<int>::const_iterator iter = states[i].next[j].begin(), end = states[i].next[j].end(); iter != end; ++iter)
				cout << " " << *iter;
			cout << endl;
		}
	}
}
