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

#include <list>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <set>
#include <map>
#include "semantics.h"
#include "specials.h"
#include "error_print.h"

using namespace std;

//#define SH_DEBUG

static bool _makeHtml = false;
static bool _makeTex = false;
static bool _makeSummary = true;
static bool _makeDetails = true;
static map<string, string> _notices;

enum metaEnum
{
	NO_META = 0,
	NEXT = 1,
	PREVIOUS = 2,
	VERSION = 3,
	AUTHOR = 4,
	SHORTCUT = 5,
	KEYWORD = 6,
	SEEALSO = 7,
	CUSTOM = 8,
	IMAGE = 9,
	PDF = 10,
	PS = 11,
	EPS = 12,
	SVG = 13,
	OBJECT = 14,
	ANCHOR = 15,
	NOLINK = 16,
	YESLINK = 17,
	INLINE = 18,
	INDENT = 19,
	HASVALUE = 20,
	ARG = 21,
	MACRODECL = 22,
	VARIABLEDECL = 23,
	TYPEDECL = 24,
	FUNCTIONDECL = 25,
	INPUTDECL = 26,
	OUTPUTDECL = 27,
	CONSTGROUPDECL = 28,
	CONSTDECL = 29,
	DEFAULTVALUE = 30,
	PARAGRAPHANDITEMS = 31,
	IMPORTANCE = 32,
	KEEPSPACE = 33
};
struct stackElement
{
	formattedText text;
	metaEnum meta;
	int extra;
};

static bool _evaluate = true;
static stack<stackElement> semantic_stack;
static list<int> item_level_stack;
docTypeEnum _docType;
static formattedText _docName;
static formattedText _docTitle;
static formattedText _next;
static formattedText _previous;
static string _version;
static string _author;
static list<string> _shortcuts;
static list<string> _keywords;
static list<string> _seealsos;
static list<formattedText> _objectsToAppend;
static set<string> _objectsAlreadySeen;
static int _figureNum = 1;
static int _figureUniqueId = 0;
static int _indentLevel = 2;
static bool _declIsGlobal = false;
static bool _concatKeepsSpace = true;
static bool _enableIndent = true;
static bool _itemIndented = false;
static bool _duringHeader = true;
static bool _duringDefinition = false;
static bool _codeOpened = false;
static bool _emphasizeOpened = false;
static bool _strongOpened = false;
static bool _duringHeading = false;
static bool _duringSeeAlso = false;
static bool _inLink = false;
static bool _duringCodeBlock = false;
static int _codeBlockIndent = 0;
struct functionArgument
{
	formattedText name;
	formattedText type;
	formattedText defaultValue;
	formattedText text;		// explanation on the argument
};
struct docElement
{
	formattedText text;
	formattedText summary;
	formattedText name;
	formattedText type;		// could be type, name for output or defined value. Basically, what comes after :
					// a better name wouldn't be bad though!
	string anchorName;
	string notice;			// if it has a notice, its notice name
	list<functionArgument> args;
	formattedText output;		// explanation on the output of function/macro
	bool hasArgs;			// if a function or macro, then true. If a variable, constant or type, then false.
	bool hasType;			// if a macro, doesn't have type or default value. If other it does
};
struct constantGroup
{
	formattedText text;
	formattedText summary;
	formattedText name;
	string anchorName;
	string notice;			// if it has a notice, its notice name
	list<docElement> constants;
};
// Elements of the documentation. They can't be kept on the stack because they need to be sorted out.
static list<docElement> _docMacros,
			_docVariables,
			_docGlobalVariables,
			_docTypes,
			_docGlobalTypes,
			_docFunctions,
			_docGlobalFunctions;
static list<constantGroup> _docConstants;
static list<docGlobalItem> _globalsList;
static formattedText _docHeader;
static formattedText _docBottom;
static formattedText _docOverview;

#define INDENT(x) \
	({\
		string p_indent = "";\
	 	if (_enableIndent)\
		 	for (int p_i = 0; p_i < _indentLevel; ++p_i)\
				p_indent += '\t';\
		p_indent+(x);\
	})

#define SUBMESSAGE_INDENT "        "

