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

#include <fstream>
#include <cstring>
#include <string>
#include <set>
#include <list>
#include <stack>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <lexer.h>
#include <parser.h>
#include "tokens.h"
#include "nonstd.h"

using namespace std;

//#define SH_DEBUG

static void _emptyMatchFunction(string &){}
static void _noMatchDefaultFunction(char *fc, int *lb, int *f, const char *file, int *l, int *c)
{
	cout << file << ":" << *l << ":" << *c << ": no token can be matched, probably because of an invalid character (is it '";
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
		if (fc[*lb] == '\n')
		{
			++*l;
			*c = 0;
		}
	if (fc[*f] == '\n')
	{
		++*l;
		*c = 0;
	}
}

shLexer::shLexer()
{
	p_initialized = false;
	p_text2lex = '\0';
	p_spaceMet = false;
	p_finished = false;
	p_token[0] = '\0';
	p_lastTokenType = "";
	p_currentTokenType = "";
	p_noTokenMatch = _noMatchDefaultFunction;
	p_line = 1;
	p_column = 0;
	p_lexemeBeginning = 0;
	p_ignoreWhiteSpace = true;
	p_peeked.clear();
}

void shLexer::shLFreeText()
{
	if (p_text2lex != '\0')
		delete[] p_text2lex;
	p_filename = "";
	p_text2lex = '\0';
	p_spaceMet = false;
	p_finished = false;
	p_token[0] = '\0';
	p_lastTokenType = "";
	p_currentTokenType = "";
	p_line = 1;
	p_column = 0;
	p_lexemeBeginning = 0;
	p_ignoreWhiteSpace = true;
	p_peeked.clear();
}

int shLexer::shLInitialize(const char *lettersFile, const char *tokensFile, const char *keywordsFile, bool rebuild)
{
	shLexer l;
	shParser p;
	l.shLInitToTokensFile();
	p.shPInitToTokensFile();
	ifstream lin(FIX_PATH(lettersFile).c_str());
	if (!lin.is_open())
	{
		cout << "Could not open letter's file" << endl;
		return SH_L_FILE_NOT_FOUND;
	}
	if (l.shLOpenFile(tokensFile) == SH_L_FILE_NOT_FOUND)
	{
		cout << "Could not open token's file" << endl;
		return SH_L_FILE_NOT_FOUND;
	}
	string cachePath = "";
	const char *lastSlash;
	for (lastSlash = tokensFile+strlen(tokensFile); lastSlash != tokensFile; --lastSlash)
		if (*lastSlash == '/')
			break;
	for (const char *p = tokensFile; p != lastSlash; ++p)
		cachePath += *p;
	if (lastSlash == tokensFile)		// there was no / at all
		cachePath += '.';
	cachePath += "/cache";
	(void)makeDir(cachePath);		// if could not create path (return value not zero), silently continue
	shcompiler_TF_init(rebuild, lin, cachePath.c_str());
	p.shPParseLLK(l);			// if parse error, the phrase level error handler would set the error so no need to check the output of parser
	cout << p.shPError();
	if (shcompiler_TF_error() != SH_L_SUCCESS)
		return shcompiler_TF_error();
	list<shDFA> dfas = shcompiler_TF_getDFAs();
	shcompiler_TF_cleanup();
	for (list<shDFA>::const_iterator i = dfas.begin(), end = dfas.end(); i != end; ++i)
		shLAddDFA(*i);
	ifstream kin(FIX_PATH(keywordsFile).c_str());
	if (!kin.is_open())
	{
		cout << "Could not open keyword's file" << endl;
		return SH_L_FILE_NOT_FOUND;
	}
	int ret = shLReadKeywords(kin);
	if (ret != SH_L_SUCCESS)
	{
		cout << "Error reading keywords" << endl;
		return ret;
	}
	p_initialized = true;
	return SH_L_SUCCESS;
}

