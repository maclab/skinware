/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of DocThis!.
 *
 * DocThis! is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DocThis! is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DocThis!.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SEMANTICS_H_BY_CYCLOPS
#define SEMANTICS_H_BY_CYCLOPS

#include <shcompiler.h>
#include <list>
#include <string>

using namespace std;

struct formattedText
{
	string html;
	string tex;
	string plain;		// This could be used for anchors and labels
	formattedText operator +(const formattedText &rhs)
	{
		formattedText res = *this;
		res.html += rhs.html;
		res.tex += rhs.tex;
		res.plain += rhs.plain;
		return res;
	}
	formattedText &operator +=(const formattedText &rhs)
	{
		html += rhs.html;
		tex += rhs.tex;
		plain += rhs.plain;
		return *this;
	}
};

struct docIdentifier
{
	formattedText name;
	string anchor;
};

enum docTypeEnum
{
	INVALID = 0,		// This file was not a valid DocThis! file
	INDEX,
	CLASS,
	STRUCT,
	FUNCTIONS,
	VARIABLES,
	API,
	CONSTANTS,
	IOFILE,
	EXAMPLE,
	OTHER,
	GLOBALS
};

struct docGlobalItem
{
	string filename;
	int line, column;	// line and column the filename was read
};

void stopEvaluation();
void resetSemantics();
void outputFormats(bool html, bool tex, bool summary, bool details);	// true or false for either to set which output would be generated
int readNotices(const string &noticeFile);
bool noError();
formattedText		docName();
formattedText		docTitle();
string			docAuthor();
docTypeEnum		docType();
string			docVersion();
list<string>		docKeywords();
list<docGlobalItem>	docGlobalsList();
void			docGlobals(list<docIdentifier> &macros, list<docIdentifier> &types, list<docIdentifier> &globalTypes,
				list<docIdentifier> &variables, list<docIdentifier> &globalVariables,
				list<docIdentifier> &functions, list<docIdentifier> &globalFunctions);
formattedText		docHeader();
formattedText		docBottom();

void docType(shLexer &lexer, shParser &parser);
void setDocName(shLexer &lexer, shParser &parser);
void fieldType(shLexer &lexer, shParser &parser);
void setNextPrevType(shLexer &lexer, shParser &parser);
void fieldValue(shLexer &lexer, shParser &parser);
void seealsoValue(shLexer &lexer, shParser &parser);
void pop(shLexer &lexer, shParser &parser);
void push(shLexer &lexer, shParser &parser);
void pushEmpty(shLexer &lexer, shParser &parser);
void pushCodeWord(shLexer &lexer, shParser &parser);
void pushLinkKeyword(shLexer &lexer, shParser &parser);
void pushCode(shLexer &lexer, shParser &parser);
void pushEmphasize(shLexer &lexer, shParser &parser);
void pushStrong(shLexer &lexer, shParser &parser);
void pushHTML(shLexer &lexer, shParser &parser);
void pushAnchor(shLexer &lexer, shParser &parser);
void pushNoLink(shLexer &lexer, shParser &parser);
void pushYesLink(shLexer &lexer, shParser &parser);
void pushInline(shLexer &lexer, shParser &parser);
void pushIndent(shLexer &lexer, shParser &parser);
void pushNewLine(shLexer &lexer, shParser &parser);
void memberOrGlobal(shLexer &lexer, shParser &parser);
void pushMacroDeclaration(shLexer &lexer, shParser &parser);
void pushVariableDeclaration(shLexer &lexer, shParser &parser);
void pushTypeDeclaration(shLexer &lexer, shParser &parser);
void pushFunctionDeclaration(shLexer &lexer, shParser &parser);
void pushInputDeclaration(shLexer &lexer, shParser &parser);
void pushOutputDeclaration(shLexer &lexer, shParser &parser);
void pushConstGroupDeclaration(shLexer &lexer, shParser &parser);
void pushConstDeclaration(shLexer &lexer, shParser &parser);
void pushImportance(shLexer &lexer, shParser &parser);
void pushMightHaveSpace(shLexer &lexer, shParser &parser);
void pushArg(shLexer &lexer, shParser &parser);
void pushHasValue(shLexer &lexer, shParser &parser);
void pushDefaultValue(shLexer &lexer, shParser &parser);
void pushParagraphAndItems(shLexer &lexer, shParser &parser);
void concat(shLexer &lexer, shParser &parser);
void concatKeepsSpace(shLexer &lexer, shParser &parser);
void concatIgnoresSpace(shLexer &lexer, shParser &parser);
void concatStoreState(shLexer &lexer, shParser &parser);
void concatRestoreState(shLexer &lexer, shParser &parser);
void concatDefaultValue(shLexer &lexer, shParser &parser);
void concatParagraphAndItems(shLexer &lexer, shParser &parser);
void inLink(shLexer &lexer, shParser &parser);
void notInLink(shLexer &lexer, shParser &parser);
void closeItemsUntil(shLexer &lexer, shParser &parser);
void headingBegin(shLexer &lexer, shParser &parser);
void headingEnd(shLexer &lexer, shParser &parser);
void itemBegin(shLexer &lexer, shParser &parser);
void itemEnd(shLexer &lexer, shParser &parser);
void paragraphEnd(shLexer &lexer, shParser &parser);
void codeBlockBegin(shLexer &lexer, shParser &parser);
void codeBlockEnd(shLexer &lexer, shParser &parser);
void hrEnd(shLexer &lexer, shParser &parser);
void importantCodeEnd(shLexer &lexer, shParser &parser);
void definitionBegin(shLexer &lexer, shParser &parser);
void definitionEnd(shLexer &lexer, shParser &parser);
void makeLink(shLexer &lexer, shParser &parser);
void readWhiteSpace(shLexer &lexer, shParser &parser);
void ignoreWhiteSpace(shLexer &lexer, shParser &parser);
void setOverview(shLexer &lexer, shParser &parser);
void setElementExplanation(shLexer &lexer, shParser &parser);
void setNotice(shLexer &lexer, shParser &parser);
void setShortExplanation(shLexer &lexer, shParser &parser);
void checkCompleteness(shLexer &lexer, shParser &parser);
void increaseIndent1(shLexer &lexer, shParser &parser);
void decreaseIndent1(shLexer &lexer, shParser &parser);
void increaseIndent2(shLexer &lexer, shParser &parser);
void decreaseIndent2(shLexer &lexer, shParser &parser);
void increaseIndent3(shLexer &lexer, shParser &parser);
void decreaseIndent3(shLexer &lexer, shParser &parser);
void increaseIndent4(shLexer &lexer, shParser &parser);
void decreaseIndent4(shLexer &lexer, shParser &parser);
void increaseIndent5(shLexer &lexer, shParser &parser);
void decreaseIndent5(shLexer &lexer, shParser &parser);
void addToGlobalList(shLexer &lexer, shParser &parser);
void documentHeader(shLexer &lexer, shParser &parser);
void documentBottom(shLexer &lexer, shParser &parser);
void writeDocument(shLexer &lexer, shParser &parser);

#endif