#define WARNING(x, ...)		pprintMessage(parser,					lexer,	"warning: "x, ##__VA_ARGS__)
#define ERROR(x, ...)		pprintMessage(((_evaluate = false)?parser:parser),	lexer,	"error: "x, ##__VA_ARGS__)
#define INTERNAL(x, ...) \
	do {\
				pprintMessage(((_evaluate = false)?parser:parser),	lexer,	"internal error: "x, ##__VA_ARGS__);\
				printMessage(lexer.shLLine(), lexer.shLColumn(),		"internal error: "x, ##__VA_ARGS__);\
				fflush(stdout);\
	} while (0)
#define SUBMESSAGE(x, ...)	pprintMessage(parser,					lexer,	SUBMESSAGE_INDENT x, ##__VA_ARGS__)

#ifdef SH_DEBUG
#define DEBUGMESSAGE(x, ...)	printMessage(lexer.shLLine(), lexer.shLColumn(), "Debug: "x, ##__VA_ARGS__)
#define DEBUGSUBMESSAGE(x, ...)	printMessage(lexer.shLLine(), lexer.shLColumn(), SUBMESSAGE_INDENT x, ##__VA_ARGS__)
#define STACK_POP \
	do {\
		DEBUGMESSAGE("Function %s: Read from stack: meta(%s)(value: %d)", __func__,\
				metaString(semantic_stack.top().meta, lexer, parser).c_str(), semantic_stack.top().extra);\
		DEBUGSUBMESSAGE("HTML text: %s", semantic_stack.top().text.html.c_str());\
		DEBUGSUBMESSAGE("TeX text: %s", semantic_stack.top().text.tex.c_str());\
		DEBUGSUBMESSAGE("Plain text: %s", semantic_stack.top().text.plain.c_str());\
		fflush(stdout);\
		semantic_stack.pop();\
	} while (0)
#define STACK_PUSH(x) \
	do {\
		DEBUGMESSAGE("Function %s: Writing to stack: meta(%s)(value: %d)", __func__,\
				metaString((x).meta, lexer, parser).c_str(), (x).extra);\
		DEBUGSUBMESSAGE("HTML text: %s", (x).text.html.c_str());\
		DEBUGSUBMESSAGE("TeX text: %s", (x).text.tex.c_str());\
		DEBUGSUBMESSAGE("Plain text: %s", (x).text.plain.c_str());\
		fflush(stdout);\
		semantic_stack.push(x);\
	} while (0)
#else
#define STACK_POP	semantic_stack.pop()
#define STACK_PUSH(x)	semantic_stack.push(x)
#endif

#define CLOSE_ITEMS \
do {\
	if (!item_level_stack.empty())\
	{\
		stackElement p_elem2;\
		while (!item_level_stack.empty())\
		{\
			p_elem2.text.html += INDENT("</li>\n");\
			--_indentLevel;\
			if (item_level_stack.back() <= 3)\
			{\
				p_elem2.text.html += INDENT("</ul>\n");\
				p_elem2.text.tex += "\\end{itemize}\n";\
			}\
			else\
			{\
				p_elem2.text.html += INDENT("</ol>\n");\
				p_elem2.text.tex += "\\end{enumerate}\n";\
			}\
			item_level_stack.pop_back();\
		}\
		p_elem2.text.tex += '\n';\
		p_elem2.meta = NO_META;\
		STACK_PUSH(p_elem2);\
		concat(lexer, parser);\
	}\
} while (0)

void stopEvaluation()
{
	_evaluate = false;
}

void outputFormats(bool html, bool tex, bool summary, bool details)
{
	_makeHtml = html;
	_makeTex = tex;
	_makeSummary = summary;
	_makeDetails = details;
}

static string trimNewLines(const string &s);

int readNotices(const string &noticeFile)
{
	ifstream fin(noticeFile.c_str());
	if (!fin.is_open())
		return -1;

	string line;
	string id;
	string text;
	bool idInNextLine = false;
	while (getline(fin, line))
	{
		istringstream sin(line);
		string temp;
		if (!(sin >> temp))
		{
			text += "\n";
			continue;
		}
		if (temp == "NOTICE" || temp == "NOTE")
		{
			if (id != "")
				_notices[id] = trimNewLines(text);
			id = "";
			text = "";
			idInNextLine = !(sin >> id);
			if (getline(sin, text))
				text += "\n";
			continue;
		}
		if (idInNextLine)
		{
			id = temp;
			idInNextLine = false;
		}
		else
			text += temp + " ";
		if (getline(sin, line))
			text += line + "\n";
	}
	if (id != "")
		_notices[id] = trimNewLines(text);
	return 0;
}

void resetSemantics()
{
	_evaluate = true;
	while (!semantic_stack.empty())
		semantic_stack.pop();
	_docName.html = "";
	_docName.tex = "";
	_docName.plain = "";
	_next.html = "";
	_next.tex = "";
	_next.plain = "";
	_previous.html = "";
	_previous.tex = "";
	_previous.plain = "";
	_version = "";
	_author = "";
	_docType = INVALID;
	_indentLevel = 2;
	_declIsGlobal = false;
	_concatKeepsSpace = true;
	_enableIndent = true;
	_itemIndented = false;
	_makeHtml = false;
	_makeTex = false;
	_duringHeader = true;
	_duringDefinition = false;
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
	_duringHeading = false;
	_duringSeeAlso = false;
	_duringCodeBlock = false;
	_codeBlockIndent = 0;
	_inLink = false;
	_shortcuts.clear();
	_keywords.clear();
	_seealsos.clear();
	_objectsToAppend.clear();
	_objectsAlreadySeen.clear();
	_figureNum = 1;
	_figureUniqueId = 0;
	_docConstants.clear();
	_docMacros.clear();
	_docVariables.clear();
	_docGlobalVariables.clear();
	_docTypes.clear();
	_docGlobalTypes.clear();
	_docFunctions.clear();
	_docGlobalFunctions.clear();
	_globalsList.clear();
	_notices.clear();
}

bool noError()
{
	return _evaluate;
}

formattedText docName()
{
	return _docName;
}

formattedText docTitle()
{
	return _docTitle;
}

string docAuthor()
{
	return _author;
}

docTypeEnum docType()
{
	return _docType;
}

string docVersion()
{
	return _version;
}

list<string> docKeywords()
{
	return _keywords;
}

list<docGlobalItem> docGlobalsList()
{
	return _globalsList;
}

void docGlobals(list<docIdentifier> &macros, list<docIdentifier> &types, list<docIdentifier> &globalTypes,
				list<docIdentifier> &variables, list<docIdentifier> &globalVariables,
				list<docIdentifier> &functions, list<docIdentifier> &globalFunctions)
{
	macros.clear();
	types.clear();
	globalTypes.clear();
	variables.clear();
	globalVariables.clear();
	functions.clear();
	globalFunctions.clear();
	for (list<docElement>::const_iterator i = _docMacros.begin(), end = _docMacros.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		macros.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docTypes.begin(), end = _docTypes.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		types.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docGlobalTypes.begin(), end = _docGlobalTypes.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		globalTypes.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docVariables.begin(), end = _docVariables.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		variables.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docGlobalVariables.begin(), end = _docGlobalVariables.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		globalVariables.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docFunctions.begin(), end = _docFunctions.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		functions.push_back(id);
	}
	for (list<docElement>::const_iterator i = _docGlobalFunctions.begin(), end = _docGlobalFunctions.end(); i != end; ++i)
	{
		docIdentifier id;
		id.name = i->name;
		id.anchor = i->anchorName;
		globalFunctions.push_back(id);
	}
}

formattedText docHeader()
{
	return _docHeader;
}

formattedText docBottom()
{
	return _docBottom;
}

static string metaString(metaEnum m, shLexer &lexer, shParser &parser)
{
	switch (m)
	{
	case NO_META:
		return string("NO_META");
	case NEXT:
		return string("NEXT");
	case PREVIOUS:
		return string("PREVIOUS");
	case VERSION:
		return string("VERSION");
	case AUTHOR:
		return string("AUTHOR");
	case SHORTCUT:
		return string("SHORTCUT");
	case KEYWORD:
		return string("KEYWORD");
	case SEEALSO:
		return string("SEEALSO");
	case CUSTOM:
		return string("CUSTOM");
	case IMAGE:
		return string("IMAGE");
	case PDF:
		return string("PDF");
	case PS:
		return string("PS");
	case EPS:
		return string("EPS");
	case SVG:
		return string("SVG");
	case OBJECT:
		return string("OBJECT");
	case ANCHOR:
		return string("ANCHOR");
	case NOLINK:
		return string("NOLINK");
	case YESLINK:
		return string("YESLINK");
	case INLINE:
		return string("INLINE");
	case INDENT:
		return string("INDENT");
	case HASVALUE:
		return string("HASVALUE");
	case ARG:
		return string("ARG");
	case MACRODECL:
		return string("MACRODECL");
	case VARIABLEDECL:
		return string("VARIABLEDECL");
	case TYPEDECL:
		return string("TYPEDECL");
	case FUNCTIONDECL:
		return string("FUNCTIONDECL");
	case INPUTDECL:
		return string("INPUTDECL");
	case OUTPUTDECL:
		return string("OUTPUTDECL");
	case CONSTGROUPDECL:
		return string("CONSTGROUPDECL");
	case CONSTDECL:
		return string("CONSTDECL");
	case DEFAULTVALUE:
		return string("DEFAULTVALUE");
	case PARAGRAPHANDITEMS:
		return string("PARAGRAPHANDITEMS");
	case IMPORTANCE:
		return string("IMPORTANCE");
	case KEEPSPACE:
		return string("KEEPSPACE");
	default:
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to print value of non-existing meta: %d", (int)m);
		return string("NO_META");
	}
}

string removeSpace(const string &s)
{
	string res;
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
			res += s[i];
	return res;
}

static string fixHtml(const string &s, shLexer &lexer, shParser &parser, bool codeWord = false)
{
	string result;
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (s[i] == '\n')
			result += "\n"+INDENT("");
		else if (s[i] == '<')
			result += "&lt;";
		else if (s[i] == '>')
			result += "&gt;";
		else if (s[i] == '&' && codeWord)
			result += "&amp;";
		else if (s[i] == '\\')
			if (i+1 == size)
			{
				if (_docType != INVALID && (_makeHtml || _makeTex))
				{
					WARNING("Lone \\ appeared at the end of token: %s", s.c_str());
					SUBMESSAGE("Is that the last word in the file?");
					SUBMESSAGE("If so, please use an editor that automatically appends the text by a new-line, or manually do so");
				}
				result += '\\';
			}
			else
			{
				++i;
				switch (s[i])
				{
				case ' ':
					result += "&nbsp;";
					break;
				case '\n':
					result += "<br />\n"+INDENT("");
					break;
				case '\t':
				case '(':
				case ')':
				case '\\':
				case '`':
				case '*':
				case '_':
				case '[':
				case ']':
				case '#':
				case '-':
				case '+':
				case '.':
				case ':':
					result += s[i];
					break;
				case '&':
					result += "&amp;";
					break;
				case '<':
					result += "&lt;";
					break;
				case '>':
					result += "&gt;";
					break;
				default:
					result += '\\';
					if (_docType != INVALID && (_makeHtml || _makeTex))
						WARNING("Character '%c' does not need to be escaped by \\", s[i]);
					--i;
					break;
				}
			}
		else result += s[i];
	return result;
}

static string fixTex(const string &s, shLexer &lexer, shParser &parser, bool codeWord = false)
{
	string result;
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (s[i] == '_') result += "\\_";
		else if (s[i] == '{') result += "\\{";
		else if (s[i] == '}') result += "\\}";
		else if (s[i] == '%') result += "\\%";
		else if (s[i] == '$') result += "\\$";
		else if (s[i] == '#') result += "\\#";
		else if (s[i] == '~') result += "\\textasciitilde{}";
		else if (s[i] == '^') result += "\\^{}";
//		else if (s[i] == '-') result += "{-}";
		else if (s[i] == '&')
		{
			if (!codeWord)	// then an HTML escape sequence
				if (i+6 < size && s[i+1] == 't' && s[i+2] == 'i' && s[i+3] == 'm' && s[i+4] == 'e' && s[i+5] == 's' && s[i+6] == ';')
				{
					result += "$\\times$";
					i += 6;
				}
				else if (i+5 < size && s[i+1] == 'l' && s[i+2] == 'a' && s[i+3] == 'r' && s[i+4] == 'r' && s[i+5] == ';')
				{
					result += "$\\leftarrow$";
					i += 5;
				}
				else if (i+5 < size && s[i+1] == 'r' && s[i+2] == 'a' && s[i+3] == 'r' && s[i+4] == 'r' && s[i+5] == ';')
				{
					result += "$\\rightarrow$";
					i += 5;
				}
				else
					result += "\\&";
			else
				result += "\\&";
		}
		else if (s[i] == '\\')
			if (i+1 == size)
			{
				if (_docType != INVALID && (_makeHtml || _makeTex)) { /* Warning already given when fixHtml was called */ }
				result += "\\backslash ";
			}
			else
			{
				++i;
				switch (s[i])
				{
				case ' ':
					result += '~';
					break;
				case '\n':
					result += "\\\\\n";
					break;
				case '\\':
					result += "\\textbackslash{}";
					break;
				case '_':
					result += "\\_";
					break;
				case '&':
					result += "\\&";
					break;
				case '#':
					result += "\\#";
					break;
				case '`':
					result += "\\`{}";
					break;
				case '\t':
				case '(':
				case ')':
				case '*':
				case '[':
				case ']':
				case '-':
				case '+':
				case '.':
				case '<':
				case '>':
				case ':':
					result += s[i];
					break;
				default:
					result += "\\textbackslash{}";
					if (_docType != INVALID && (_makeHtml || _makeTex)) { /* Warning already given when fixHtml was called */ }
					--i;
					break;
				}
			}
		else result += s[i];
	return result;
}

static string fixPlain(const string &s, shLexer &lexer, shParser &parser, bool codeWord = false)
{
	string result;
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (s[i] == '<')
		{
			for (++i; i < size && !(s[i] == '>' && s[i-1] != '\\'); ++i);
			continue;
		}
		else if (s[i] == '\\')
			if (i+1 == size)
			{
				if (_docType != INVALID && (_makeHtml || _makeTex)) { /* Warning already given when fixHtml was called */ }
				result += '\\';
			}
			else
			{
				++i;
				switch (s[i])
				{
				case ' ':
					result += " ";
					break;
				case '\n':
					result += "\n";
					break;
				case '\t':
				case '(':
				case ')':
				case '\\':
				case '`':
				case '*':
				case '_':
				case '[':
				case ']':
				case '-':
				case '+':
				case '.':
				case ':':
/*					result += s[i];		// it was, this here and the next for result += "".
					break;*/
				case '#':
				case '&':
				case '<':
				case '>':
					result += s[i];
					break;
				default:
					result += '\\';
					if (_docType != INVALID && (_makeHtml || _makeTex)) { /* Warning already given when fixHtml was called */ }
					--i;
					break;
				}
			}
		else result += s[i];
	return result;
}

string fixHtmlAnchor(const string &str)
{
	// According to the standard, ids in html have to start with a letter, colon or underscore, then followed by letters, colon, underscore, digits, . or -
	string result;
	string s = removeSpace(str);
	if (s.size() > 0 && !(s[0] == ':' || s[0] == '_' || (s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z')))
		result += '_';
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		switch (s[i])
		{
		case ' ': result += "_"; break;			// space is already removed, but I put it here for completion
		case '!': result += "Exclamation"; break;
		case '"': result += "DoubleQuote"; break;
		case '#': result += "Hash"; break;
		case '$': result += "Dollar"; break;
		case '%': result += "Percent"; break;
		case '&': result += "And"; break;
		case '\'': result += "Quote"; break;
		case '(': result += "ParenLeft"; break;
		case ')': result += "ParenRight"; break;
		case '*': result += "Star"; break;
		case '+': result += "Plus"; break;
		case ',': result += "Comma"; break;
		case '/': result += "Slash"; break;
		case ';': result += "Semicolon"; break;
		case '<': result += "AngleLeft"; break;
		case '=': result += "Equal"; break;
		case '>': result += "AngleRight"; break;
		case '?': result += "Question"; break;
		case '@': result += "At"; break;
		case '[': result += "BracketLeft"; break;
		case '\\': result += "Backslash"; break;
		case ']': result += "BracketRight"; break;
		case '^': result += "Hat"; break;
		case '`': result += "Tick"; break;
		case '{': result += "CurlyLeft"; break;
		case '|': result += "Bar"; break;
		case '}': result += "CurlyRight"; break;
		case '~': result += "Tilde"; break;
		default: result += s[i];
		}
	return result;
}

string fixTexAnchor(const string &str)
{
	string result;
	string s = removeSpace(str);
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (s[i] == '~') result += "TILDE";
		else if (s[i] == '\\') result += "BACKSLASH";
		else if (s[i] == '_') result += "UNDERSCORE";
		// TODO: other forbidden characters need to be converted
		else result += s[i];
	return result;
}

static string trimNewLines(const string &s)
{
	string res;
	string newLines;
	unsigned int i, size;
	for (i = 0, size = s.size(); i < size && s[i] == '\n'; ++i);		// ignore first new lines
	for (; i < size; ++i)
		if (s[i] == '\n')
			newLines += s[i];
		else
		{
			res += newLines;							// This way, final new lines are also ignored
			newLines = "";
			res += s[i];
		}
	return res;
}

static string reduceIndent(const string &s)
{
	if (_codeBlockIndent == 0)
		return s;
	string res;
	bool beginningOfLine = true;
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
	{
		if (beginningOfLine)
		{
			for (int j = 0; j < _codeBlockIndent; ++j)	// remove _codeBlockIndent whitespace characters from beginning of each line
				if (s[i] == ' ' || s[i] == '\t')
					++i;
				else
					break;
			beginningOfLine = false;
		}
		if (s[i] == '\n')
		{
			beginningOfLine = true;
			res += s[i];
		}
		else
			res += s[i];
	}
	return res;
}

static string prepareForListing(const string &s)
{
	bool needsOpen = true;
	string res;
	bool inIndentation = true;
#define APPEND_TO_RES(x) \
	do {\
		if (needsOpen)\
		{\
			res += "&\\unimportantCode{";\
			needsOpen = false;\
		}\
		res += (x);\
	} while (0)
	for (unsigned int i = 0, size = s.size(); i < size; ++i)
		if (inIndentation && s[i] == ' ') APPEND_TO_RES("\\ ");
		else if (inIndentation && s[i] == '\t') APPEND_TO_RES("\\ \\ ");
		else
		{
			inIndentation = false;
			if (s[i] == '<' && i+1 < size && s[i+1] == '<') APPEND_TO_RES("<{}");		// prevent << ligature
			else if (s[i] == '>' && i+1 < size && s[i+1] == '>') APPEND_TO_RES(">{}");	// prevent >> ligature
			else if (s[i] == '\n')
			{
				if (!needsOpen)
					res += "}&";
				res += "\n";
				needsOpen = true;
				inIndentation = true;
			}
			else
				APPEND_TO_RES(s[i]);
		}
#undef APPEND_TO_RES
	if (!needsOpen)
		res += "}&";
	return res;
}

static string removeTags(const string &s, shLexer &lexer, shParser &parser)
{
	string res;
	unsigned int i, size;
	for (i = 0, size = s.size(); i < size && (s[i] == '\t' || s[i] == ' '); ++i);
	for (; i < size; ++i)
		if (s[i] == '<')
		{
			for (++i; i < size && s[i] != '>'; ++i);
			if (i == size)
				if (_docType != INVALID && (_makeHtml || _makeTex))
					WARNING("Unclosed tag in heading: %s", s.c_str());
		}
		else if (s[i] != '\n')
			res += s[i];
	return res;
}

static string removeFirstAndLastTag(const string &s, shLexer &lexer, shParser &parser)
{
	int i, size;
	for (i = 0, size = s.size(); i < size && (s[i] == '\t' || s[i] == ' ' || s[i] == '\n'); ++i);
	if (s[i] != '<')
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to remove first tag of a string that doesn't begin with a tag: %s", s.c_str());
	}
	else
	{
		for (++i; i < size && s[i] != '>'; ++i);
		if (i == size)
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				WARNING("Unclosed tag in string: %s", s.c_str());
		}
		else
			++i;
	}
	for (--size; size >= 0 && (s[size] == '\t' || s[size] == ' ' || s[size] == '\n'); --size);
	if (s[size] != '>')
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to remove last tag of a string that doesn't end in a tag: %s", s.c_str());
	}
	else
	{
		for (--size; size >= 0 && s[size] != '<'; --size);
		if (size < 0)
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				WARNING("No match for closing > in string: %s", s.c_str());
		}
		else
			--size;
	}
	return s.substr(i, size-i+1);
}

static string getAnchor(const string &s)
{
	int i = 1;
	bool exists = false;;
	string toSearch = removeSpace(s);
	do
	{
		exists = false;
		for (list<docElement>::const_iterator e = _docMacros.begin(), end = _docMacros.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docVariables.begin(), end = _docVariables.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docGlobalVariables.begin(), end = _docGlobalVariables.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docTypes.begin(), end = _docTypes.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docGlobalTypes.begin(), end = _docGlobalTypes.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docFunctions.begin(), end = _docFunctions.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		for (list<docElement>::const_iterator e = _docGlobalFunctions.begin(), end = _docGlobalFunctions.end(); e != end && !exists; ++e)
			if (toSearch == e->anchorName)
				exists = true;
		if (exists)
		{
			char temp[10] = {'\0'};
			sprintf(temp, "%d", ++i);
			toSearch = removeSpace(s)+temp;
		}
	} while (exists && i < 1000);		// Who would have a 1000 overloaded functions
	return toSearch;
}

void docType(shLexer &lexer, shParser &parser)
{
	string token = lexer.shLCurrentToken();
	if (token == "index")
	{
		_docType = INDEX;
		_docName.html = _docName.tex = _docName.plain = "index";
	}
	else if (token == "class")
		_docType = CLASS;
	else if (token == "struct")
		_docType = STRUCT;
	else if (token == "functions")
		_docType = FUNCTIONS;
	else if (token == "variables")
		_docType = VARIABLES;
	else if (token == "api")
		_docType = API;
	else if (token == "iofile")
		_docType = IOFILE;
	else if (token == "constants")
	{
		_docType = CONSTANTS;
		_docName.html = _docName.tex = _docName.plain = "constants";
	}
	else if (token == "globals")
	{
		_docType = GLOBALS;
		_docName.html = _docName.tex = _docName.plain = "globals";
	}
	else if (token == "example")
		_docType = EXAMPLE;
	else if (token == "other")
		_docType = OTHER;
	else
	{
		_docType = INVALID;
		_evaluate = false;
	}
	_duringHeader = true;
}

void setDocName(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	_docName = elem.text;
}

void fieldType(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	string field = lexer.shLCurrentToken();
	if (field == "next")
		elem.meta = NEXT;
	else if (field == "previous" || field == "prev")
		elem.meta = PREVIOUS;
	else if (field == "version")
		elem.meta = VERSION;
	else if (field == "author")
		elem.meta = AUTHOR;
	else if (field == "shortcut")
		elem.meta = SHORTCUT;
	else if (field == "keyword")
		elem.meta = KEYWORD;
	else if (field == "seealso")
	{
		elem.meta = SEEALSO;
		_duringSeeAlso = true;
	}
	else
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to set field of meta data with invalid field: %s", field.c_str());
		elem.meta = NO_META;
	}
	STACK_PUSH(elem);
}

void setNextPrevType(shLexer &lexer, shParser &parser)
{
	string nextPrevType = lexer.shLCurrentToken();
	stackElement elem = semantic_stack.top();
	STACK_POP;
	if (elem.meta == NEXT || elem.meta == PREVIOUS)
		if (nextPrevType == "index")
			elem.extra = INDEX;
		else if (nextPrevType == "class")
			elem.extra = CLASS;
		else if (nextPrevType == "struct")
			elem.extra = STRUCT;
		else if (nextPrevType == "functions")
			elem.extra = FUNCTIONS;
		else if (nextPrevType == "variables")
			elem.extra = VARIABLES;
		else if (nextPrevType == "api")
			elem.extra = API;
		else if (nextPrevType == "constants")
			elem.extra = CONSTANTS;
		else if (nextPrevType == "iofile")
			elem.extra = IOFILE;
		else if (nextPrevType == "example")
			elem.extra = EXAMPLE;
		else if (nextPrevType == "other")
			elem.extra = OTHER;
		else if (nextPrevType == "globals")
			elem.extra = GLOBALS;
		else
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("Asked to set field of next or previous to a document of invalid type: %s", nextPrevType.c_str());
			elem.extra = INVALID;
		}
	else
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to set field of next or previous to a document of invalid type: %s", metaString(elem.meta, lexer, parser).c_str());
		elem.extra = INVALID;
	}
	STACK_PUSH(elem);
}

void fieldValue(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	stackElement field;
	if (elem.meta != NO_META)				// input had been something like: "next index"
								// as opposed to "next class ClassName"
	{
		field = elem;
		elem.text.html = "";
		elem.text.tex = "";
		elem.text.plain = "";
		elem.meta = NO_META;
	}
	else
	{
		field = semantic_stack.top();
		STACK_POP;
	}
	if (field.meta == NEXT)
		_next.plain = elem.text.plain;			// used only in html, so it should be fine.
	else if (field.meta == PREVIOUS)
		_previous.plain = elem.text.plain;
	else if (field.meta == VERSION)
		_version = elem.text.html;
	else if (field.meta == AUTHOR)
		_author = elem.text.html;
	else if (field.meta == SHORTCUT)
		_shortcuts.push_back(elem.text.html);
	else if (field.meta == KEYWORD)
		_keywords.push_back(elem.text.html);
	else
		if (_docType != INVALID && (_makeHtml || _makeTex))
		{
			INTERNAL("Asked to set field of meta data with invalid field: %s", metaString(field.meta, lexer, parser).c_str());
			SUBMESSAGE("Value to be assigned is: %s", elem.text.html.c_str());
		}
	if (field.meta == NEXT || field.meta == PREVIOUS)
	{
		formattedText *nextOrPrev = '\0';
		if (field.meta == NEXT)
			nextOrPrev = &_next;
		else
			nextOrPrev = &_previous;
		switch (field.extra)
		{
		case CLASS:
		case STRUCT:
		case FUNCTIONS:
		case VARIABLES:
			nextOrPrev->html = "<code class=\"nextprev\">"+elem.text.html+"</code>";
			break;
		case API:
		case IOFILE:
		case EXAMPLE:
		case OTHER:
			nextOrPrev->html = elem.text.html;
			break;
		case INDEX:
			nextOrPrev->html = "Home";
			nextOrPrev->plain = "index";
			break;
		case GLOBALS:
			nextOrPrev->html = "Globals";
			nextOrPrev->plain = "globals";
			break;
		case CONSTANTS:
			nextOrPrev->html = "Constants";
			nextOrPrev->plain = "constants";
			break;
		case INVALID:
		default:
			nextOrPrev->html = elem.text.html;
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("Setting field value of next or previous with invalid or unknown document type");
		}
	}
}