int shLexer::shLOpenFile(const char *filename)
{
	FILE *file;
	if (filename == '\0')
		return SH_L_FAIL;
	if ((file = fopen(FIX_PATH(filename).c_str(), "r")) == '\0')
		return SH_L_FILE_NOT_FOUND;
	fseek(file, 0, SEEK_END);
	unsigned int length = ftell(file);
#ifdef SH_DEBUG
	cout << "File length is: " << length << endl;
#endif
	char *text = new char[length+1];
	if (text == '\0')
	{
		fclose(file);
		return SH_L_NOT_ENOUGH_MEMORY;
	}
	memset(text, 0, sizeof(char)*(length+1));
	fseek(file, 0, SEEK_SET);
	if (fread(text, 1, length, file) != length)
	{
		delete[] text;
		cout << "Error reading from file \"" << filename << "\"" << endl;
		return SH_L_FAIL;
	}
	fclose(file);
	text[length] = '\0';
	int res = shLLexText(text);
	delete[] text;
	if (res == SH_L_SUCCESS)
	{
#ifdef _WIN32
		int filename_length = strlen(filename);
		if ((filename_length > 1 && filename[1] == ':') || filename[0] == '\\')
#else
		if (filename[0] == '/')         // absolute path
#endif
			p_filename = filename;
		else
		{
			char *currentPath = new char[PATH_MAX+1];
			if (currentPath == '\0')
				return SH_L_NOT_ENOUGH_MEMORY;
			if (!getCurrentDir(currentPath, PATH_MAX))
				p_filename = "";		// if couldn't retrieve the path, just ignore the path
			else
				p_filename = FIX_PATH(currentPath);
			delete[] currentPath;
			int pathlen = p_filename.length();
			if (pathlen > 0 && p_filename[pathlen-1] != '/' && p_filename[pathlen-1] != '\\')
#ifdef _WIN32
				p_filename += '\\';
#else
				p_filename += '/';
#endif
			p_filename += filename;
		}
	}
	return res;
}

int shLexer::shLLexText(const char *text)
{
	shLFreeText();		// If lexer was previously working on a text, free it!
	unsigned int length = 0;
	unsigned int convertedLength = 0;
	for (length = 0; text[length]; ++length)
	{
		if (text[length] < 0 || text[length] > 127)	// then file is not ascii
			return SH_L_FAIL;
		++convertedLength;
		if ((text[length] == '\n' && text[length+1] == '\r') || (text[length] == '\r' && text[length+1] == '\n'))
			++length;
	}
	p_text2lex = new char[convertedLength+1];
	if (p_text2lex == '\0')
		return SH_L_NOT_ENOUGH_MEMORY;
	// convert all types of line endings to '\n'
	for (unsigned int i = 0, j = 0; i < length; ++i, ++j)
	{
		p_text2lex[j] = (text[i] == '\r')?'\n':text[i];
		if ((text[i] == '\n' && text[i+1] == '\r') || (text[i] == '\r' && text[i+1] == '\n'))
			++i;
	}
	p_text2lex[convertedLength] = '\0';
#ifdef SH_DEBUG
	fwrite(p_text2lex, 1, length, stdout);
#endif
	return SH_L_SUCCESS;
}

string shLexer::shLNextToken(string &tokenType)
{
	p_spaceMet = false;
	tokenType = "";
	p_lastTokenType = p_currentTokenType;
	p_currentTokenType = "";
	p_lastToken = p_token;
	p_token = "";
	if (p_finished)
		return "";
	if (p_peeked.size() == 0)
		shLPeekNextToken(tokenType);
	p_line = p_peeked.front().p_lineAfterPeek;
	p_column = p_peeked.front().p_columnAfterPeek;
	p_lexemeBeginning = p_peeked.front().p_lexemeBeginningAfterPeek;
	p_spaceMet = p_peeked.front().p_spaceMetAfterPeek;
	p_finished = p_peeked.front().p_finishedAfterPeek;
	p_token = p_peeked.front().p_peekedToken;
	p_currentTokenType = tokenType = p_peeked.front().p_peekedTokenType;
	p_peeked.pop_front();
	return p_token;
}

