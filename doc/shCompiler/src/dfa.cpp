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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dfa.h>

using namespace std;

void shDFA::shDFAStore(ostream &out)
{
	out << "start = " << start << endl;
	out << "trapState = " << trapState << endl;
	out << "alphabetSize = " << alphabetSize << endl;
	out << "size = " << to.size() << endl;
	out << "states =" << endl;
	for (unsigned int i = 0, size = to.size(); i < size; ++i)
	{
		out << final[i];
		for (int j = 0; j < alphabetSize; ++j)
			out << " " << to[i][j];
		out << endl;
	}
	out << "name = " << name << endl;
	out << "alphabet =" << endl;
	for (int i = 0; i < alphabetSize; ++i)
	{
		out << alphabet[i].name << " " << alphabet[i].letter.size();
		for (set<char>::const_iterator iter = alphabet[i].letter.begin(), end = alphabet[i].letter.end(); iter != end; ++iter)
			out << " " << (int)*iter;
		out << endl;
	}
}

int shDFA::shDFALoad(istream &in)
{
	unsigned int size = 1;
	alphabetSize = 1;
	string s;
	string eq;
	while (in >> s >> eq)
	{
		if (s == "start")
			in >> start;
		else if (s == "trapState")
			in >> trapState;
		else if (s == "alphabetSize")
			in >> alphabetSize;
		else if (s == "size")
			in >> size;
		else if (s == "name")
			in >> name;
		else if (s == "states")
		{
			final.resize(size);
			to.resize(size);
			for (unsigned int i = 0; i < size; ++i)
			{
				to[i].resize(alphabetSize);
				int is_final;
				in >> is_final;
				final[i] = is_final;
				for (int j = 0; j < alphabetSize; ++j)
					in >> to[i][j];
			}
		}
		else if (s == "alphabet")
		{
			for (int i = 0; i < alphabetSize; ++i)
			{
				int asciiCode;
				int count;
				in >> alphabet[i].name >> count;
				for (int j = 0; j < count; ++j)
				{
					in >> asciiCode;
					alphabet[i].letter.insert(asciiCode);
				}
			}
		}
		else
		{
			cout << "Error in dfa file!" << endl;
			return SH_DFA_FAIL;
		}
	}
	memset(inLetter, -1, sizeof(inLetter));
	for (int i = 0; i < alphabetSize; ++i)
	{
		for (set<char>::const_iterator iter = alphabet[i].letter.begin(), end = alphabet[i].letter.end(); iter != end; ++iter)
		{
			if (inLetter[(int)*iter] != -1)
			{
				cout << "Letters " << alphabet[i].name << " and " << alphabet[inLetter[(int)*iter]].name
					<< " overlap in character " << *iter << endl;
				return SH_DFA_FAIL;
			}
			else
				inLetter[(int)*iter] = i;
		}
	}
	return SH_DFA_SUCCESS;
}

void shDFA::shDFADump()
{
	for (unsigned int i = 0, size = to.size(); i < size; ++i)
	{
		cout << ((final[i])?"Final":"     ");
		for (int j = 0; j < alphabetSize; ++j)
			cout << " " << to[i][j];
		if ((int)i == trapState)
			cout << " Trap!";
		cout << endl;
	}
}