void seealsoValue(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	if (semantic_stack.empty() || semantic_stack.top().meta == NO_META)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When reading value of seealso, the stack doesn't contain seealso meta tag");
		return;
	}
	if (semantic_stack.top().meta != SEEALSO)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When reading value of seealso, the stack has incorrect meta tag: %s", metaString(semantic_stack.top().meta, lexer, parser).c_str());
		return;
	}
	STACK_POP;		// remove seealso
	_seealsos.push_back(elem.text.html);
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before seealso end");
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before seealso end");
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before seealso end");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
	_duringSeeAlso = false;
}

void pop(shLexer &lexer, shParser &parser)
{
	if (semantic_stack.empty())
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Pop requested but there are no elements on the stack");
	}
	else
		STACK_POP;
}

void push(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	string token = lexer.shLCurrentToken();
	elem.text.html = fixHtml(token, lexer, parser);
	elem.text.tex = fixTex(token, lexer, parser);
	elem.text.plain = fixPlain(token, lexer, parser);
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void pushEmpty(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	if (_concatKeepsSpace && lexer.shLSpaceMet())
	{
		elem.text.html = " ";
		elem.text.tex = " ";
		elem.text.plain = " ";
	}
	else
	{
		elem.text.html = "";
		elem.text.tex = "";
		elem.text.plain = "";
	}
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void pushCodeWord(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	string token = lexer.shLCurrentToken();
	elem.text.html = fixHtml(token, lexer, parser, true);
	elem.text.tex = fixTex(token, lexer, parser, true);
	elem.text.plain = fixPlain(token, lexer, parser, true);
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void pushLinkKeyword(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	string keyword = lexer.shLCurrentToken();
	if (keyword == "custom")
		elem.meta = CUSTOM;
	else if (keyword == "image")
		elem.meta = IMAGE;
	else if (keyword == "pdf")
		elem.meta = PDF;
	else if (keyword == "ps")
		elem.meta = PS;
	else if (keyword == "eps")
		elem.meta = EPS;
	else if (keyword == "svg")
		elem.meta = SVG;
	else if (keyword == "object")
		elem.meta = OBJECT;
	else
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Asked to push a link keyword, but the token is not a link keyword. Token is: %s", keyword.c_str());
		elem.meta = NO_META;
	}
	STACK_PUSH(elem);
}

void pushCode(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	if (_codeOpened)
	{
		elem.text.html = "</code>";
		elem.text.tex = "}";
		elem.text.plain = "";
	}
	else if (_duringHeading)
	{
		elem.text.html = "<code class=\"inHeading\">";
		elem.text.tex = "\\code{";
		elem.text.plain = "";
	}
	else if (_duringSeeAlso)
	{
		elem.text.html = "<code class=\"inSeeAlso\">";
		elem.text.tex = "\\code{";
		elem.text.plain = "";
	}
	else if (_inLink)
	{
		elem.text.html = "<code class=\"inLink\">";
		elem.text.tex = "\\code{";
		elem.text.plain = "";
	}
	else
	{
		elem.text.html = "<code class=\"inline\">";
		elem.text.tex = "\\code{";
		elem.text.plain = "";
	}
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_codeOpened = !_codeOpened;
}

void pushEmphasize(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	if (_emphasizeOpened)
	{
		elem.text.html = "</em>";
		elem.text.tex = "}";
		elem.text.plain = "";
	}
	else
	{
		elem.text.html = "<em>";
		elem.text.tex = "\\textit{";
		elem.text.plain = "";
	}
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_emphasizeOpened = !_emphasizeOpened;
}

void pushStrong(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	if (_strongOpened)
	{
		elem.text.html = "</strong>";
		elem.text.tex = "}";
		elem.text.plain = "";
	}
	else
	{
		elem.text.html = "<strong>";
		elem.text.tex = "\\textbf{";
		elem.text.plain = "";
	}
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_strongOpened = !_strongOpened;
}

void pushHTML(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	string tag = lexer.shLCurrentToken();
	string fixed;
	for (unsigned int i = 0, size = tag.size(); i < size; ++i)
		if (tag[i] == '\\')
		{
			++i;
			if (i < size)
				fixed += tag[i];
		}
		else
			if (i > 0 && tag[i] == '<')
				fixed += "&lt;";
			else if (tag[i] == '&')
				fixed += "&amp;";
			else
				fixed += tag[i];
	if (fixed != "<>")
		elem.text.html = fixed;
	else
		elem.text.html = "";	// Treat <> as nothing. Useful for delimiting stuff that would otherwise be combined
	elem.text.tex = "";		// Note: Maybe later you could try to read the tag and create an equivalent latex, but
					// for now I don't care about the latex one enough to do this!
	elem.text.plain = "";
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void pushAnchor(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = ANCHOR;
	STACK_PUSH(elem);
}

void pushNoLink(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = NOLINK;
	STACK_PUSH(elem);
}

void pushYesLink(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = YESLINK;
	STACK_PUSH(elem);
}

void pushInline(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = INLINE;
	STACK_PUSH(elem);
}

void pushIndent(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = INDENT;
	STACK_PUSH(elem);
}

void pushNewLine(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "\n"+INDENT("");
	elem.text.tex = "\n";
	elem.text.plain = "\n";
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void memberOrGlobal(shLexer &lexer, shParser &parser)
{
	string token = lexer.shLCurrentToken();
	string prefix = token.substr(0, 3);
	if (prefix == "MEM")
		_declIsGlobal = false;
	else if (prefix == "GLO")
		_declIsGlobal = true;
	else
	{
		switch (_docType)
		{
		case CLASS:	// Default is member
			_declIsGlobal = false;
			break;
		case STRUCT:	// Default is member for variables and global for functions and types
			if (token.find("VAR") != string::npos)
				_declIsGlobal = false;
			else
				_declIsGlobal = true;
			break;
		case FUNCTIONS:
		case VARIABLES:
		case API:
		case CONSTANTS:
		case GLOBALS:
		case INDEX:
		case IOFILE:
		case EXAMPLE:
		case OTHER:
			_declIsGlobal = true;
			break;
		default:
			// Warning already given
			break;
		}
	}
}

void pushMacroDeclaration(shLexer &lexer, shParser &parser)
{
	docElement newElement;
	if (semantic_stack.top().meta == HASVALUE)
	{
		STACK_POP;
		newElement.type = semantic_stack.top().text;
		STACK_POP;
	}
	while (!semantic_stack.empty() && semantic_stack.top().meta == ARG)
	{
		STACK_POP;
		if (semantic_stack.empty())
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("When inspecting macro args, there is an arg meta that is not preceeded with an actual arg");
			return;
		}
		functionArgument arg;
		arg.name = semantic_stack.top().text;
		STACK_POP;
		newElement.args.push_front(arg);
	}
	if (semantic_stack.empty())
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When inspecting macro, after args there was no macro name");
		return;
	}
	stackElement name = semantic_stack.top();
	STACK_POP;
	newElement.name = name.text;
	newElement.anchorName = getAnchor(name.text.plain);
	newElement.hasArgs = true;
	newElement.hasType = false;
	_docMacros.push_back(newElement);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = MACRODECL;
	STACK_PUSH(elem);
}

void pushVariableDeclaration(shLexer &lexer, shParser &parser)
{
	docElement newElement;
	stackElement type = semantic_stack.top();
	STACK_POP;
	stackElement name = semantic_stack.top();
	STACK_POP;
	newElement.name = name.text;
	newElement.type = type.text;
	newElement.anchorName = getAnchor(name.text.plain);
	newElement.hasArgs = false;
	newElement.hasType = true;
	if (_declIsGlobal)
		_docGlobalVariables.push_back(newElement);
	else
		_docVariables.push_back(newElement);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = VARIABLEDECL;
	STACK_PUSH(elem);
}

void pushTypeDeclaration(shLexer &lexer, shParser &parser)
{
	docElement newElement;
	stackElement type = semantic_stack.top();
	STACK_POP;
	stackElement name = semantic_stack.top();
	STACK_POP;
	newElement.name = name.text;
	newElement.type = type.text;
	newElement.anchorName = getAnchor(name.text.plain);
	newElement.hasArgs = false;
	newElement.hasType = true;
	if (_declIsGlobal)
		_docGlobalTypes.push_back(newElement);
	else
		_docTypes.push_back(newElement);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = TYPEDECL;
	STACK_PUSH(elem);
}

void pushFunctionDeclaration(shLexer &lexer, shParser &parser)
{
	docElement newElement;
	if (semantic_stack.top().meta == HASVALUE)
	{
		STACK_POP;
		newElement.type = semantic_stack.top().text;
		STACK_POP;
	}
	while (!semantic_stack.empty() && semantic_stack.top().meta == ARG)
	{
		STACK_POP;
		if (semantic_stack.empty())
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("When inspecting function args, there is an arg meta that is not preceeded with an actual arg");
			return;
		}
		functionArgument arg;
		if (semantic_stack.top().meta == DEFAULTVALUE)
		{
			STACK_POP;
			arg.defaultValue = semantic_stack.top().text;
			STACK_POP;
		}
		arg.type = semantic_stack.top().text;
		STACK_POP;
		arg.name = semantic_stack.top().text;
		STACK_POP;
		newElement.args.push_front(arg);
	}
	if (semantic_stack.empty())
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When inspecting function, after args there was no macro name");
		return;
	}
	stackElement name = semantic_stack.top();
	STACK_POP;
	newElement.name = name.text;
	newElement.anchorName = getAnchor(name.text.plain);
	newElement.hasArgs = true;
	newElement.hasType = true;
	if (_declIsGlobal)
		_docGlobalFunctions.push_back(newElement);
	else
		_docFunctions.push_back(newElement);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = FUNCTIONDECL;
	STACK_PUSH(elem);
}

void pushInputDeclaration(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = INPUTDECL;
	STACK_PUSH(elem);
}

void pushOutputDeclaration(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = OUTPUTDECL;
	STACK_PUSH(elem);
}

void pushConstGroupDeclaration(shLexer &lexer, shParser &parser)
{
	constantGroup newGroup;
	stackElement name = semantic_stack.top();
	STACK_POP;
	newGroup.name = name.text;
	newGroup.anchorName = getAnchor(name.text.plain);
	_docConstants.push_back(newGroup);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = CONSTGROUPDECL;
	STACK_PUSH(elem);
}

void pushConstDeclaration(shLexer &lexer, shParser &parser)
{
	docElement newElement;
	if (semantic_stack.top().meta == HASVALUE)
	{
		STACK_POP;
		newElement.type = semantic_stack.top().text;
		STACK_POP;
		newElement.hasType = true;
	}
	else
	{
		newElement.type.html = "";
		newElement.type.tex = "";
		newElement.type.plain = "";
		newElement.hasType = false;
	}
	stackElement name = semantic_stack.top();
	STACK_POP;
	newElement.name = name.text;
	newElement.anchorName = getAnchor(name.text.plain);
	newElement.hasArgs = false;
	_docConstants.back().constants.push_back(newElement);
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = CONSTDECL;
	STACK_PUSH(elem);
}

void pushImportance(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = IMPORTANCE;
	elem.extra = lexer.shLCurrentToken().size();
	STACK_PUSH(elem);
}

void pushMightHaveSpace(shLexer &lexer, shParser &parser)
{
	string token = lexer.shLCurrentToken();
	string res;
	if (_concatKeepsSpace)
		res = token;
	else
		for (unsigned int i = 0, size = token.size(); i < size; ++i)
			if (token[i] != ' ' && token[i] != '\t')
				res += token[i];
	stackElement elem;
	elem.text.html = fixHtml(res, lexer, parser);
	elem.text.tex = fixTex(res, lexer, parser);
	elem.text.plain = fixPlain(res, lexer, parser);
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void pushArg(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = ARG;
	elem.extra = lexer.shLCurrentToken().size();
	STACK_PUSH(elem);
}

void pushHasValue(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = HASVALUE;
	elem.extra = lexer.shLCurrentToken().size();
	STACK_PUSH(elem);
}

void pushDefaultValue(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = DEFAULTVALUE;
	STACK_PUSH(elem);
}

void pushParagraphAndItems(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = PARAGRAPHANDITEMS;
	STACK_PUSH(elem);
}

void concat(shLexer &lexer, shParser &parser)
{
	if (semantic_stack.size() == 0)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Concat expects two elements to be on the stack. There are none");
		return;
	}
	stackElement elem2 = semantic_stack.top();
	STACK_POP;
	if (semantic_stack.size() == 0)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Concat expects two elements to be on the stack. There is only one");
		return;
	}
	stackElement elem1 = semantic_stack.top();
	STACK_POP;
	if (elem1.meta != NO_META || elem2.meta != NO_META)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Concatenating two elements that are not non-meta. Top of stack has meta: %s and below it has meta: %s",
					metaString(elem2.meta, lexer, parser).c_str(), metaString(elem1.meta, lexer, parser).c_str());
	if (_concatKeepsSpace && lexer.shLSpaceMet() && elem1.text.html != "")
	{
		elem1.text.html += " ";
		elem1.text.tex += " ";
		elem1.text.plain += " ";
	}
	elem1.text += elem2.text;
	elem1.meta = NO_META;
	STACK_PUSH(elem1);
}

void concatKeepsSpace(shLexer &lexer, shParser &parser)
{
	_concatKeepsSpace = true;
}

void concatIgnoresSpace(shLexer &lexer, shParser &parser)
{
	_concatKeepsSpace = false;
}

void concatStoreState(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	if (_concatKeepsSpace && lexer.shLSpaceMet())
	{
		elem.text.html = " ";
		elem.text.tex = " ";
		elem.text.plain = " ";
	}
	else
	{
		elem.text.html = "";
		elem.text.tex = "";
		elem.text.plain = "";
	}
	elem.meta = KEEPSPACE;
	elem.extra = _concatKeepsSpace;
	STACK_PUSH(elem);
}

void concatRestoreState(shLexer &lexer, shParser &parser)
{
	stackElement concatData;
	stack<stackElement> generatedInBetween;
	while (!semantic_stack.empty() && semantic_stack.top().meta != KEEPSPACE)
	{
		generatedInBetween.push(semantic_stack.top());
		STACK_POP;
	}
	if (semantic_stack.empty())
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Request to restore state of concat, but stack does not contains keepspace meta element");
	}
	else
	{
		_concatKeepsSpace = semantic_stack.top().extra;
		STACK_POP;
	}
	while (!generatedInBetween.empty())
	{
		STACK_PUSH(generatedInBetween.top());
		generatedInBetween.pop();
	}
}

void concatDefaultValue(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	if (elem.meta == DEFAULTVALUE)
	{
		STACK_POP;
		elem = semantic_stack.top();
		STACK_POP;
		stackElement elem2 = semantic_stack.top();
		STACK_POP;
		elem2.text.html += " = ";
		elem2.text.tex += " = ";
		elem2.text.plain += " = ";
		elem2.text += elem.text;
		elem2.meta = NO_META;
		STACK_PUSH(elem2);
	}
}

void concatParagraphAndItems(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "";
	elem.text.tex = "";
	elem.text.plain = "";
	elem.meta = NO_META;
	while (!semantic_stack.empty() && semantic_stack.top().meta != PARAGRAPHANDITEMS)
	{
		stackElement elem2 = semantic_stack.top();
		STACK_POP;
		elem.text.html = elem2.text.html+elem.text.html;
		elem.text.tex = elem2.text.tex+elem.text.tex;
		elem.text.plain = elem2.text.plain+elem.text.plain;
	}
	if (semantic_stack.empty())
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When concating paragraphs and items, an empty stack was encountered");
		pushEmpty(lexer, parser);
		return;
	}
	STACK_POP;
	STACK_PUSH(elem);
}