string shLexer::shLPeekNextToken(string &tokenType, unsigned int n, int *line, int *column)
{
	tokenType = "";
	if (p_finished)
	{
		if (line)
			*line = p_line;
		if (column)
			*column = p_column;
		return "";
	}
	while (n >= p_peeked.size())
	{
		string possibleTokenType = "";
		string peekedToken = "";
		string peekedTokenType = "";
		int lineAfterPeek;
		int columnAfterPeek;
		bool spaceMetAfterPeek = false;
		bool finishedAfterPeek = false;
		int lexemeBeginningAfterPeek;
		if (p_peeked.size() > 0)
		{
			lineAfterPeek = p_peeked.back().p_lineAfterPeek;
			columnAfterPeek = p_peeked.back().p_columnAfterPeek;
			lexemeBeginningAfterPeek = p_peeked.back().p_lexemeBeginningAfterPeek;
		}
		else
		{
			lineAfterPeek = p_line;
			columnAfterPeek = p_column;
			lexemeBeginningAfterPeek = p_lexemeBeginning;
		}
		int currentLine;
		int currentColumn;
		int lineInLastMatch = lineAfterPeek;
		int columnInLastMatch = columnAfterPeek;
		int forward;
		int lastMatchPos;
		int *posInDFA = new int[p_typeDFAs.size()];
		do
		{
			currentLine = lineAfterPeek;
			currentColumn = columnAfterPeek+1;
			forward = lexemeBeginningAfterPeek;
			lastMatchPos = -1;							// If remains -1, then could not match anything!
			if (p_text2lex[lexemeBeginningAfterPeek] == '\0')
			{
				delete[] posInDFA;
				finishedAfterPeek = true;
				p_peekAhead res = {peekedToken, peekedTokenType, spaceMetAfterPeek, lineAfterPeek,
					columnAfterPeek, lexemeBeginningAfterPeek, finishedAfterPeek};
				p_peeked.push_back(res);
				if (line)
				{
					if (p_peeked.size() > 0)
						*line = p_peeked.back().p_lineAfterPeek;
					else
						*line = p_line;
				}
				if (column)
				{
					if (p_peeked.size() > 0)
						*column = p_peeked.back().p_columnAfterPeek;
					else
						*column = p_column;
				}
				return "";
			}
			peekedToken = "";
			int i = 0;
			for (list<shDFA>::const_iterator iter = p_typeDFAs.begin(), end = p_typeDFAs.end(); iter != end; ++iter, ++i)
				posInDFA[i] = iter->start;
			for (; p_text2lex[forward] != '\0'; ++forward)
			{
//				cout << "Checking for @" << forward << " = " << p_text2lex[forward] << endl;
				bool canContinue = false;
				i = 0;
				for (list<shDFA>::const_iterator iter = p_typeDFAs.begin(), end = p_typeDFAs.end(); iter != end; ++iter, ++i)
					// If not in trap state and there is a letter associated with this character in this dfa
					if (posInDFA[i] != iter->trapState)
					{
						if (iter->inLetter[(int)p_text2lex[forward]] != -1)
						{
							posInDFA[i] = iter->to[posInDFA[i]][iter->inLetter[(int)p_text2lex[forward]]];
							if (iter->final[posInDFA[i]])
							{
								possibleTokenType = iter->name;
								lastMatchPos = forward;
								lineInLastMatch = currentLine;
								columnInLastMatch = currentColumn;
								if (p_text2lex[forward] == '\n')
								{
									++lineInLastMatch;
									columnInLastMatch = 0;
								}
							}
							if (posInDFA[i] != iter->trapState)
								canContinue = true;
#ifdef SH_DEBUG
							cout << "Going forwards in dfa: " << iter->name << " with character: " <<
								p_text2lex[forward] << endl;
#endif
						}
						else
						{
#ifdef SH_DEBUG
							cout << "Undefined character: " << p_text2lex[forward] << " for dfa: " << iter->name << endl;
#endif
							posInDFA[i] = iter->trapState;
						}
					}
#ifdef SH_DEBUG
					else
					{
						cout << "Cannot go any further in dfa: " << iter->name << " with character: " << p_text2lex[forward];
						if (iter->inLetter[p_text2lex[forward]] != -1)
							cout << " which is part of letter: " << iter->alphabet[iter->inLetter[p_text2lex[forward]]].name;
						cout << endl;
						char c;
						cin >> c;
					}
#endif
				if (!canContinue)
					break;
				if (p_text2lex[forward] == '\n')
				{
					++currentLine;
					currentColumn = 0;
				}
				++currentColumn;
			}
			if (lastMatchPos == -1) // Could not match even one token!
			{
				// call error handler
				++columnAfterPeek;
				(p_noTokenMatch)(p_text2lex, &lexemeBeginningAfterPeek, &forward, p_filename.c_str(), &lineAfterPeek, &columnAfterPeek);
				lineInLastMatch = lineAfterPeek;
				columnInLastMatch = columnAfterPeek;
				lastMatchPos = lexemeBeginningAfterPeek;
				possibleTokenType = "WHITE_SPACE";
			}
			lineAfterPeek = lineInLastMatch;
			columnAfterPeek = columnInLastMatch;
			if (!p_ignoreWhiteSpace || possibleTokenType != "WHITE_SPACE")
			{
				char t = p_text2lex[lastMatchPos+1];
				p_text2lex[lastMatchPos+1] = '\0';
				peekedToken = p_text2lex+lexemeBeginningAfterPeek;
				p_text2lex[lastMatchPos+1] = t;
			}
			else
				spaceMetAfterPeek = true;
			lexemeBeginningAfterPeek = lastMatchPos+1;
		} while (p_ignoreWhiteSpace && possibleTokenType == "WHITE_SPACE");
		delete[] posInDFA;
		if (p_keywords.find(peekedToken) != p_keywords.end())
			tokenType = peekedToken;
		else
			tokenType = possibleTokenType;
		peekedTokenType = tokenType;
		(*p_functionToCall[tokenType])(peekedToken);
		p_peekAhead res = {peekedToken, peekedTokenType, spaceMetAfterPeek, lineAfterPeek, columnAfterPeek,
			lexemeBeginningAfterPeek, finishedAfterPeek};
		p_peeked.push_back(res);
	}
	list<p_peekAhead>::const_iterator i = p_peeked.begin();
	for (unsigned int j = 0; j < n; ++j)
		++i;
	tokenType = i->p_peekedTokenType;
	if (line)
		*line = i->p_lineAfterPeek;
	if (column)
		*column = i->p_columnAfterPeek;
	return i->p_peekedToken;
}

