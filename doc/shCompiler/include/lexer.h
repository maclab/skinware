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

#ifndef LEXER_H_BY_CYCLOPS
#define LEXER_H_BY_CYCLOPS

#include <string>
#include <cstdio>
#include <map>
#include <iostream>
#include <dfa.h>

using namespace std;

#define SH_L_SUCCESS 0
#define SH_L_FILE_NOT_FOUND -1
#define SH_L_NOT_ENOUGH_MEMORY -2
#define SH_L_FAIL -3

class shParser;

class shLexer
{
public:
	typedef void (*shLTokenMatchFunction)(string &);		// An shLTokenMatchFunction is called whenever a token is matched.
									// Argument is the token matched!
									// If no action is set, an empty function will be called!
	typedef void (*shLErrorHandler)(char *, int *, int *, const char *, int *, int *);
									// Arguments are file content, lexeme beginning (up to where input was fine),
									// where the error occured (all DFAs went in trap), filename, line, column
protected:
	bool p_initialized;
	list<shDFA> p_typeDFAs;
	set<string> p_keywords;
	string p_filename;
	char *p_text2lex;
	int p_lexemeBeginning;
	string p_token;
	string p_lastToken;
	string p_currentTokenType;
	string p_lastTokenType;
	bool p_spaceMet;
	int p_line, p_column;
	bool p_ignoreWhiteSpace;
	bool p_finished;
	struct p_peekAhead
	{
		string p_peekedToken;
		string p_peekedTokenType;
		bool p_spaceMetAfterPeek;
		int p_lineAfterPeek;
		int p_columnAfterPeek;
		int p_lexemeBeginningAfterPeek;
		bool p_finishedAfterPeek;
	};
	list<p_peekAhead> p_peeked;					// If peeked, they would be cached here so that next peeks/nextTokens
									// would not redo. This cache is emptied if shLIgnoreWhiteSpace is called
	map<string, shLTokenMatchFunction> p_functionToCall;
	shLErrorHandler p_noTokenMatch;					// If token is matched, this function is called which can skip parts
									// of the input (update the line number in the process) so that the lex
									// process could be continued
	void shLFreeText();
public:
	// returns SH_L_SUCCESS on success, SH_L_FILE_NOT_FOUND if any file could not be opened.
	// This is a wrapper for shLAddDFAs and shLReadKeywords
	// Note: this function is not reentrant!
	int shLInitialize(const char *lettersFile, const char *tokensFile, const char *keywordsFile, bool rebuild = true);

	// returns SH_L_SUCCESS on successful openning and SH_L_FILE_NOT_FOUND if could not open.
	// It would return SH_L_NOT_ENOUGH_MEMORY in case memory could not be allocated
	int shLOpenFile(const char *filename);

	// returns either SH_L_SUCCESS or SH_L_NOT_ENOUGH_MEMORY
	// Takes a copy of the array, so it can be deleted or changed
	int shLLexText(const char *text);

	// returns the next token from the input.
	// WHITE_SPACE tokens are ignored and will not be returned (unless shLIgnoreWhiteSpace(false) is called)
	// if no tokens are left, returns ""
	// returns the type of token matched in string `type`!
	string shLNextToken(string &type);

	// returns the `n`th next token from the input (start from 0), without actually going forward in the input.
	// That is, calling this function twice with the same n, returns the same thing!
	// WHITE_SPACE tokens are ignored and will not be returned (unless shLIgnoreWhiteSpace(false) is called)
	// if no tokens are left, returns ""
	// returns the type of token matched in string `type`!
	// returns the line the token was matched in `line`, if it is not NULL
	// returns the column in the line the token was matched in `column`, it is not NULL
	string shLPeekNextToken(string &type, unsigned int n = 0, int *line = '\0', int *column = '\0');

	// acts as if value was the current token, if value is NULL, generates a token of given type
	// this is useful for error handling where a missing token can be inserted as if it is correctly read
	// the function generates a valid token of this type, so the semantic analyzers don't get confused if
	// reading the fake token
	void shLInsertFakeToken(const string &type, const char *value = '\0');

	// return current and last matched token
	string shLCurrentToken();
	string shLLastToken();

	// return current and last matched token's type
	string shLCurrentTokenType();
	string shLLastTokenType();

	// add a dfa to possible tokens that can be matched
	void shLAddDFA(const shDFA &dfa);

	// returns SH_L_SUCCESS on success and SH_L_FILE_NOT_FOUND if file could not be found
	int shLReadKeywords(istream &keywords);

	// Call a certain function if a token is matched. This is useful for example to change two consecutive
	// characters '\\' and 'n' in an input string to a '\n' in the token.
	void shLAddMatchFunction(const string &token, shLTokenMatchFunction func);

	// Call this function if no token could be matched in the input
	// By default the erroneous part of the input is ignored, and a message printed
	void shLAddNoMatchErrorFunction(shLErrorHandler eh);

	// Returns whether during last shLNextToken call a WHITE_SPACE has been matched (and ignored)
	bool shLSpaceMet();

	// Tells the lexer whether to ignore WHITE_SPACE or not. It will ignore by default
	void shLIgnoreWhiteSpace(bool ignore = true);

	// Tells whether the lexer is ignoring WHITE_SPACE or not.
	bool shLIgnoresWhiteSpace();

	// Returns the current line in the text
	int shLLine();

	// Returns the current column in the line
	int shLColumn();

	// Returns the current file name
	string shLFileName();

	// Free the memory of the keywords
	void shLDiscardKeywords();

	// Free the memory of the DFAs
	void shLFreeDFAs();

	shLexer();

	~shLexer()
	{
		shLFreeText();
		shLFreeDFAs();
		shLDiscardKeywords();
	}

private:
	void shLInitToTokensFile();
	void shLInitToGrammarFile();

	friend class shParser;
};

#endif