void inLink(shLexer &lexer, shParser &parser)
{
	_inLink = true;
}

void notInLink(shLexer &lexer, shParser &parser)
{
	_inLink = false;
}

void closeItemsUntil(shLexer &lexer, shParser &parser)
{
	int itemType = -1;
	stackElement topOfStack = semantic_stack.top();
	STACK_POP;
	string itemToken = topOfStack.text.plain;
	bool found = false;
	if (itemToken.size() >= 3)
		found = itemToken[0] == '.' && itemToken[1] == '.' && itemToken[2] == '.';
	if (found)
		itemType = 0;
	for (unsigned int i = 0, size = itemToken.size(); i < size && !found; ++i)
	{
		found = true;
		if (itemToken[i] == '-')
			itemType = 1;
		else if (itemToken[i] == '+')
			itemType = 2;
		else if (itemToken[i] == '*')
			itemType = 3;
		else if (itemToken[i] == '.')
			itemType = 4;
		else if (itemToken[i] == ',')
			itemType = 5;
		else if (itemToken[i] == ')')
			itemType = 6;
		else
			found = false;
	}
	if (!found)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Matched token is supposed to indicate item but doesn't contain any of \"-+*.,)\". Token is: %s", itemToken.c_str());
	if (itemType == 0 && item_level_stack.size() == 0)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			WARNING("Token \"... \" shows continuation of an item in the next paragraph. There are no items before though");
		STACK_PUSH(topOfStack);
		return;
	}
	if (itemType == 0)		// This form `... ...` is not going to be documented, but if written, it would close one item list
					// and continue from the last item before it
	{
		stackElement elem;
		elem.text.html += INDENT("</li>\n");
		--_indentLevel;
		if (item_level_stack.back() <= 3)
		{
			elem.text.html += INDENT("</ul>\n");
			elem.text.tex += "\\end{itemize}\n";
		}
		else
		{
			elem.text.html += INDENT("</ol>\n");
			elem.text.tex += "\\end{enumerate}\n";
		}
		elem.text.tex += '\n';
		item_level_stack.pop_back();
		elem.meta = NO_META;
		STACK_PUSH(elem);
		concat(lexer, parser);
	}
	else
	{
		found = false;
		for (list<int>::const_iterator i = item_level_stack.begin(), end = item_level_stack.end(); i != end && !found; ++i)
			if (*i == itemType)
				found = true;
		if (!found)
			WARNING("There are no items of type \"%s%s\" to be continued with \"...\"", (itemType > 3?"1":""), itemToken.c_str());
		else
		{
			stackElement elem;
			while (!item_level_stack.empty() && item_level_stack.back() != itemType)
			{
				elem.text.html += INDENT("</li>\n");
				--_indentLevel;
				if (item_level_stack.back() <= 3)
				{
					elem.text.html += INDENT("</ul>\n");
					elem.text.tex += "\\end{itemize}\n";
				}
				else
				{
					elem.text.html += INDENT("</ol>\n");
					elem.text.tex += "\\end{enumerate}\n";
				}
				item_level_stack.pop_back();
			}
			elem.text.tex += '\n';
			elem.meta = NO_META;
			STACK_PUSH(elem);
			concat(lexer, parser);
		}
	}
	STACK_PUSH(topOfStack);
}

void headingBegin(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.plain = "";
	switch (lexer.shLCurrentToken().size())
	{
	case 1:
		elem.text.html = "<h1";
		elem.text.tex = "\\section";
		elem.extra = 1;
		break;
	case 2:
		elem.text.html = "<h2";
		elem.text.tex = "\\subsection";
		elem.extra = 2;
		break;
	case 3:
		elem.text.html = "<h3";
		elem.text.tex = "\\subsubsection";
		elem.extra = 3;
		break;
	case 4:
		elem.text.html = "<h4";
		elem.text.tex = "\\paragraph";
		elem.extra = 4;
		break;
	case 5:
	default:
		elem.text.html = "<h5";
		elem.text.tex = "\\subparagraph";
		elem.extra = 5;
		break;
	}
	if (_duringHeader)
		elem.text.html += " id=\"title-heading\"><a class=\"title\" href=\"index.html\"";
	elem.text.html += ">";
	if (_duringDefinition)
		elem.text.tex += "*";
	elem.text.tex += "{";
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_duringHeading = true;
}

void headingEnd(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	stackElement begin = semantic_stack.top();
	STACK_POP;
	CLOSE_ITEMS;			// before writing the new heading, close all open item lists
	stackElement res;
	if (_duringHeader)
		res.text.html = INDENT(begin.text.html+elem.text.html+"</a>");
	else
		res.text.html = INDENT("<a id=\""+fixHtmlAnchor(elem.text.plain)+"\"></a>\n")+INDENT(begin.text.html+elem.text.html);
	res.text.tex = begin.text.tex+elem.text.tex+"}\\label{"+fixTexAnchor(_docName.plain);
	if (!_duringHeader)
		res.text.tex += " "+fixTexAnchor(elem.text.plain);
	res.text.tex += "}\n";
	res.text.plain = begin.text.plain+elem.text.plain;
	switch (begin.extra)
	{
	case 1:
		res.text.html += "</h1>\n";
		break;
	case 2:
		res.text.html += "</h2>\n";
		break;
	case 3:
		res.text.html += "</h3>\n";
		break;
	case 4:
		res.text.html += "</h4>\n";
		break;
	case 5:
	default:
		res.text.html += "</h5>\n";
		break;
	}
	res.meta = NO_META;
	STACK_PUSH(res);
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before header end");
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before header end");
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before header end");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
	_duringHeading = false;
}

void itemBegin(shLexer &lexer, shParser &parser)
{
	int itemType = -1;
	bool shouldIndent = false;
	if (semantic_stack.top().meta == INDENT)
	{
		shouldIndent = true;
		STACK_POP;
	}
	string itemToken = semantic_stack.top().text.plain;
	STACK_POP;
	bool found = false;
	if (itemToken.size() >= 3)
		found = itemToken[0] == '.' && itemToken[1] == '.' && itemToken[2] == '.';
	if (found)
		itemType = 0;
	for (unsigned int i = 0, size = itemToken.size(); i < size && !found; ++i)
	{
		found = true;
		if (itemToken[i] == '-')
			itemType = 1;
		else if (itemToken[i] == '+')
			itemType = 2;
		else if (itemToken[i] == '*')
			itemType = 3;
		else if (itemToken[i] == '.')
			itemType = 4;
		else if (itemToken[i] == ',')
			itemType = 5;
		else if (itemToken[i] == ')')
			itemType = 6;
		else
			found = false;
	}
	if (!found)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("Matched token is supposed to indicate item but doesn't contain any of -+*.,). Token is: %s", itemToken.c_str());
	if (itemType == 0 && item_level_stack.size() == 0)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
		{
			WARNING("Token \"... \" shows continuation of an item in the next paragraph. There are no items before though");
			SUBMESSAGE("Assuming new unordered list of type \"- \"");
		}
		itemType = 1;
	}
	if (itemType == 0)
	{
		stackElement elem;
		elem.text.html += INDENT("<p class=\"itemContinuation"+string(shouldIndent?"WithIndent":"")+"\">");
		elem.text.tex += "\n"+string(shouldIndent?"\\leftskip 20pt ":"");
		elem.text.plain = "";
		elem.meta = NO_META;
		STACK_PUSH(elem);
		_itemIndented = shouldIndent;
	}
	else
	{
		found = false;
		for (list<int>::const_iterator i = item_level_stack.begin(), end = item_level_stack.end(); i != end && !found; ++i)
			if (*i == itemType)
				found = true;
		if (!found)
		{
			item_level_stack.push_back(itemType);		// nested item list
			stackElement elem;
			if (itemType <= 3)
			{
				elem.text.html += INDENT("<ul>\n");
				elem.text.tex += "\\begin{itemize}\n";
				elem.text.plain = "";
			}
			else
			{
				elem.text.html += INDENT("<ol>\n");
				elem.text.tex += "\\begin{enumerate}\n";
				elem.text.plain = "";
			}
			++_indentLevel;
			elem.text.html += INDENT("<li><p class=\"inListItem\">");
			elem.text.tex += "\\item ";
			elem.text.plain = "";
			elem.meta = NO_META;
			STACK_PUSH(elem);
		}
		else
		{
			stackElement elem;
			while (!item_level_stack.empty() && item_level_stack.back() != itemType)
			{
				elem.text.html += INDENT("</li>\n");
				--_indentLevel;
				if (item_level_stack.back() <= 3)
				{
					elem.text.html += INDENT("</ul>\n");
					elem.text.tex += "\\end{itemize}\n";
					elem.text.plain = "";
				}
				else
				{
					elem.text.html += INDENT("</ol>\n");
					elem.text.tex += "\\end{enumerate}\n";
					elem.text.plain = "";
				}
				item_level_stack.pop_back();
			}
			elem.text.tex += '\n';
			elem.meta = NO_META;
			STACK_PUSH(elem);
//			concat(lexer, parser);
			if (item_level_stack.empty())
			{
				if (_docType != INVALID && (_makeHtml || _makeTex))
					INTERNAL("When closing item lists because of item of previous type, "
							"the item is found once but later the stack is emptied without finding it");
				pushEmpty(lexer, parser);
			}
			else
			{
				elem.text.html = INDENT("</li><li><p class=\"inListItem\">");
				elem.text.tex = "\\item ";
				elem.text.plain = "";
				elem.meta = NO_META;
				STACK_PUSH(elem);
				_itemIndented = false;
			}
		}
	}
}

void itemEnd(shLexer &lexer, shParser &parser)
{
	stackElement elem;
	elem.text.html = "</p>\n";
	elem.text.tex = "\n"+string(_itemIndented?"\n\\leftskip 0pt\n":"");
	elem.text.plain = "\n";
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_itemIndented = false;
	concat(lexer, parser);
	if (!_objectsToAppend.empty())
	{
		stackElement elem2;
		while (!_objectsToAppend.empty())
		{
			elem2.text.html += INDENT(_objectsToAppend.front().html);
			elem2.text.tex += _objectsToAppend.front().tex+"\n\n";
			elem2.text.plain += _objectsToAppend.front().plain+"\n";
			_objectsToAppend.pop_front();
		}
		elem2.meta = NO_META;
		STACK_PUSH(elem2);
		concat(lexer, parser);
	}
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before paragraph end");
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before paragraph end");
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before paragraph end");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
}

void paragraphEnd(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	bool shouldIndent = false;
	if (!semantic_stack.empty() && semantic_stack.top().meta == INDENT)
	{
		shouldIndent = true;
		STACK_POP;
	}
	pushEmpty(lexer, parser);
	CLOSE_ITEMS;			// before writing the new paragraph, close all open item lists
	elem.text.html = INDENT("<p"+string(shouldIndent?" class=\"indent\"":"")+">"+trimNewLines(elem.text.html)+"</p>\n");
	elem.text.tex = string(shouldIndent?"\\leftskip 20pt ":"")+elem.text.tex+"\n\n"+string(shouldIndent?"\\leftskip 0pt\n":"");
	elem.meta = NO_META;
	STACK_PUSH(elem);
	concat(lexer, parser);
	if (!_objectsToAppend.empty())
	{
		stackElement elem2;
		while (!_objectsToAppend.empty())
		{
			elem2.text.html += INDENT(_objectsToAppend.front().html);
			elem2.text.tex += _objectsToAppend.front().tex+"\n\n";
			elem2.text.plain += _objectsToAppend.front().plain+"\n";
			_objectsToAppend.pop_front();
		}
		elem2.meta = NO_META;
		STACK_PUSH(elem2);
		concat(lexer, parser);
	}
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before paragraph end");
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before paragraph end");
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before paragraph end");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
}

void codeBlockBegin(shLexer &lexer, shParser &parser)
{
	_duringCodeBlock = true;
	string symbol = lexer.shLCurrentToken();
	int len;
	for (_codeBlockIndent = 0, len = symbol.size(); _codeBlockIndent < len &&
			(symbol[_codeBlockIndent] == ' ' || symbol[_codeBlockIndent] == '\t'); ++_codeBlockIndent);
}

void codeBlockEnd(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	elem.text.html = "<pre><code class=\"pre\">"+reduceIndent(trimNewLines(elem.text.html))+"</code></pre>\n";
	elem.text.tex = "\\begin{codeblockframe}\n"
				"\\begin{lstlisting}\n"+
					prepareForListing(reduceIndent(trimNewLines(elem.text.tex)))+"\n"
				"\\end{lstlisting}\n"
			"\\end{codeblockframe}\n\n";
	elem.meta = NO_META;
	STACK_PUSH(elem);
	_duringCodeBlock = false;
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before code block end");			// This shouldn't happen
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before code block end");		// This shouldn't happen
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before code block end");		// This shouldn't happen
	if (_codeOpened || _emphasizeOpened || _strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("These flags are not used in code block! This shouldn't have happened");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
}

void hrEnd(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;			// before writing the horizontal rule, close all open item lists
	stackElement elem;
	elem.text.html = INDENT("<div class=\"hr\"></div>\n\n");
	elem.text.tex = "\\rule{\\linewidth}{0.3mm}\n";
	elem.meta = NO_META;
	STACK_PUSH(elem);
	if (_codeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed code before horizontal rule is encountered");
	if (_emphasizeOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed emphasis before horizontal rule is encountered");
	if (_strongOpened)
		if (_docType != INVALID && (_makeHtml || _makeTex))
			ERROR("Unclosed strong before horizontal rule is encountered");
	_codeOpened = false;
	_emphasizeOpened = false;
	_strongOpened = false;
}

void importantCodeEnd(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	if (semantic_stack.empty() || semantic_stack.top().meta == NO_META)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When determining importance level of statement in block code, the stack doesn't contain importance meta tag");
		return;
	}
	if (semantic_stack.top().meta != IMPORTANCE)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When determining importance level of statement in block code, the stack has incorrect meta tag: %s",
					metaString(semantic_stack.top().meta, lexer, parser).c_str());
		return;
	}
	if (semantic_stack.top().extra <= 1)
	{
		elem.text.html = "<span class=\"somewhatImportantCode\">"+elem.text.html+"</span>";
		elem.text.tex = "}\\somewhatImportantCode{"+elem.text.tex+"}\\unimportantCode{";
	}
	else
	{
		elem.text.html = "<span class=\"veryImportantCode\">"+elem.text.html+"</span>";
		elem.text.tex = "}\\veryImportantCode{"+elem.text.tex+"}\\unimportantCode{";
	}
	STACK_POP;
	elem.meta = NO_META;
	STACK_PUSH(elem);
}

void definitionBegin(shLexer &lexer, shParser &parser)
{
	_duringDefinition = true;
}

void definitionEnd(shLexer &lexer, shParser &parser)
{
	_duringDefinition = false;
}