void shLexer::shLInsertFakeToken(const string &type, const char *value)
{
	p_lastTokenType = p_currentTokenType;
	p_currentTokenType = type;
	p_lastToken = p_token;
	if (value)
		p_token = value;
	else
	{
		list<shDFA>::const_iterator dfa, end;
		for (dfa = p_typeDFAs.begin(), end = p_typeDFAs.end(); dfa != end; ++dfa)
			if (dfa->name == type)
				break;
		if (dfa != end)
		{
			int state = dfa->start;
			vector<bool> visited(dfa->final.size(), false);
			visited[dfa->trapState] = true;		// make sure you don't fall in trap!
			p_token = "";
			bool foundAnotherState = true;
			while (!dfa->final[state] && foundAnotherState)		// if couldn't find another state, it is stuck in a loop. I made sure it won't go to trap, but better have a check here just in case
										// there was a bug elsewhere or the cache was tampered with
			{
				foundAnotherState = false;
				visited[state] = true;
				for (int i = 0; i < 256; ++i)
					if (dfa->inLetter[i] != -1)
					{
						int to = dfa->to[state][dfa->inLetter[i]];
						if (dfa->final[to] || !visited[to])
						{
							p_token += (char)i;
							state = to;
							foundAnotherState = true;
							break;
						}
					}
			}
		}
		else			// must be keyword or a bug!
			p_token = type;
	}
}

string shLexer::shLCurrentToken()
{
	return p_token;
}

string shLexer::shLLastToken()
{
	return p_lastToken;
}

string shLexer::shLCurrentTokenType()
{
	return p_currentTokenType;
}

string shLexer::shLLastTokenType()
{
	return p_lastTokenType;
}

void shLexer::shLAddDFA(const shDFA &dfa)
{
	p_typeDFAs.push_back(dfa);
	p_functionToCall[dfa.name] = _emptyMatchFunction;
}

int shLexer::shLReadKeywords(istream &kin)
{
	string s;
	while (kin >> s)
	{
		p_keywords.insert(s);
		p_functionToCall[s] = _emptyMatchFunction;
	}
	return SH_L_SUCCESS;
}

void shLexer::shLAddMatchFunction(const string &token, shLTokenMatchFunction func)
{
	p_functionToCall[token] = func;
}

void shLexer::shLAddNoMatchErrorFunction(shLErrorHandler eh)
{
	p_noTokenMatch = eh;
}

bool shLexer::shLSpaceMet()
{
	return p_spaceMet;
}

void shLexer::shLIgnoreWhiteSpace(bool ignore)
{
	p_ignoreWhiteSpace = ignore;
	p_peeked.clear();			// The next tokens could be changed, so make the lexer read them again
}

bool shLexer::shLIgnoresWhiteSpace()
{
	return p_ignoreWhiteSpace;
}

int shLexer::shLLine()
{
	return p_line;
}

int shLexer::shLColumn()
{
	return p_column;
}

string shLexer::shLFileName()
{
	return p_filename;
}

void shLexer::shLDiscardKeywords()
{
	p_keywords.clear();
}

void shLexer::shLFreeDFAs()
{
	for (list<shDFA>::iterator iter = p_typeDFAs.begin(), end = p_typeDFAs.end(); iter != end; ++iter)
	{
		iter->final.clear();
		iter->to.clear();
	}
	p_typeDFAs.clear();
}