void makeLink(shLexer &lexer, shParser &parser)
{
	// Possible entries in the stack and the corresponding text/address/address_anchor:
	// stack elements (from bottom to top)                       text                address                address_anchor
	// text1 meta:NOLINK                                         text1               nospace(text1).html
	// text1 meta:YESLINK meta:CUSTOM text2                      text1               text2
	// text1 meta:YESLINK meta:CUSTOM text2 meta:ANCHOR text3    text1               text2                  text3
	// text1 meta:YESLINK meta:IMAGE text2                       text1(alt text)     text2
	// text1 meta:YESLINK meta:IMAGE text2 meta:ANCHOR text3     text1(alt text)     text2                  text3  <-- anchor for image is meaningless
	// text1 meta:YESLINK meta:OBJECT text2                      text1(alt text)     text2
	// text1 meta:YESLINK meta:OBJECT text2 meta:ANCHOR text3    text1(alt text)     text2                  text3  <-- anchor for object is meaningless
	// text1 meta:YESLINK text2                                  text1               text2.html
	// text1 meta:YESLINK text2 meta:ANCHOR text3                text1               text2.html             text3
	// text1 meta:YESLINK meta:ANCHOR text3                      text1                                      text3
	// meta:ANCHOR text1 meta:NOLINK                             text1                                      nospace(text1)
	// meta:ANCHOR text1 meta:YESLINK meta:CUSTOM text2          text1               text2                  nospace(text1)
	// meta:ANCHOR text1 meta:YESLINK meta:IMAGE text2           text1(alt text)     text2                         <-- anchor for image is meaningless
	// meta:ANCHOR text1 meta:YESLINK meta:OBJECT text2          text1(alt text)     text2                         <-- anchor for object is meaningless
	// meta:ANCHOR text1 meta:YESLINK text2                      text1               text2.html             nospace(text1)
	// in all cases of meta:CUSTOM, meta:IMAGE or meta:OBJECT there could be a meta:INLINE after it. After custom it has no effect, but after the
	// others makes them inline rather than appending to the next paragraph (or item).
	stackElement text, address, address_anchor;
	stackElement link;
	stackElement elem = semantic_stack.top();
	STACK_POP;
	bool shouldInline = false;
	formattedText toAppend;
	if (elem.meta == NOLINK)
	{
		text = semantic_stack.top();
		STACK_POP;
		if (!semantic_stack.empty() && semantic_stack.top().meta == ANCHOR)
		{
			address_anchor.text.html = removeSpace(text.text.plain);
			address_anchor.text.tex = removeSpace(text.text.plain);
			address_anchor.text.plain = removeSpace(text.text.plain);
			address.text.tex = removeSpace(_docName.plain);
			STACK_POP;
		}
		else
		{
			address.text.html = removeSpace(text.text.plain);
			address.text.tex = removeSpace(text.text.plain);
			address.text.plain = removeSpace(text.text.plain);
		}
		link.text.html = "<a "+(_duringCodeBlock?string("class=\"pre\" "):string(""))+"href=\"";
		link.text.tex = _duringHeading?"\\texorpdfstring{":"";
		link.text.tex += "\\ulhyperref{";
		if (address.text.html != "")
			link.text.html += address.text.html+".html";
		link.text.tex += fixTexAnchor(address.text.tex);
		if (address_anchor.text.html != "")
		{
			link.text.html += "#"+fixHtmlAnchor(address_anchor.text.html);
			link.text.tex += " "+fixTexAnchor(address_anchor.text.tex);
		}
		link.text.html += "\">"+text.text.html+"</a>";
		link.text.tex += "}{"+text.text.tex+"}";
		link.text.tex += _duringHeading?"}{"+text.text.tex+"}":string("");
		link.text.plain = text.text.plain;
		shouldInline = true;
	}
	else if (elem.meta != NO_META)
	{
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When generating link, a non-meta data was expected on top of the stack. However, that element has meta tag: %s",
				metaString(elem.meta, lexer, parser).c_str());
		link.meta = NO_META;
		STACK_PUSH(link);
		return;
	}
	else
	{
#define CHECK_NOT_EMPTY \
do {\
	if (semantic_stack.empty())\
	{\
		if (_docType != INVALID && (_makeHtml || _makeTex))\
			INTERNAL("When generating link, empty stack is encountered");\
		return;\
	}\
} while (0)
#define CHECK_META \
do {\
	if (semantic_stack.top().meta == NO_META)\
	{\
		if (_docType != INVALID && (_makeHtml || _makeTex))\
			INTERNAL("When generating link, a meta tag was expected after the text on top of the stack. However, that element has html value: %s",\
				elem.text.html.c_str());\
		return;\
	}\
} while (0)
		address = elem;
		address.text.tex = address.text.plain;
		CHECK_NOT_EMPTY;
		CHECK_META;
		if (semantic_stack.top().meta == ANCHOR)
		{
			address_anchor = address;		// It was actually anchor, not address
			address_anchor.text.html = removeSpace(address_anchor.text.plain);
			address_anchor.text.tex = removeSpace(address_anchor.text.plain);
			address_anchor.text.plain = removeSpace(address_anchor.text.plain);
			address.text.html = "";
			address.text.tex = "";
			address.text.plain = "";
			STACK_POP;
			CHECK_NOT_EMPTY;
			if (semantic_stack.top().meta == NO_META)
			{
				address = semantic_stack.top();
				STACK_POP;
				address.text.tex = address.text.plain;
			}
		}
		CHECK_NOT_EMPTY;
		CHECK_META;
		bool makeCustom = false;
		bool makeImage = false;
		bool makeObject = false;
		metaEnum objType = NO_META;
		if (semantic_stack.top().meta == INLINE)
		{
			STACK_POP;
			CHECK_NOT_EMPTY;
			shouldInline = true;
		}
		if (semantic_stack.top().meta == YESLINK)
		{
			STACK_POP;
			CHECK_NOT_EMPTY;
			text = semantic_stack.top();
			STACK_POP;
			if (!semantic_stack.empty() && semantic_stack.top().meta == ANCHOR)
			{
				address_anchor.text.html = removeSpace(text.text.plain);
				address_anchor.text.tex = removeSpace(text.text.plain);
				address_anchor.text.plain = removeSpace(text.text.plain);
				STACK_POP;
			}
			if (address.text.html != "")
				address.text.html += ".html";
			if (address.text.tex == "")
				address.text.tex = removeSpace(_docName.plain);
			shouldInline = true;
		}
		else if (semantic_stack.top().meta == CUSTOM)
		{
			STACK_POP;
			CHECK_NOT_EMPTY;
			CHECK_META;		// should be yeslink
			STACK_POP;
			text = semantic_stack.top();
			STACK_POP;
			makeCustom = true;
			if (!semantic_stack.empty() && semantic_stack.top().meta == ANCHOR)
			{
				address_anchor.text.html = removeSpace(text.text.plain);
				address_anchor.text.tex = removeSpace(text.text.plain);
				address_anchor.text.plain = removeSpace(text.text.plain);
				STACK_POP;
			}
			if (shouldInline)
				WARNING("inline keyword with custom links has no effect");
			shouldInline = true;
		}
		else if (semantic_stack.top().meta == IMAGE)
		{
			STACK_POP;
			CHECK_NOT_EMPTY;
			CHECK_META;		// should be yeslink
			STACK_POP;
			text = semantic_stack.top();
			STACK_POP;
			makeImage = true;
			if (!semantic_stack.empty() && semantic_stack.top().meta == ANCHOR)
			{
				address_anchor.text.html = removeSpace(text.text.plain);
				address_anchor.text.tex = removeSpace(text.text.plain);
				address_anchor.text.plain = removeSpace(text.text.plain);
				STACK_POP;
			}
			if (address_anchor.text.html != "")
				if (_docType != INVALID && (_makeHtml || _makeTex))
					WARNING("Anchor definition for images has no effect. The anchor will be dropped");
		}
		else if (semantic_stack.top().meta == OBJECT || semantic_stack.top().meta == PDF || semantic_stack.top().meta == PS ||
				semantic_stack.top().meta == EPS || semantic_stack.top().meta == SVG)
		{
			objType = semantic_stack.top().meta;
			STACK_POP;
			CHECK_NOT_EMPTY;
			CHECK_META;		// should be yeslink
			STACK_POP;
			text = semantic_stack.top();
			STACK_POP;
			makeObject = true;
			if (!semantic_stack.empty() && semantic_stack.top().meta == ANCHOR)
			{
				address_anchor.text.html = removeSpace(text.text.plain);
				address_anchor.text.tex = removeSpace(text.text.plain);
				address_anchor.text.plain = removeSpace(text.text.plain);
				STACK_POP;
			}
			if (address_anchor.text.html != "")
				if (_docType != INVALID && (_makeHtml || _makeTex))
					WARNING("Anchor definition for objects has no effect. The anchor will be dropped");
		}
		if (makeCustom)
		{
			link.text.html = "<a "+(_duringCodeBlock?string("class=\"pre\" "):string(""))+"href=\""+address.text.html;
			link.text.tex = _duringHeading?"\\texorpdfstring{":"";
			link.text.tex += "\\ulhref{"+address.text.tex;
			if (address_anchor.text.html != "")
			{
				link.text.html += "#"+fixHtmlAnchor(address_anchor.text.html);
				link.text.tex += "#"+fixHtmlAnchor(address_anchor.text.tex);		// TODO: shouldn't this be address_anchor.text.html?
													// or something like fixTex(fixHtmlAnchor(...text.html))?
			}
			link.text.html += "\">"+text.text.html+"</a>";
			link.text.tex += "}{"+text.text.tex+"}";
			link.text.tex += _duringHeading?"}{"+text.text.tex+"}":string("");
			link.text.plain = text.text.plain;
		}
		else if (makeImage)
		{
			char figureUniqueId[20];
			sprintf(figureUniqueId, "%d", _figureUniqueId++);
			if (shouldInline)
			{
				link.text.html = "<img src=\""+address.text.html+"\" alt=\""+text.text.html+"\" title=\""+text.text.html+"\" />";
				link.text.tex = "\\includegraphics[width=\\textwidth]{"+address.text.tex+"}";
				link.text.plain = text.text.plain;
			}
			else
			{
				char temp[20];
				sprintf(temp, "%d", _figureNum++);
				link.text.html = "<a href=\""+removeSpace(_docName.plain)+".html#"+"Figure"+string(figureUniqueId)+"\">Figure "+temp+"</a>";
				link.text.tex = "Figure~\\ref{Figure "+fixTexAnchor(_docName.plain)+" "+figureUniqueId+" "+fixTexAnchor(text.text.plain)+"}";
				link.text.plain = text.text.plain;
				toAppend.html = "<p class=\"center\"><a id=\"Figure"+string(figureUniqueId)+"\"></a>\n"+
					INDENT("<img src=\"")+address.text.html+"\" alt=\""+text.text.html+"\" title=\""+text.text.html+"\" /><br />\n"+
					INDENT("Figure ")+temp+". "+text.text.html+"</p>\n";
				toAppend.tex = "\\begin{figure}\n\\includegraphics[width=\\textwidth]{"+address.text.tex+"}\n\\caption{"+text.text.tex+"}\n"
					"\\label{Figure "+fixTexAnchor(_docName.plain)+" "+figureUniqueId+" "+fixTexAnchor(text.text.plain)+"}\n\\end{figure}";
				toAppend.plain = text.text.plain;
			}
		}
		else if (makeObject)
		{
			char figureUniqueId[20];
			sprintf(figureUniqueId, "%d", _figureUniqueId++);
			if (shouldInline)
			{
				link.text.html = "<object data=\""+address.text.html+"\""+(objType == PDF?" type=\"application/pdf\"":
											objType == PS?" type=\"application/ps\"":
											objType == EPS?" type=\"application/eps\"":
											objType == SVG?" type=\"image/svg+xml\"":"")+">"
					"<a href=\""+address.text.html+"\">"+text.text.html+"</a>"	// if couldn't embed, at least give the link
					"</object>";
				link.text.tex = "\\includegraphics[width=\\textwidth"+(objType != PDF?string(",type=pdf,ext=.pdf,read=.pdf"):string(""))+"]{"+address.text.tex+"}";
				link.text.plain = text.text.plain;
			}
			else
			{
				char temp[20];
				sprintf(temp, "%d", _figureNum++);
				link.text.html = "<a href=\""+removeSpace(_docName.plain)+".html#"+"Figure"+string(figureUniqueId)+"\">Figure "+temp+"</a>";
				link.text.tex = "Figure~\\ref{Figure "+fixTexAnchor(_docName.plain)+" "+figureUniqueId+" "+fixTexAnchor(text.text.plain)+"}";
				link.text.plain = text.text.plain;
				toAppend.html = "<p class=\"center\"><a id=\"Figure"+string(figureUniqueId)+"\"></a>\n"+
					INDENT("<object data=\"")+address.text.html+"\""+(objType == PDF?" type=\"application/pdf\"":
										objType == PS?" type=\"application/ps\"":
										objType == EPS?" type=\"application/eps\"":
										objType == SVG?" type=\"image/svg+xml\"":"")+">"
					"<a href=\""+address.text.html+"\">"+text.text.html+"</a>"	// if couldn't embed, at least give the link
					"</object><br />\n"+
					INDENT("Figure ")+temp+". "+text.text.html+"</p>\n";
				toAppend.tex = "\\begin{figure}\n\\includegraphics[width=\\textwidth"+(objType != PDF?string(",type=pdf,ext=.pdf,read=.pdf"):string(""))+"]"
					"{"+address.text.tex+"}\n\\caption{"+text.text.tex+"}\n"
					"\\label{Figure "+fixTexAnchor(_docName.plain)+" "+figureUniqueId+" "+fixTexAnchor(text.text.plain)+"}\n\\end{figure}";
				toAppend.plain = text.text.plain;
			}
			if (_makeTex && _objectsAlreadySeen.find(address.text.plain) == _objectsAlreadySeen.end())
			{
				if (objType == PS || objType == EPS)
				{
					ofstream fout("generated/tex/Makefile.cvt", ios::app);
					fout << address.text.plain << ".pdf: " << address.text.plain << endl;
					fout << "\t-epstopdf $< --outfile=$@" << endl;
				}
				else if (objType == SVG)
				{
					ofstream fout("generated/tex/Makefile.cvt", ios::app);
					fout << address.text.plain << ".pdf: " << address.text.plain << endl;
					fout << "\t-inkscape $< --export-pdf=$@" << endl;
				}
				else if (objType == OBJECT)
				{
					WARNING("To embed an object file in your document, you need to convert it to PDF. To name the PDF file, simply "
						"append .pdf to the whole file name, for example \"file.svg.pdf\"");
					SUBMESSAGE("I am going to try converting it with inkscape, but I can't promise anything!");
					SUBMESSAGE("If you want to add your own conversion to the Makefile, append the rule to generated/tex/Makefile.cvt and dependency to "
						"generated/tex/Makefile.obj after generation of documents");
					ofstream fout("generated/tex/Makefile.cvt", ios::app);
					fout << address.text.plain << ".pdf: " << address.text.plain << endl;
					fout << "\t-inkscape $< --export-pdf=$@" << endl;
				}
				if (objType == PS || objType == EPS || objType == SVG || objType == OBJECT)
				{
					ofstream fout2("generated/tex/Makefile.obj", ios::app);
					fout2 << " " << address.text.plain << ".pdf";			// add to objects to be built
					ofstream fout3("generated/tex/Makefile.cln", ios::app);
					fout3 << "\t-rm -f " << address.text.plain << ".pdf" << endl;	// add clean up rule
				}
				_objectsAlreadySeen.insert(address.text.plain);
			}
		}
		else
		{
			link.text.html = "<a "+(_duringCodeBlock?string("class=\"pre\" "):string(""))+"href=\""+address.text.html;
			link.text.tex = _duringHeading?"\\texorpdfstring{":"";
			link.text.tex += "\\ulhyperref{"+fixTexAnchor(address.text.tex);
			if (address_anchor.text.html != "")
			{
				link.text.html += "#"+fixHtmlAnchor(address_anchor.text.html);
				link.text.tex += " "+fixTexAnchor(address_anchor.text.tex);
			}
			link.text.html += "\">"+text.text.html+"</a>";
			link.text.tex += "}{"+text.text.tex+"}";
			link.text.tex += _duringHeading?"}{"+text.text.tex+"}":string("");
			link.text.plain = text.text.plain;
		}
	}
#undef CHECK_EMPTY
#undef CHECK_META
	link.meta = NO_META;
	STACK_PUSH(link);
	if (!shouldInline)
		_objectsToAppend.push_back(toAppend);
}

void readWhiteSpace(shLexer &lexer, shParser &parser)
{
	lexer.shLIgnoreWhiteSpace(false);
	_enableIndent = false;
}

void ignoreWhiteSpace(shLexer &lexer, shParser &parser)
{
	lexer.shLIgnoreWhiteSpace(true);
	_enableIndent = true;
}

void setOverview(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;			// before finalizing the overview, close all open item lists
	stackElement elem = semantic_stack.top();
	STACK_POP;
	_docOverview = elem.text;
}

void setElementExplanation(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;			// before finalizing the explanation, close all open item lists
	stackElement explanation = semantic_stack.top();
	STACK_POP;
	stackElement onWhat = semantic_stack.top();		// stays on the stack until pop is called
	if (onWhat.meta == VARIABLEDECL)
	{
		if (_declIsGlobal)
			_docGlobalVariables.back().text += explanation.text;
		else
			_docVariables.back().text += explanation.text;
	}
	else if (onWhat.meta == FUNCTIONDECL)
	{
		if (_declIsGlobal)
			_docGlobalFunctions.back().text += explanation.text;
		else
			_docFunctions.back().text += explanation.text;
	}
	else if (onWhat.meta == MACRODECL)
		_docMacros.back().text += explanation.text;
	else if (onWhat.meta == TYPEDECL)
	{
		if (_declIsGlobal)
			_docGlobalTypes.back().text += explanation.text;
		else
			_docTypes.back().text += explanation.text;
	}
	else if (onWhat.meta == CONSTGROUPDECL)
		_docConstants.back().text += explanation.text;
	else if (onWhat.meta == CONSTDECL)
		_docConstants.back().constants.back().text += explanation.text;
	else if (onWhat.meta == INPUTDECL)
	{
		STACK_POP;
		stackElement argName = semantic_stack.top();
		STACK_POP;
		onWhat = semantic_stack.top();			// stays on the stack until pop is called
		bool matched = false;
		if (onWhat.meta == FUNCTIONDECL)
		{
			if (_declIsGlobal)
			{
				for (list<functionArgument>::iterator i = _docGlobalFunctions.back().args.begin(), end = _docGlobalFunctions.back().args.end(); i != end && !matched; ++i)
					if (i->name.html == argName.text.html)
					{
						i->text = explanation.text;
						matched = true;
					}
				if (!matched)
					if (_docType != INVALID && (_makeHtml || _makeTex))
						ERROR("No such argument \"%s\" in function: %s", argName.text.html.c_str(), _docGlobalFunctions.back().anchorName.c_str());
			}
			else
			{
				for (list<functionArgument>::iterator i = _docFunctions.back().args.begin(), end = _docFunctions.back().args.end(); i != end && !matched; ++i)
					if (i->name.html == argName.text.html)
					{
						i->text = explanation.text;
						matched = true;
					}
				if (!matched)
					if (_docType != INVALID && (_makeHtml || _makeTex))
						ERROR("No such argument \"%s\" in function: %s", argName.text.html.c_str(), _docFunctions.back().anchorName.c_str());
			}
		}
		else if (onWhat.meta == MACRODECL)
		{
			for (list<functionArgument>::iterator i = _docMacros.back().args.begin(), end = _docMacros.back().args.end(); i != end && !matched; ++i)
				if (i->name.html == argName.text.html)
				{
					i->text = explanation.text;
					matched = true;
				}
			if (!matched)
				if (_docType != INVALID && (_makeHtml || _makeTex))
					ERROR("No such argument \"%s\" in macro: %s", argName.text.html.c_str(), _docMacros.back().anchorName.c_str());
		}
		else
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("When setting input explanation, expected the stack to have a function or macro meta below input meta. "
					"The element below input had meta: %s", metaString(onWhat.meta, lexer, parser).c_str());
	}
	else if (onWhat.meta == OUTPUTDECL)
	{
		STACK_POP;
		onWhat = semantic_stack.top();			// stays on the stack until pop is called
		if (onWhat.meta == FUNCTIONDECL)
		{
			if (_declIsGlobal)
			{
				if (_docGlobalFunctions.back().output.html != "")
				{
					if (_docType != INVALID && (_makeHtml || _makeTex))
						WARNING("Duplicate output explanation for function: %s", _docGlobalFunctions.back().anchorName.c_str());
				}
				else
					_docGlobalFunctions.back().output = explanation.text;
			}
			else
			{
				if (_docFunctions.back().output.html != "")
				{
					if (_docType != INVALID && (_makeHtml || _makeTex))
						WARNING("Duplicate output explanation for function: %s", _docFunctions.back().anchorName.c_str());
				}
				else
					_docFunctions.back().output = explanation.text;
			}
		}
		else if (onWhat.meta == MACRODECL)
			if (_docMacros.back().output.html != "")
			{
				if (_docType != INVALID && (_makeHtml || _makeTex))
					WARNING("Duplicate output explanation for macro: %s", _docMacros.back().anchorName.c_str());
			}
			else
				_docMacros.back().output = explanation.text;
		else
			if (_docType != INVALID && (_makeHtml || _makeTex))
				INTERNAL("When setting output explanation, expected the stack to have a function or macro meta below output meta. "
					"The element below output had meta: %s", metaString(onWhat.meta, lexer, parser).c_str());
	}
	else
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When setting explanation, expected the stack to have an explanation and a meta. The element below top had meta: %s",
					metaString(onWhat.meta, lexer, parser).c_str());
}

void setNotice(shLexer &lexer, shParser &parser)
{
	string notice = lexer.shLCurrentToken();
	stackElement onWhat = semantic_stack.top();		// don't pop, because it is still needed for setShortExplanation
	if (onWhat.meta == VARIABLEDECL)
		if (_declIsGlobal)
			_docGlobalVariables.back().notice = notice;
		else
			_docVariables.back().notice = notice;
	else if (onWhat.meta == FUNCTIONDECL)
		if (_declIsGlobal)
			_docGlobalFunctions.back().notice = notice;
		else
			_docFunctions.back().notice = notice;
	else if (onWhat.meta == MACRODECL)
		_docMacros.back().notice = notice;
	else if (onWhat.meta == TYPEDECL)
		if (_declIsGlobal)
			_docGlobalTypes.back().notice = notice;
		else
			_docTypes.back().notice = notice;
	else if (onWhat.meta == CONSTGROUPDECL)
		_docConstants.back().notice = notice;
	else if (onWhat.meta == CONSTDECL)
		_docConstants.back().constants.back().notice = notice;
	else
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When setting notice, expected the stack to have a meta. The top element had meta: %s",
					metaString(onWhat.meta, lexer, parser).c_str());
}

void setShortExplanation(shLexer &lexer, shParser &parser)
{
	stackElement explanation = semantic_stack.top();
	STACK_POP;
	explanation.text.html = removeFirstAndLastTag(explanation.text.html, lexer, parser);
	stackElement onWhat = semantic_stack.top();		// don't pop, because it is still needed for setElementExplanation
	if (onWhat.meta == VARIABLEDECL)
		if (_makeSummary)
			if (_declIsGlobal)
				_docGlobalVariables.back().summary = explanation.text;
			else
				_docVariables.back().summary = explanation.text;
		else
			if (_declIsGlobal)
				_docGlobalVariables.back().text = explanation.text;
			else
				_docVariables.back().text = explanation.text;
	else if (onWhat.meta == FUNCTIONDECL)
		if (_makeSummary)
			if (_declIsGlobal)
				_docGlobalFunctions.back().summary = explanation.text;
			else
				_docFunctions.back().summary = explanation.text;
		else
			if (_declIsGlobal)
				_docGlobalFunctions.back().text = explanation.text;
			else
				_docFunctions.back().text = explanation.text;
	else if (onWhat.meta == MACRODECL)
		if (_makeSummary)
			_docMacros.back().summary = explanation.text;
		else
			_docMacros.back().text = explanation.text;
	else if (onWhat.meta == TYPEDECL)
		if (_makeSummary)
			if (_declIsGlobal)
				_docGlobalTypes.back().summary = explanation.text;
			else
				_docTypes.back().summary = explanation.text;
		else
			if (_declIsGlobal)
				_docGlobalTypes.back().text = explanation.text;
			else
				_docTypes.back().text = explanation.text;
	else if (onWhat.meta == CONSTGROUPDECL)
		if (_makeSummary)
			_docConstants.back().summary = explanation.text;
		else
			_docConstants.back().text = explanation.text;
	else if (onWhat.meta == CONSTDECL)
		if (_makeSummary)
			_docConstants.back().constants.back().summary = explanation.text;
		else
			_docConstants.back().constants.back().text = explanation.text;
	else
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When setting short explanation, expected the stack to have an explanation and a meta. The element below top had meta: %s",
					metaString(onWhat.meta, lexer, parser).c_str());
}

void checkCompleteness(shLexer &lexer, shParser &parser)
{
	stackElement onWhat = semantic_stack.top();		// don't pop, because it is still needed for setElementExplanation
	if (onWhat.meta == FUNCTIONDECL)
	{
		if (_declIsGlobal)
		{
			for (list<functionArgument>::iterator i = _docGlobalFunctions.back().args.begin(), end = _docGlobalFunctions.back().args.end(); i != end; ++i)
				if (i->text.html == "")
					if (_docType != INVALID && (_makeHtml || _makeTex))
						ERROR("Missing explanation for input argument \"%s\" in function: %s", i->name.html.c_str(), _docGlobalFunctions.back().name.html.c_str());
			if (_docGlobalFunctions.back().output.html == "" && _docGlobalFunctions.back().type.html != "void" && _docGlobalFunctions.back().type.html != "")
				if (_docType != INVALID && (_makeHtml || _makeTex))
					ERROR("Missing explanation for output of non-void function: %s", _docGlobalFunctions.back().name.html.c_str());
		}
		else
		{
			for (list<functionArgument>::iterator i = _docFunctions.back().args.begin(), end = _docFunctions.back().args.end(); i != end; ++i)
				if (i->text.html == "")
					if (_docType != INVALID && (_makeHtml || _makeTex))
						ERROR("Missing explanation for input argument \"%s\" in function: %s", i->name.html.c_str(), _docFunctions.back().name.html.c_str());
			if (_docFunctions.back().output.html == "" && _docFunctions.back().type.html != "void" && _docFunctions.back().type.html != "")
				if (_docType != INVALID && (_makeHtml || _makeTex))
					ERROR("Missing explanation for output of non-void function: %s", _docFunctions.back().name.html.c_str());
		}
	}
	else if (onWhat.meta == MACRODECL)
	{
		for (list<functionArgument>::iterator i = _docMacros.back().args.begin(), end = _docMacros.back().args.end(); i != end; ++i)
			if (i->text.html == "")
				if (_docType != INVALID && (_makeHtml || _makeTex))
					ERROR("Missing explanation for input argument \"%s\" in macro: %s", i->name.html.c_str(), _docMacros.back().name.html.c_str());
		if (_docMacros.back().output.html == "" && _docMacros.back().type.html != "")
			if (_docType != INVALID && (_makeHtml || _makeTex))
				ERROR("Missing explanation for output of macro: %s", _docMacros.back().name.html.c_str());
	}
	else
		if (_docType != INVALID && (_makeHtml || _makeTex))
			INTERNAL("When checking for completeness of definition, expected the stack to have a meta function or macro meta. "
				"The element on top had meta: %s", metaString(onWhat.meta, lexer, parser).c_str());
}

void increaseIndent1(shLexer &lexer, shParser &parser)
{
	_indentLevel += 1;
}
void decreaseIndent1(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;
	_indentLevel -= 1;
}

void increaseIndent2(shLexer &lexer, shParser &parser)
{
	_indentLevel += 2;
}
void decreaseIndent2(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;
	_indentLevel -= 2;
}

void increaseIndent3(shLexer &lexer, shParser &parser)
{
	_indentLevel += 3;
}
void decreaseIndent3(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;
	_indentLevel -= 3;
}

void increaseIndent4(shLexer &lexer, shParser &parser)
{
	_indentLevel += 4;
}
void decreaseIndent4(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;
	_indentLevel -= 4;
}

void increaseIndent5(shLexer &lexer, shParser &parser)
{
	_indentLevel += 5;
}
void decreaseIndent5(shLexer &lexer, shParser &parser)
{
	CLOSE_ITEMS;
	_indentLevel -= 5;
}

void addToGlobalList(shLexer &lexer, shParser &parser)
{
	stackElement elem = semantic_stack.top();
	STACK_POP;
	for (list<docGlobalItem>::const_iterator i = _globalsList.begin(), end = _globalsList.end(); i != end; ++i)
		if (i->filename == elem.text.plain)
		{
			if (_docType != INVALID && (_makeHtml || _makeTex))
				WARNING("Duplicate file listed");
			return;
		}
	docGlobalItem item;
	item.filename = elem.text.plain;
	item.line = lexer.shLLine();
	item.column = lexer.shLColumn();
	_globalsList.push_back(item);
}

void documentHeader(shLexer &lexer, shParser &parser)
{
	stackElement title = semantic_stack.top();
	STACK_POP;
	if (_author == "")
		ERROR("It is necessary that you provide the name of the author or last editor of the documentation file");
	if (_version == "")
		ERROR("It is necessary to provide the version of the library/program the documentation file is updated to");
	if (!_evaluate)
		return;
	if (!semantic_stack.empty())
		INTERNAL("When generating the header, there should have been only one element in the stack, but there are more");
	string subtitle = "";
	string pageon = "";
	switch (_docType)
	{
	case INDEX:
		subtitle = "Welcome";
		pageon = "Index page";
		break;
	case CLASS:
		subtitle = _docName.html+" Class";
		pageon = "Page on class "+_docName.html;
		break;
	case STRUCT:
		subtitle = _docName.html+" Structure";
		pageon = "Page on struct "+_docName.html;
		break;
	case FUNCTIONS:
		subtitle = _docName.html+" Functions";
		pageon = "Page on "+_docName.html+" functions";
		break;
	case VARIABLES:
		subtitle = _docName.html+" Variables";
		pageon = "Page on "+_docName.html+" variables";
		break;
	case API:
		subtitle = _docName.html;
		pageon = "Page on "+_docName.html;
		break;
	case IOFILE:
		subtitle = _docName.html+" I/O File";
		pageon = "Page on "+_docName.html+" I/O file";
		break;
	case CONSTANTS:
		subtitle = "List of Constants";
		pageon = "Page on constants";
		break;
	case GLOBALS:
		subtitle = "Global List of Identifiers";
		pageon = "Page on globals";
		break;
	case EXAMPLE:
		subtitle = _docName.html+" Example";
		pageon = "Page on "+_docName.html+" example";
		break;
	case OTHER:
		subtitle = _docName.html;
		pageon = "Page on "+_docName.html;
		break;
	default:
		INTERNAL("When generating the header, the document type is not properly set. Value is: %d", (int)_docType);
		break;
	}
	time_t now_sec;
	time(&now_sec);
	struct tm *now;
	now = localtime(&now_sec);
	char date[30];
	strftime(date, 30, "%b %e, %Y", now);
	formattedText header;
	string titleWithoutTag = removeTags(title.text.html, lexer, parser);	// Title would be in this form: <a id="heading"></a><h1>heading</h1>
	_docTitle = title.text;
	if (_makeHtml)
	{
		header.html = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
				"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
				"<head>\n"
				"\t<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />\n"
				"\t<title>"+titleWithoutTag+" - "+subtitle+"</title>\n"
				"\t<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />\n";
		header.html += "\t<meta name=\"author\" content=\""+fixHtml(_author, lexer, parser)+"\" />\n";
		header.html += "\t<meta name=\"revise-date\" content=\""+string(date)+"\" />\n"
			"\t<meta name=\"description\" content=\"Documentation on "+titleWithoutTag;
		header.html += ", version "+_version;
		header.html += ". "+pageon+".\" />\n";
		if (_keywords.size() == 0)
			WARNING("It is recommended that you provide a list of keywords for the documentation file");
		else
		{
			header.html += "\t<meta name=\"keywords\" content=\"";
			for (list<string>::const_iterator i = _keywords.begin(), end = _keywords.end(); i != end; ++i)
				header.html += (i == _keywords.begin()?"":", ")+*i;
			header.html += "\" />\n";
		}
		header.html += "</head>\n"
			"<body>\n"
			"<div id=\"overall-layout\">\n"
			"\t<div id=\"title\">\n"
			+title.text.html+
			"\t\t<h5 id=\"title-subheading\">"+_version+"</h5>\n"
			"\t</div>\n";
		if (_previous.plain != "" || _next.plain != "" || _shortcuts.size() != 0)
		{
			header.html += "\t<div id=\"header\" class=\"header-bottom\">\n"
					"\t\t<div id=\"previous-page-header\" class=\"header-bottom\">\n";
			if (_previous.plain != "")
				header.html += "\t\t\t<a href=\""+removeSpace(_previous.plain)+".html\">Previous:</a><br />"+_previous.html+"\n";
			header.html += "\t\t</div>\n"
				"\t\t<div id=\"shortcuts-header\" class=\"header-bottom\">\n";
			for (list<string>::const_iterator i = _shortcuts.begin(), end = _shortcuts.end(); i != end; ++i)
				header.html += (i == _shortcuts.begin()?"\t\t\t":"\t\t\t| ")+("<a href=\""+removeSpace(*i))+".html\">"+
					(*i == "index"?string("Home"):*i == "globals"?string("Globals"):*i == "constants"?string("Constants"):*i)+"</a>\n";
			header.html += "\t\t</div>\n"
				"\t\t<div id=\"next-page-header\" class=\"header-bottom\">\n";
			if (_next.plain != "")
				header.html += "\t\t\t<a href=\""+removeSpace(_next.plain)+".html\">Next:</a><br />"+_next.html+"\n";
			header.html += "\t\t</div>\n"
				"\t</div>\n";
		}
		header.html += "\t<div class=\"hr\"></div>\n"
				"\t<div id=\"main-body\">\n";
	}

	if (_makeTex)
		header.tex = "";
	formattedText firstHeading;
	switch (_docType)
	{
	case INDEX:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n";
		//firstHeading.tex = "\\section{"+titleWithoutTag+"}\\label{index}\n";		// No need to write the title of the page because the title also goes
												// on the first page
		firstHeading.tex = "\\phantomsection\n\\label{index}\\label{index Overview}\n";
		break;
	case CLASS:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1><code class=\"inHeading\">"+_docName.html+
			"</code> Class</h1>\n";
		firstHeading.tex = "\\section{\\code{"+_docName.tex+"} Class}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+
			" Overview}\n";
		break;
	case STRUCT:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1><code class=\"inHeading\">"+_docName.html+
			"</code> Structure</h1>\n";
		firstHeading.tex = "\\section{\\code{"+_docName.tex+"} Structure}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+
			" Overview}\n";
		break;
	case FUNCTIONS:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1><code class=\"inHeading\">"+_docName.html+
			"</code> Functions</h1>\n";
		firstHeading.tex = "\\section{\\code{"+_docName.tex+"} Functions}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+
			" Overview}\n";
		break;
	case VARIABLES:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1><code class=\"inHeading\"e>"+_docName.html+
			"</code> Variables</h1>\n";
		firstHeading.tex = "\\section{\\code{"+_docName.tex+"} Variables}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+
			" Overview}\n";
		break;
	case API:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1>"+_docName.html+"</h1>\n";
		firstHeading.tex = "\\section{"+_docName.tex+"}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+" Overview}\n";
		break;
	case IOFILE:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1>"+_docName.html+"</h1>\n";
		firstHeading.tex = "\\section{"+_docName.tex+"}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+" Overview}\n";
		break;
	case CONSTANTS:
		firstHeading.html = "\t<h1>List of Constants</h1>\n";
		firstHeading.tex = "\\section{List of Constants}\\label{constants}\n";
		break;
	case GLOBALS:
		firstHeading.html = "\t<h1>Global List of Identifiers</h1>\n";
		firstHeading.tex = "\\section{Global List of Identifiers}\\label{globals}\n";
		break;
	case EXAMPLE:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1>"+_docName.html+"</h1>\n";
		firstHeading.tex = "\\section{"+_docName.tex+"}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+" Overview}\n";
		break;
	case OTHER:
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1>"+_docName.html+"</h1>\n";
		firstHeading.tex = "\\section{"+_docName.tex+"}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+" Overview}\n";
		break;
	default:
		// Error already given
		firstHeading.html = "\t<a id=\"Overview\"></a>\n\t<h1>"+_docName.html+"</h1>\n";
		firstHeading.tex = "\\section{"+_docName.tex+"}\\label{"+fixTexAnchor(_docName.plain)+"}\\label{"+fixTexAnchor(_docName.plain)+" Overview}\n";
		break;
	}
	header.html += firstHeading.html;
	header.tex += firstHeading.tex;
	_docHeader = header;
	_duringHeader = false;
}

void documentBottom(shLexer &lexer, shParser &parser)
{
	if (!_evaluate)
		return;
	formattedText bottom;
	if (_makeHtml)
	{
		if (_seealsos.size() != 0)
		{
			bottom.html = INDENT("<h2>See Also:</h2>\n");
			++_indentLevel;
			for (list<string>::const_iterator i = _seealsos.begin(), end = _seealsos.end(); i != end; ++i)
				bottom.html += (i == _seealsos.begin()?"":",\n")+INDENT(*i);
			--_indentLevel;
			bottom.html += "\n";
		}
		if (_previous.plain != "" || _next.plain != "" || _shortcuts.size() != 0)
		{
			bottom.html += "\t</div>\n"
					"\t<div class=\"hr\"></div>\n";
			bottom.html += "\t<div id=\"bottom\" class=\"header-bottom\">\n"
					"\t\t<div id=\"previous-page-bottom\" class=\"header-bottom\">\n";
			if (_previous.plain != "")
				bottom.html += "\t\t\t<a href=\""+removeSpace(_previous.plain)+".html\">Previous:</a><br />"+_previous.html+"\n";
			bottom.html += "\t\t</div>\n"
				"\t\t<div id=\"shortcuts-bottom\" class=\"header-bottom\">\n";
			for (list<string>::const_iterator i = _shortcuts.begin(), end = _shortcuts.end(); i != end; ++i)
				bottom.html += (i == _shortcuts.begin()?"\t\t\t":"\t\t\t| ")+("<a href=\""+removeSpace(*i))+".html\">"+
					(*i == "index"?string("Home"):*i == "globals"?string("Globals"):*i == "constants"?string("Constants"):*i)+"</a>\n";
			bottom.html += "\t\t</div>\n"
				"\t\t<div id=\"next-page-bottom\" class=\"header-bottom\">\n";
			if (_next.plain != "")
				bottom.html += "\t\t\t<a href=\""+removeSpace(_next.plain)+".html\">Next:</a><br />"+_next.html+"\n";
			bottom.html += "\t\t</div>\n"
				"\t</div>\n";
		}
		bottom.html += "\t<div><p id=\"docthis\">"
				"<i>This documentation page has been generated by</i> <b>DocThis! Documentation Generator</b> <i>program.</i>"
				"</p></div>\n"
				"</div>\n"
				"</body>\n"
				"</html>\n";
	}
	if (_makeTex)
		bottom.tex = "\\textcolor[rgb]{0.5, 0.5, 0.5}{\\footnotesize{\\textit{Generated by} \\textbf{DocThis! Documentation Generator} \\textit{program.}}}\n";
	_docBottom = bottom;
}

static void output(ofstream &htmlout, ofstream &texout, const formattedText &s)
{
	if (_makeHtml)
		htmlout << s.html;
	if (_makeTex)
		texout << s.tex;
}

static void outputElementsSummary(ofstream &htmlout, ofstream &texout, const list<docElement> &elems, shLexer &lexer, shParser &parser)
{
	for (list<docElement>::const_iterator i = elems.begin(), end = elems.end(); i != end; ++i)
	{
		if (_makeHtml)
		{
			htmlout << INDENT("<p class=\"overview\"><code class=\"overview\"><a class=\"overview\" href=\"#"+
					fixHtmlAnchor(i->anchorName)+"\">"+i->name.html+"</a>");
			if (i->hasArgs)
			{
				htmlout << "(";
				for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					htmlout << (j == i->args.begin()?"":", ") << (i->hasType?j->type.html:j->name.html);
				htmlout << ")";
			}
			if (i->type.html != "")
				htmlout << ": " << i->type.html;
			htmlout << "</code>";
			if (i->notice != "")
				htmlout << "<span class=\"noticeSummary\">" << fixHtml(i->notice, lexer, parser) << "</span>";
			htmlout << "</p>" << endl;
			htmlout << INDENT("<p class=\"shortDesc\">"+i->summary.html+"</p>") << endl;
		}
		if (_makeTex)
		{
			texout << "\\noindent \\code{\\ulhyperref{"+fixTexAnchor(_docName.plain)+" "+fixTexAnchor(i->anchorName)+"}{"+i->name.tex+"}";
			if (i->hasArgs)
			{
				texout << "}\\code{(";
				for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					texout << (j == i->args.begin()?"":",} \\code{") << (i->hasType?j->type.tex:j->name.tex);
				texout << ")";
			}
			if (i->type.tex != "")
				texout << ":} \\code{"+i->type.tex;
			texout << "}";
			if (i->notice != "")
				texout << " \\notice{"+fixTex(i->notice, lexer, parser)+"}";
			texout << "\\\\" << endl;
			texout << "\\hangindent=0.5cm " << i->summary.tex << endl << endl;
		}
	}
}

static string trim(const string &s)
{
	string res;
	string spaces;
	unsigned int i, size;
	for (i = 0, size = s.size(); i < size && (s[i] == ' ' || s[i] == '\t'); ++i);		// ignore initial whitespace
	for (; i < size; ++i)
		if (s[i] == ' ' || s[i] == '\t')
			spaces += s[i];
		else
		{
			res += spaces;								// This way, final spaces are also ignored
			spaces = "";
			res += s[i];
		}
	return res;
}

static string fixHtmlNotice(const string &s)
{
	string res = "";
	istringstream sin(s);
	string line;
	bool startParagraph = true;

	while (getline(sin, line))
	{
		string l = trim(line);
		if (l == "")
		{
			if (!startParagraph)
				res += "</p>\n";
			startParagraph = true;
			continue;
		}
		if (startParagraph)
			res += INDENT("<p>");
		startParagraph = false;
		res += "\n" + INDENT(l);
	}
	if (!startParagraph)
		res += "</p>";
	return res;
}

static string fixTexNotice(const string &s)
{
	string res = "";
	istringstream sin(s);
	string line;
	bool startParagraph = true;

	while (getline(sin, line))
	{
		string l = trim(line);
		if (l == "")
		{
			if (!startParagraph)
				res += "}\n\n";
			startParagraph = true;
			continue;
		}
		if (startParagraph)
			res += "\\notice{";
		startParagraph = false;
		res += "\n" + l;
	}
	if (!startParagraph)
		res += "}";
	return res;
}

static void outputElements(ofstream &htmlout, ofstream &texout, const list<docElement> &elems, shLexer &lexer, shParser &parser)
{
	// The definitions are written in a table that looks like this (shape of table based on properties):
	// If !hasArgs && !hasType
	//   name
	// If !hasArgs && hasType
	//   name: type
	// If hasArgs && nargs == 0
	//   name()
	// If hasArgs && nargs == 0
	//   name(): type
	// If hasArgs && nargs == 1 && !hasType && type == ""
	//   name(arg)
	// If hasArgs && nargs == 1 && !hasType && type != ""
	//   name(arg): type
	// If hasArgs && nargs > 1 && !hasType && type == ""
	//   name(arg,
	//        arg)
	// If hasArgs && nargs > 1 && !hasType && type != ""
	//   name(arg,
	//        arg
	//       ): type
	// If hasArgs && nargs == 1 && hasType && type == ""
	//   name(name:  arg[   = default])
	// If hasArgs && nargs == 1 && hasType && type != ""
	//   name(name:  arg[   = default]): type
	// If hasArgs && nargs > 1 && hasType && type == ""
	//   name(name:  arg,
	//        name:  arg    = default,
	//        name:  arg    = default)
	// If hasArgs && nargs > 1 && hasType && type != ""
	//   name(name:  arg,
	//        name:  arg    = default,
	//        name:  arg    = default
	//       ): type
	htmlout << INDENT("<div class=\"def-list\">") << endl;
	++_indentLevel;
	for (list<docElement>::const_iterator i = elems.begin(), end = elems.end(); i != end; ++i)
	{
		if (_makeHtml)
		{
			htmlout << INDENT("<a id=\""+fixHtmlAnchor(i->anchorName)+"\"></a>") << endl;
			htmlout << INDENT("<div class=\"def-id\">") << endl;
			++_indentLevel;
			htmlout << INDENT("<table class=\"def\" width=\"100%\">") << endl;
			++_indentLevel;
			htmlout << INDENT("<tr>") << endl;
			++_indentLevel;
			if (!i->hasArgs || i->args.size() <= 1)
			{
				htmlout << INDENT("<td class=\"def\"><code class=\"def\">"+i->name.html);
				if (i->hasArgs)
					htmlout << "(";
				if (i->args.size() == 1)
				{
					htmlout << i->args.front().name.html;
					if (i->hasType)
					{
						htmlout << ": " << i->args.front().type.html;
						if (i->args.front().defaultValue.html != "")
							htmlout << " = " << i->args.front().defaultValue.html;
					}
				}
				if (i->hasArgs)
					htmlout << ")";
				if (i->type.html != "")
					htmlout << ": " << i->type.html;
				htmlout << "</code></td>" << endl;
			}
			else
			{
				htmlout << INDENT("<td class=\"def\"><code class=\"def\">"+i->name.html+"</code></td>") << endl;
				htmlout << INDENT("<td class=\"def\"><code class=\"def\">(</code></td>") << endl;
				if (i->hasType)
				{
					htmlout << INDENT("<td class=\"def\"><code class=\"def\">");
					list<functionArgument>::const_iterator lastj;
					for (list<functionArgument>::const_iterator j = lastj = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					{
						if (j != i->args.begin())
						{
							if (lastj->defaultValue.html != "")
							{
								htmlout << "</code></td>" << endl;
								htmlout << INDENT("<td class=\"defDefValue\"><code class=\"def\">= "+
									lastj->defaultValue.html+",</code></td>") << endl;	// todo: add space before `=`
							}
							else
							{
								htmlout << ",</code></td>" << endl;
								htmlout << INDENT("<td class=\"defLastColumn\"></td>") << endl;
							}
							--_indentLevel;
							htmlout << INDENT("</tr>") << endl;
							htmlout << INDENT("<tr>") << endl;
							++_indentLevel;
							htmlout << INDENT("<td class=\"def\"></td>") << endl;
							htmlout << INDENT("<td class=\"def\"></td>") << endl;
							htmlout << INDENT("<td class=\"def\"><code class=\"def\">");
						}
						htmlout << j->name.html << "</code></td>" << endl;
						htmlout << INDENT("<td class=\"def\"><code class=\"def\">: "+j->type.html);
						lastj = j;
					}
					if (i->type.html == "")
						if (lastj->defaultValue.html != "")
						{
							htmlout << "</code></td>" << endl;
							htmlout << INDENT("<td class=\"defDefValue\"><code class=\"def\">= "+
								lastj->defaultValue.html+")</code></td>") << endl;	// todo: add space before `=`
						}
						else
						{
							htmlout << ")</code></td>" << endl;
							htmlout << INDENT("<td class=\"defLastColumn\"></td>") << endl;
						}
					else
					{
						htmlout << "</code></td>" << endl;
						if (lastj->defaultValue.html != "")
							htmlout << INDENT("<td class=\"defDefValue\"><code class=\"def\">= "+
								lastj->defaultValue.html+"</code></td>") << endl;	// todo: add space before `=`
						else
							htmlout << INDENT("<td class=\"defLastColumn\"></td>") << endl;
						--_indentLevel;
						htmlout << INDENT("</tr>") << endl;
						htmlout << INDENT("<tr>") << endl;
						++_indentLevel;
						htmlout << INDENT("<td class=\"def\"></td>") << endl;
						htmlout << INDENT("<td class=\"def\"><code class=\"def\">)</code></td>") << endl;
						htmlout << INDENT("<td class=\"defRetValue\" colspan=\"3\"><code class=\"def\">: "+i->type.html+
								"</code></td>") << endl;
					}
				}
				else
				{
					htmlout << INDENT("<td class=\"defLastColumn\"><code class=\"def\">");
					for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					{
						if (j != i->args.begin())
						{
							htmlout << ",</code></td>" << endl;
							--_indentLevel;
							htmlout << INDENT("</tr>") << endl;
							htmlout << INDENT("<tr>") << endl;
							++_indentLevel;
							htmlout << INDENT("<td class=\"def\"></td>") << endl;
							htmlout << INDENT("<td class=\"def\"></td>") << endl;
							htmlout << INDENT("<td class=\"defLastColumn\"><code class=\"def\">");
						}
						htmlout << j->name.html;
					}
					if (i->type.html == "")
						htmlout << ")</code></td>" << endl;
					else
					{
						htmlout << "</code></td>" << endl;
						--_indentLevel;
						htmlout << INDENT("</tr>") << endl;
						htmlout << INDENT("<tr>") << endl;
						++_indentLevel;
						htmlout << INDENT("<td class=\"def\"></td>") << endl;
						htmlout << INDENT("<td class=\"def\"><code class=\"def\">)</code></td>") << endl;
						htmlout << INDENT("<td class=\"defRetValue\"><code class=\"def\">: "+i->type.html+"</code></td>") << endl;
					}
				}
			}
			--_indentLevel;
			htmlout << INDENT("</tr>") << endl;
			--_indentLevel;
			htmlout << INDENT("</table>") << endl;
			--_indentLevel;
			htmlout << INDENT("</div>") << endl;
			htmlout << INDENT("<div class=\"def-explanation\">") << endl;
			++_indentLevel;
			if (i->notice != "")
			{
				string notice = i->notice;
				if (_notices.find(notice) != _notices.end())
					notice = _notices[notice];
				htmlout << INDENT("<div class=\"notice\">") << endl;
				++_indentLevel;
				htmlout << fixHtmlNotice(notice) << endl;
				--_indentLevel;
				htmlout << INDENT("</div>") << endl;
			}
			if (i->hasArgs)
			{
				htmlout << INDENT("<h3>Inputs</h3>") << endl;
				++_indentLevel;
				if (i->args.size() == 0)
					htmlout << INDENT("<p>None</p>") << endl;
				else
				{
					htmlout << INDENT("<ul class=\"defInput\">") << endl;
					++_indentLevel;
					for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					{
						if (j == i->args.begin())
							htmlout << INDENT("");
						htmlout << "<li><p class=\"defInput\"><code class=\"defInput\">"+j->name.html+"</code></p>" << endl;
						htmlout << j->text.html;
						htmlout << INDENT("</li>");
					}
					if (i->args.size() > 0)
						htmlout << endl;
					--_indentLevel;
					htmlout << INDENT("</ul>") << endl;
				}
				--_indentLevel;
				if (i->output.html != "")
				{
					htmlout << INDENT("<h3>Output</h3>") << endl;
					htmlout << i->output.html << endl;
				}
				htmlout << INDENT("<h3>Description</h3>") << endl;
			}
			htmlout << i->text.html << endl;
			--_indentLevel;
			htmlout << INDENT("</div>") << endl;
		}
		if (_makeTex)
		{
			if (i != elems.begin())
				texout << "\\begin{center}\n"
					"\\rule{0.75\\linewidth}{0.1mm}\n"
					"\\end{center}" << endl << endl;
			texout << "\\phantomsection\n\\label{"+fixTexAnchor(_docName.plain)+" "+fixTexAnchor(i->anchorName)+"}" << endl;
			if (!i->hasArgs || i->args.size() <= 1)
			{
				texout << "\\large \\begin{tabular}{@{}l}" << endl;
				texout << "\\code{"+i->name.tex;
				if (i->hasArgs)
					texout << "(";
				if (i->args.size() == 1)
				{
					texout << i->args.front().name.tex;
					if (i->hasType)
					{
						texout << ": " << i->args.front().type.tex;
						if (i->args.front().defaultValue.tex != "")
							texout << " = " << i->args.front().defaultValue.tex;
					}
				}
				if (i->hasArgs)
					texout << ")";
				if (i->type.tex != "")
					texout << ": " << i->type.tex;
				texout << "}";
			}
			else
			{
				if (i->hasType)
				{
					texout << "\\large \\begin{tabular}{@{}l@{}l@{}l@{}l l}" << endl;
					texout << "\\code{"+i->name.tex+"} & \\code{(}";
					texout << " & \\code{";
					list<functionArgument>::const_iterator lastj;
					for (list<functionArgument>::const_iterator j = lastj = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					{
						if (j != i->args.begin())
						{
							if (lastj->defaultValue.tex != "")
								texout << "} & \\code{= "+lastj->defaultValue.tex+",}";
							else
								texout << ",} &";
							texout << " \\\\" << endl;
							texout << "& & \\code{";
						}
						texout << j->name.tex << "}";
						texout << " & \\code{: "+j->type.tex;
						lastj = j;
					}
					if (i->type.tex == "")
					{
						if (lastj->defaultValue.tex != "")
							texout << "} &\\code{= "+lastj->defaultValue.tex+")}";
						else
							texout << ")} &";
					}
					else
					{
						texout << "} \\\\" << endl;
						texout << "& \\code{)} & \\multicolumn{3}{@{}l}{\\code{: "+i->type.tex+"}}";
					}
				}
				else
				{
					texout << "\\large \\begin{tabular}{@{}l@{}l@{}l}" << endl;
					texout << "\\code{"+i->name.tex+"} & \\code{(}";
					texout << " & \\code{";
					for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
					{
						if (j != i->args.begin())
						{
							texout << ",} \\\\" << endl;
							texout << "& & \\code{";
						}
						texout << j->name.tex;
					}
					if (i->type.tex == "")
						texout << ")}";
					else
					{
						texout << "} \\\\" << endl;
						texout << "& \\code{)} & \\code{: "+i->type.tex+"}";
					}
				}
			}
			texout << endl << "\\end{tabular} \\normalsize" << endl << endl;
			if (i->notice != "")
			{
				string notice = i->notice;
				if (_notices.find(notice) != _notices.end())
					notice = _notices[notice];
				texout << fixTexNotice(fixTex(notice, lexer, parser)) << endl << endl;
			}
			if (i->hasArgs)
			{
				texout << "\\subsubsection*{Inputs}" << endl;
				if (i->args.size() == 0)
					texout << "None" << endl;
				else
				{
					texout << "\\begin{itemize}" << endl;
					for (list<functionArgument>::const_iterator j = i->args.begin(), argsend = i->args.end(); j != argsend; ++j)
						texout << "\\item \\code{"+j->name.tex+"} - "+j->text.tex << endl;
					texout << "\\end{itemize}" << endl;
				}
				if (i->output.tex != "")
				{
					texout << "\\subsubsection*{Output}" << endl;
					texout << i->output.tex << endl;
				}
				texout << "\\subsubsection*{Description}" << endl;
			}
			texout << i->text.tex << endl;
		}
	}
	--_indentLevel;
	htmlout << INDENT("</div>") << endl;
}

void writeDocument(shLexer &lexer, shParser &parser)
{
	if (!_evaluate)
		return;
	CLOSE_ITEMS;			// before writing the document, close all open item lists
	if (!_makeHtml && !_makeTex)
		return;
	if (_docType == GLOBALS)
		return;
	string filename_html = "generated/html/";
	string filename_tex = "generated/tex/";
	switch (_docType)
	{
	case INDEX:
	case CLASS:
	case STRUCT:
	case FUNCTIONS:
	case VARIABLES:
	case API:
	case IOFILE:
	case CONSTANTS:
	case GLOBALS:
	case EXAMPLE:
	case OTHER:
		filename_html += removeSpace(_docName.plain)+".html";
		filename_tex += removeSpace(_docName.plain)+".tex";
		break;
	default:
		INTERNAL("When determining document name, the document type is not properly set. Value is: %d", (int)_docType);
		filename_html += removeSpace(_docName.plain)+".html";
		filename_tex += removeSpace(_docName.plain)+".tex";
		break;
	}
	ofstream htmlout;
	ofstream texout;
	if (_makeHtml)
	{
		htmlout.open(filename_html.c_str());
		if (!htmlout.is_open())
		{
			ERROR("Could not open file for writing. File name: %s", filename_html.c_str());
			_makeHtml = false;
		}
	}
	if (_makeTex)
	{
		texout.open(filename_tex.c_str());
		if (!texout.is_open())
		{
			ERROR("Could not open file for writing. File name: %s", filename_tex.c_str());
			_makeTex = false;
		}
	}
	output(htmlout, texout, _docHeader);
	output(htmlout, texout, _docOverview);
	switch (_docType)
	{
	case CLASS:
	case STRUCT:
	case FUNCTIONS:
	case VARIABLES:
	case API:
		// Although each of these have recommended elements, it is the choice of documentor to have either
		// of the elements in either type of documentation
		// First the summaries
		if (_makeSummary)
		{
			if (!_docGlobalTypes.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Defined Types:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Defined Types}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docGlobalTypes, lexer, parser);
				--_indentLevel;
			}
			if (!_docMacros.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Defined Macros:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Defined Macros}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docMacros, lexer, parser);
				--_indentLevel;
			}
			if (!_docTypes.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Internal Types:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Internal Types}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docTypes, lexer, parser);
				--_indentLevel;
			}
			if (!_docVariables.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Member Variables:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Member Variables}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docVariables, lexer, parser);
				--_indentLevel;
			}
			if (!_docFunctions.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Member Functions:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Member Functions}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docFunctions, lexer, parser);
				--_indentLevel;
			}
			if (!_docGlobalVariables.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Global Variables:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Global Variables}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docGlobalVariables, lexer, parser);
				--_indentLevel;
			}
			if (!_docGlobalFunctions.empty())
			{
				if (_makeHtml) htmlout << INDENT("<h3>Global Functions:</h3>") << endl;
				if (_makeTex) texout << "\\subsubsection*{Global Functions}" << endl;
				++_indentLevel;
				outputElementsSummary(htmlout, texout, _docGlobalFunctions, lexer, parser);
				--_indentLevel;
			}
		}
		// Now the details
		if (_makeDetails)
		{
			if (_makeHtml) htmlout << INDENT("<div class=\"hr\"></div>") << endl;
			if (!_docGlobalTypes.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Defined Types:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Defined Types}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docGlobalTypes, lexer, parser);
				--_indentLevel;
			}
			if (!_docMacros.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Defined Macros:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Defined Macros}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docMacros, lexer, parser);
				--_indentLevel;
			}
			if (!_docTypes.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Internal Types:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Internal Types}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docTypes, lexer, parser);
				--_indentLevel;
			}
			if (!_docVariables.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Member Variables:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Member Variables}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docVariables, lexer, parser);
				--_indentLevel;
			}
			if (!_docFunctions.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Member Functions:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Member Functions}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docFunctions, lexer, parser);
				--_indentLevel;
			}
			if (!_docGlobalVariables.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Global Variables:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Global Variables}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docGlobalVariables, lexer, parser);
				--_indentLevel;
			}
			if (!_docGlobalFunctions.empty())
			{
				if (_makeTex) texout << "\\rule{0.75\\linewidth}{0.3mm}\n" << endl;
				if (_makeHtml) htmlout << INDENT("<h2>Global Functions:</h2>") << endl;
				if (_makeTex) texout << "\\subsection*{Global Functions}" << endl;
				++_indentLevel;
				outputElements(htmlout, texout, _docGlobalFunctions, lexer, parser);
				--_indentLevel;
			}
		}
		break;
	case CONSTANTS:
		if (!_docConstants.empty())
		{
			if (_makeSummary)
			{
				for (list<constantGroup>::const_iterator i = _docConstants.begin(), end = _docConstants.end(); i != end; ++i)
				{
					if (_makeHtml) htmlout << INDENT("<h3>"+i->name.html+"</h3>") << endl;
					if (_makeHtml) htmlout << INDENT("<p class=\"groupShortDesc\">"+i->summary.html+"</p>") << endl;
					if (_makeTex) texout << "\\subsubsection*{"+i->name.tex+"}" << endl;
					if (_makeTex) texout << i->summary.tex << endl << endl;
					++_indentLevel;
					outputElementsSummary(htmlout, texout, i->constants, lexer, parser);
					--_indentLevel;
				}
			}
			if (_makeDetails)
			{
				if (_makeHtml) htmlout << INDENT("<div class=\"hr\"></div>") << endl;
				for (list<constantGroup>::const_iterator i = _docConstants.begin(), end = _docConstants.end(); i != end; ++i)
				{
					if (_makeTex) texout << "\\begin{center}\n"
						"\\rule{0.75\\linewidth}{0.3mm}\n"
						"\\end{center}" << endl;
					if (_makeHtml) htmlout << INDENT("<a id=\""+fixHtmlAnchor(i->name.plain)+"\"></a>") << endl;
					if (_makeHtml) htmlout << INDENT("<h2>"+i->name.html+"</h2>") << endl;
					if (_makeHtml) htmlout << i->text.html << endl;
					if (_makeTex) texout << "\\subsection*{" << i->name.tex << "}\\label{" << fixTexAnchor(_docName.plain) << " " <<
						fixTexAnchor(i->name.plain) << "}" << endl;
					if (_makeTex) texout << i->text.tex << endl << endl;
					++_indentLevel;
					if (i->constants.size() > 0)
						texout << "\\begin{center}\n"
							"\\rule{0.75\\linewidth}{0.1mm}\n"
							"\\end{center}" << endl << endl;
					outputElements(htmlout, texout, i->constants, lexer, parser);
					--_indentLevel;
				}
			}
		}
		break;
	case GLOBALS:
	case INDEX:
	case IOFILE:
	case EXAMPLE:
	case OTHER:
		// nothing. They just have overview
		break;
	default:
		// Warning already given
		break;
	}
	output(htmlout, texout, _docBottom);
}
