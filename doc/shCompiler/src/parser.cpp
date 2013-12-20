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

#include <sstream>
#include <queue>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <parser.h>

using namespace std;

//#define SH_DEBUG
//#define SH_DETAILED_DEBUG

void shParser::p_findFollowDependency(string nonterminal, int s)
{
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "Finding follow dependency of " << nonterminal << " with length " << s << endl;
#endif
	set<pair<string, int> > x;
	if (s == 0)
	{
		p_followGraphAdjList[pair<string, int>(nonterminal, 0)] = x;			// An empty set
		return;
	}
	// If computed before, return
	if (p_followGraphAdjList.find(pair<string, int>(nonterminal, s)) != p_followGraphAdjList.end())
		return;
	p_followGraphAdjList[pair<string, int>(nonterminal, s)] = x;
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "\twhich was not computed before!" << endl;
#endif
	set<p_parentAndAfterData> possibility = p_parentAndAfter[nonterminal];
	for (set<p_parentAndAfterData>::const_iterator iter = possibility.begin(), end = possibility.end(); iter != end; ++iter)
	{
		for (int k = 0; k < s; ++k)
		{
			set<list<string> > temp1 = p_findLLKFirst(iter->after, k, true);	// In case of LRK, since all the firsts are already computed and memoized, then this function will return the result without
												// doing any computation
			if (temp1.size() != 0)
				p_followGraphAdjList[pair<string, int>(nonterminal, s)].insert(pair<string, int>(iter->parent, s-k));
		}
	}
}

pair<string ,int> shParser::p_findParent(pair<string, int> node)
{
	pair<string, int> orig = node;
	while (p_followParent.find(node) != p_followParent.end())
		node = p_followParent[node];
	// Past compression
	while (p_followParent.find(orig) != p_followParent.end())
	{
		pair<string, int> temp = orig;
		orig = p_followParent[orig];
		p_followParent[temp] = node;
	}
	return node;
}

pair<bool, pair<string, int> > shParser::p_removeCycles(pair<string, int> node)
{
	node = p_findParent(node);
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "Finding cycle for (" << node.first << ", " << node.second << ")" << endl;
#endif
	if (p_followDFSVisited.find(node) != p_followDFSVisited.end() && p_followDFSVisited[node] == true)
		return pair<bool, pair<string, int> >(false, pair<string, int>("", 0));
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "\twhich was not computed before!" << endl;
#endif
	p_followDFSVisited[node] = true;
	set<pair<string, int> > toBeRemoved;
	for (set<pair<string, int> >::const_iterator adj = p_followGraphAdjList[node].begin();
			adj != p_followGraphAdjList[node].end(); ++adj)
	{
		if (node == p_findParent(*adj))
		{
			toBeRemoved.insert(*adj);
			continue;
		}
		if (p_followDFSVisited.find(p_findParent(*adj)) != p_followDFSVisited.end() && p_followDFSVisited[p_findParent(*adj)] == true)
		{
			for (set<pair<string, int> >::const_iterator iter = toBeRemoved.begin(), end = toBeRemoved.end(); iter != end; ++iter)
				p_followGraphAdjList[node].erase(*iter);
			p_followDFSVisited[node] = false;
#ifdef SH_DEBUG
			cout << "node (" << node.first << ") should be equal to parent(node) (" << p_findParent(node).first << ")" << endl;
			cout << "node: " << node.first << " adj: " << adj->first << endl;
#endif
			p_followGraphAdjList[node].erase(*adj);					// remove cycle edge
			p_followGraphAdjList[*adj].erase(node);					// remove cycle edge
			pair<string, int> parent, child;
			// In order to have parent have minimum code:
			if (p_nonterminalCodes[node.first] < p_nonterminalCodes[adj->first])
			{
				parent = node;
				child = *adj;
			}
			else
			{
				parent = p_findParent(*adj);
				child = node;
			}
			for (set<pair<string, int> >::const_iterator iter = p_followGraphAdjList[p_findParent(child)].begin();
					iter != p_followGraphAdjList[p_findParent(child)].end(); ++iter)
				if (parent != p_findParent(*iter))			// If not adding itself as an adjacent!
					p_followGraphAdjList[parent].insert(p_findParent(*iter));
			p_followGraphAdjList[p_findParent(child)].clear();
			p_followParent[p_findParent(child)] = parent;
			p_followFoundLoop = true;
#ifdef SH_DEBUG
			cout << "returning: " << parent.first << " p: " << p_findParent(parent).first << endl;
#endif
			return pair<bool, pair<string, int> >(true, parent);
		}
		pair<string, int> edgeToBeRemoved = p_findParent(*adj);
		pair<bool, pair<string, int> > res = p_removeCycles(p_findParent(*adj));
#ifdef SH_DEBUG
		cout << "After remove cycle node: " << node.first << " pnode: " << p_findParent(node).first << endl;
		cout << "After remove cycle res: " << res.second.first << " pres: " << p_findParent(res.second).first << endl;
#endif
		if (res.first && res.second != node)
		{
			for (set<pair<string, int> >::const_iterator iter = toBeRemoved.begin(), end = toBeRemoved.end(); iter != end; ++iter)
				p_followGraphAdjList[node].erase(*iter);
			p_followGraphAdjList[node].erase(edgeToBeRemoved);		// remove cycle edge
			pair<string, int> parent, child;
			// In order to have parent have minimum code:
			if (p_nonterminalCodes[node.first] < p_nonterminalCodes[res.second.first])
			{
				parent = p_findParent(node);
				child = res.second;
			}
			else
			{
				parent = p_findParent(res.second);
				child = node;
			}
			for (set<pair<string, int> >::const_iterator iter = p_followGraphAdjList[child].begin();
					iter != p_followGraphAdjList[child].end(); ++iter)
				if (parent != *iter)		// If not adding itself as an adjacent!
					p_followGraphAdjList[p_findParent(parent)].insert(*iter);
			p_followGraphAdjList[child].clear();
			p_followParent[child] = p_findParent(parent);
			p_followDFSVisited[node] = false;
			return res;
		}
	}
	p_followDFSVisited[p_findParent(node)] = false;
	for (set<pair<string, int> >::const_iterator iter = toBeRemoved.begin(), end = toBeRemoved.end(); iter != end; ++iter)
		p_followGraphAdjList[p_findParent(node)].erase(*iter);
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "Returning, finding no loops!" << endl;
#endif
	return pair<bool, pair<string, int> >(false, pair<string, int>("", 0));
}

void shParser::p_updateParentAndAfter()
{
	for (int k = 0; k <= p_k_in_grammar; ++k)
		for (vector<string>::const_iterator nont = p_nonterminals.begin(), end = p_nonterminals.end(); nont != end; ++nont)
		{
			pair<string, int> node(*nont, k);
			set<p_parentAndAfterData> forParentAndAfter2;
			for (set<p_parentAndAfterData>::const_iterator iter = p_parentAndAfter[*nont].begin(), end2 = p_parentAndAfter[*nont].end();
					iter != end2; ++iter)
				forParentAndAfter2.insert(p_parentAndAfterData(p_findParent(pair<string, int>(iter->parent, k)).first, iter->after));
			p_parentAndAfter2[node] = forParentAndAfter2;
		}
	for (int k = 0; k <= p_k_in_grammar; ++k)
		for (vector<string>::const_iterator nont = p_nonterminals.begin(), end = p_nonterminals.end(); nont != end; ++nont)
		{
			pair<string, int> node(*nont, k);
			node = p_findParent(node);
			for (set<p_parentAndAfterData>::const_iterator iter = p_parentAndAfter[*nont].begin(), end2 = p_parentAndAfter[*nont].end();
					iter != end2; ++iter)
				p_parentAndAfter2[node].insert(*iter);					// Accumulate p_parentAndAfterData in parent
		}
}

// This function, recurses on the LL(k) grammar and finds out different follows of a symbol with length s
// It uses a map to memoize the recursion, it also uses p_findLLKFirst function
// The grammar must be ensured not to be left recursive first
// Follow dependencies must be found before and its graph must be DAGgified first if not, it will get stuck in a loop
set<list<string> > shParser::p_findFollow(string nonterminal, int s)
{
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "Finding follow of " << nonterminal << " with length " << s << endl;
#endif
	set<list<string> > answer;
	list<string> x;
	set<list<string> > temp;
	temp.insert(x);
	if (s == 0)
		return temp;					// return an empty list
	// If computed before, return result directly
	if (p_follow[pair<string, int>(p_findParent(pair<string, int>(nonterminal, s)))].size() != 0)
		return p_follow[pair<string, int>(p_findParent(pair<string, int>(nonterminal, s)))];
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "\twhich was not computed before!" << endl;
#endif
	set<p_parentAndAfterData> possibility = p_parentAndAfter2[p_findParent(pair<string, int>(nonterminal, s))];
	for (set<p_parentAndAfterData>::const_iterator iter = possibility.begin(), end = possibility.end(); iter != end; ++iter)
	{
		for (int k = 0; k < s; ++k)
		{
			// To prevent self loop!
			if (k == 0 && p_findParent(pair<string, int>(nonterminal, s)) == p_findParent(pair<string, int>(iter->parent, s)))
				continue;
			set<list<string> > temp1 = p_findLLKFirst(iter->after, k, true);
			if (temp1.size() == 0)
				continue;
			pair<string, int> parentOfLeftOver = p_findParent(pair<string, int>(iter->parent, s-k));
			set<list<string> > temp2 = p_findFollow(parentOfLeftOver.first, parentOfLeftOver.second);
			// If follow of start was computed, then (s-k) times $ (or "" here) should be added to follows
			if (p_nonterminalCodes[iter->parent] == 0)
			{
				list<string> endOfFile;
				for (int i = 0; i < s-k; ++i)
					endOfFile.push_back("");
				temp2.insert(endOfFile);
			}
			for (set<list<string> >::const_iterator l1 = temp1.begin(), end2 = temp1.end(); l1 != end2; ++l1)
				for (set<list<string> >::const_iterator l2 = temp2.begin(), end3 = temp2.end(); l2 != end3; ++l2)
				{
					list<string> res = *l1;
					for (list<string>::const_iterator s2 = l2->begin(), end4 = l2->end(); s2 != end4; ++s2)
						res.push_back(*s2);
					answer.insert(res);
				}
		}
		temp = p_findLLKFirst(iter->after, s, false);
		for (set<list<string> >::const_iterator l = temp.begin(), end2 = temp.end(); l != end2; ++l)
			answer.insert(*l);
	}
	// If follow of start was computed, then s times $ (or "" here) should be added to follows
	if (p_nonterminalCodes[p_findParent(pair<string, int>(nonterminal, s)).first] == 0)
	{
		list<string> endOfFile;
		for (int i = 0; i < s; ++i)
			endOfFile.push_back("");
		answer.insert(endOfFile);
	}
	p_follow[p_findParent(pair<string, int>(nonterminal, s))] = answer;
	return answer;
}

void shParser::shPReset()
{
	while (!p_LLK_parseStack.empty())
		p_LLK_parseStack.pop();
	while (!p_LRK_parseStack.empty())
		p_LRK_parseStack.pop();
	p_errorMessage = "";
}

void shParser::shPAddActionRoutine(const string &actionRoutineName, shPActionRoutineFunction func)
{
	if (actionRoutineName[0] != '@')
		p_actionRoutine['@'+actionRoutineName] = func;
	else
		p_actionRoutine[actionRoutineName] = func;
}

void shParser::shPAmbiguityResolution(int r)
{
	p_ambiguityResolution = r;
}

void shParser::p_cleanupIntermediateData()
{
	p_first.clear();
	p_parentAndAfter.clear();
	p_parentAndAfter2.clear();
	p_follow.clear();
	p_followGraphAdjList.clear();
	p_followParent.clear();
	p_firstComplete.clear();
	p_firstDFSVisited.clear();
	p_followDFSVisited.clear();
}

string shParser::shPError()
{
	return p_errorMessage;
}

void shParser::shPAppendError(const string &error)
{
	p_errorMessage += error;
}

void shParser::shPAppendError(const char *error)
{
	if (error)
		p_errorMessage += error;
}

void shParser::shPAppendError(const unsigned char *error)
{
	if (error)
		p_errorMessage += (const char *)error;
}

static string _toC(const string &str)
{
	string res = "\"";
	for (unsigned int i = 0, size = str.size(); i < size; ++i)
		if (str[i] == '\t')		res += "\\t";
		else if (str[i] == '\n')	res += "\\n";
		else if (str[i] == '\r')	res += "\\r";
		else if (str[i] == '\\')	res += "\\\\";
		else if (str[i] == '"')		res += "\\\"";
		else				res += str[i];
	res += "\"";
	return res;
}

static string _indent(int k)
{
	string indent = "\t\t\t";
	for (int i = 0; i < k; ++i)
		indent += '\t';
	return indent;
}

int shParser::shPGeneratePLErrorHandler(const char *plehFile)
{
	if (plehFile == '\0')
		return SH_P_FILE_NOT_FOUND;
	if (!p_LLK_tableBuilt && !p_LRK_diagramBuilt)
		return SH_P_RULES_NOT_LOADED;
	ofstream hdr((string(plehFile)+".h").c_str());
	ofstream src((string(plehFile)+".cpp").c_str());
	if (!hdr.is_open() || !src.is_open())
		return SH_P_FILE_NOT_FOUND;
	string hdrGaurd;
	for (const char *p = plehFile; *p; ++p)
		if (!(*p >= 'a' && *p <= 'z') && !(*p >= 'A' && *p <= 'Z') && !(*p >= '0' && *p <= '9'))
			hdrGaurd += '_';
		else if (*p >= 'a' && *p <= 'z')
			hdrGaurd += *p-'a'+'A';
		else
			hdrGaurd += *p;
	hdrGaurd += "_H_BY_SH_COMPILER";
	hdr << "#ifndef " << hdrGaurd << "\n"
		"#define " << hdrGaurd << "\n"
		"\n"
		"#include <stack>\n"
		"#include <string>\n"
		"#include <string.h>\n"
		"#include <shcompiler.h>\n"
		"\n"
		"using namespace std;\n"
		"\n";
	src << "#include \"" << plehFile << ".h\"\n"
		"\n"
		"#define MAX_MESSAGE_SIZE 1000\n"
		"\n";
	if (p_LLK_tableBuilt)
	{
		hdr << "bool llkpleh(stack<shParser::shLLKStackElement> &s,    // Parse stack\n"
			"             shLexer &l,                               // Lexer\n"
			"             string &em,                               // Error message\n"
			"             string &cur,                              // The token to be matched (could be terminal or non-terminal)\n"
			"             shKLimitedList<string> &la,               // Lookahead (containing type of tokens)\n"
			"             shKLimitedList<string> &lat);             // Lookahead (containing tokens themselves)\n";
		src << "bool llkpleh(stack<shParser::shLLKStackElement> &s,    // Parse stack\n"
			"             shLexer &l,                               // Lexer\n"
			"             string &em,                               // Error message\n"
			"             string &cur,                              // The token to be matched (could be terminal or non-terminal)\n"
			"             shKLimitedList<string> &la,               // Lookahead (containing type of tokens)\n"
			"             shKLimitedList<string> &lat)              // Lookahead (containing tokens themselves)\n"
			"{\n"
			"\t// Note: this function needs to notify the semantics analyzer that there is an error.\n"
			"\t// Therefore, before returning from the function, replace STOP_EVALUATION with your own method.\n"
			"\tchar temp[MAX_MESSAGE_SIZE+1];\n"
			"\tbool is_warning = false;\t\t\t// if only a warning is generated, set this to true\n"
			"\tbool recovered = true;\n";
		for (set<string>::const_iterator i = p_tokenTypes.begin(), end = p_tokenTypes.end(); i != end; ++i)
			src << (i == p_tokenTypes.begin()?"\t":"\telse ") << "if (cur == " << _toC(*i) << ")\n"
				"\t{\n"
				"\t\tl.shLInsertFakeToken(cur);\n"
				"\t\tsnprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: missing %s, inserted \\\"%s\\\"\\n\",\n"
				"\t\t\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), l.shLCurrentToken().c_str());\n"
				"\t\tem += temp;\n"
				"\t}\n";
		src << "\telse\n"
			"\t{\n";
		for (int i = 0; i < p_k_in_grammar; ++i)
		{
			src << "\t\tlist<string>::const_iterator token" << i << " = ";
			if (i == 0)
				src << "la.k_elements().begin();\n";
			else
				src << "token" << i-1 << ";\n"
					"\t\t++token" << i << ";\n";
		}
		src << "\t\tstring la_str;\n"
			"\t\tfor (list<string>::const_iterator i = la.k_elements().begin(), end = la.k_elements().end(); i != end; ++i)\n"
			"\t\t\tla_str += (i == la.k_elements().begin()?\"\":\" \")+((*i == \"\")?string(\"$\"):*i);\n";

		vector<string> lookAheadTypes;
		for (set<string>::const_iterator i = p_tokenTypes.begin(), end = p_tokenTypes.end(); i != end; ++i)
			lookAheadTypes.push_back(*i);
		lookAheadTypes.push_back(string(""));
		bool thereWasNone = true;
		for (unsigned i = 0, size = p_nonterminals.size(); i < size; ++i)
		{
			p_LLK_table_lookup possibleLookUp;
			possibleLookUp.nonterminal = p_nonterminals[i];
			for (int j = 0; j < p_k_in_grammar; ++j)
				possibleLookUp.lookAhead.push_back(lookAheadTypes[0]);
			vector<unsigned int> lookAheadIndices(p_k_in_grammar, 0);
			bool first = true;
			int lastOpen = -1;
			while (*possibleLookUp.lookAhead.begin() != "")
			{
				if (p_LLK_table.find(possibleLookUp) == p_LLK_table.end())
				{
#define OPEN_IFS \
	do {\
		first = false;\
		thereWasNone = false;\
		src << (i == 0?"\t\t":"\t\telse ") << "if (cur == " << _toC(possibleLookUp.nonterminal) << ")\n"\
			"\t\t{\n";\
		int k = 0;\
		for (list<string>::const_iterator j = possibleLookUp.lookAhead.begin(), end = possibleLookUp.lookAhead.end(); j != end; ++j, ++k)\
			src << _indent(k) << "if (*token" << k << " == " << _toC(*j) << ")\n" <<\
				_indent(k) << "{\n";\
	} while (0)
#define CLOSE_IFS \
	do {\
		list<string>::const_iterator j = possibleLookUp.lookAhead.begin(), end = possibleLookUp.lookAhead.end();\
		for (int k = p_k_in_grammar-1; k > lastOpen+1; --k)\
			src << _indent(k) << "}\n" <<\
				_indent(k) << "else\n" <<\
				_indent(k) << "{\n" <<\
				_indent(k) << "\tsnprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: unhandled error, expected to match %s, but found %s\\n\",\n" <<\
				_indent(k) << "\t\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());\n" <<\
				_indent(k) << "\tem += temp;\n" <<\
				_indent(k) << "}\n";\
		src << _indent(lastOpen+1) << "}\n";\
		int k = 0;\
		for (; k <= lastOpen; ++j, ++k);\
		for (; j != end; ++j, ++k)\
		{\
			src << _indent(k);\
			if (k == lastOpen+1)\
				src << "else ";\
			src << "if (*token" << k << " == " << _toC(*j) << ")\n" <<\
				_indent(k) << "{\n";\
		}\
	} while (0)
					if (first)
						OPEN_IFS;
					else
						CLOSE_IFS;
					lastOpen = p_k_in_grammar-1;
					src << _indent(p_k_in_grammar) << "snprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: expected to match %s, but found %s\\n\",\n" <<
						_indent(p_k_in_grammar) << "\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());\n" <<
						_indent(p_k_in_grammar) << "em += temp;\n" <<
						_indent(p_k_in_grammar) << "// TODO: Your choice of actions are:\n" <<
						_indent(p_k_in_grammar) << "// 1. stop parsing\n" <<
						_indent(p_k_in_grammar) << "      em += \"Could not recover from last error!\\n\";    // Note: The default is to fail, so if you handle\n" <<
						_indent(p_k_in_grammar) << "      recovered = false;                               // the error properly, remove these two lines\n" <<
						_indent(p_k_in_grammar) << "// or handle the case by a combination of some of these actions:\n" <<
						_indent(p_k_in_grammar) << "// 2. skip the nonterminal (simply keep recovered true), and possibly fake some input\n" <<
						_indent(p_k_in_grammar) << "//    l.shLInsertFakeToken(\"some_type\");               // add a \"value\" parameter if you want to generate the token yourself\n" <<
						_indent(p_k_in_grammar) << "// 3. change the lookahead tokens (for example 1 token). Useful for example to rename an identifier which was given a reserved name\n" <<
						_indent(p_k_in_grammar) << "//    l.shLInsertFakeToken(\"some_type\");\n" <<
						_indent(p_k_in_grammar) << "//    *la.k_elements().begin() = l.shLCurrentTokenType();\n" <<
						_indent(p_k_in_grammar) << "//    *lat.k_elements().begin() = l.shLCurrentToken();\n" <<
						_indent(p_k_in_grammar) << "// 4. skip some of the tokens (for example 3 tokens) and retry nonterminal\n" <<
						_indent(p_k_in_grammar) << "//    s.push(cur);\n" <<
						_indent(p_k_in_grammar) << "//    s.push(*token2);\n" <<
						_indent(p_k_in_grammar) << "//    s.push(*token1);\n" <<
						_indent(p_k_in_grammar) << "//    s.push(*token0);\n" <<
						_indent(p_k_in_grammar) << "// 5. expand nonterminal and remove a few of its first elements (possibly fake input, see second choice). Rules associated with this nonterminal are:\n";
					set<int> rules = p_rulesByNonterminalCode[p_nonterminalCodes[p_nonterminals[i]]];
					for (set<int>::const_iterator r = rules.begin(), end = rules.end(); r != end; ++r)
					{
						shRule rule = p_ruleByCodeWithActionRoutine[*r];
						src << _indent(p_k_in_grammar) << "//  " << rule.head << " ->";
						for (list<string>::const_iterator b = rule.body.begin(), end2 = rule.body.end(); b != end2; ++b)
							src << " " << *b;
						src << "\n";
						for (list<string>::const_reverse_iterator b = rule.body.rbegin(), end2 = rule.body.rend(); b != end2; ++b)
							src << _indent(p_k_in_grammar) << "//    s.push(\"" << *b << "\");\n";
					}
				}
				int k = p_k_in_grammar-1;
				list<string>::const_reverse_iterator end = possibleLookUp.lookAhead.rend();
				for (list<string>::reverse_iterator j = possibleLookUp.lookAhead.rbegin(); j != end; ++j, --k)
				{
					++lookAheadIndices[k];
					if (lookAheadIndices[k] < lookAheadTypes.size()-1)
					{
						*j = lookAheadTypes[lookAheadIndices[k]];
						list<string>::const_iterator end2 = possibleLookUp.lookAhead.end();
						if (lastOpen > k-1)
							lastOpen = k-1;
						++k;
						for (list<string>::iterator jj = j.base(); jj != end2; ++jj, ++k)
						{
							*jj = lookAheadTypes[0];
							lookAheadIndices[k] = 0;
						}
						break;
					}
					else if (lookAheadIndices[k] == lookAheadTypes.size()-1)
					{
						*j = lookAheadTypes[lookAheadTypes.size()-1];		// which is ""
						list<string>::const_iterator end2 = possibleLookUp.lookAhead.end();
						if (lastOpen > k-1)
							lastOpen = k-1;
						++k;
						for (list<string>::iterator jj = j.base(); jj != end2; ++jj, ++k)
						{
							*jj = lookAheadTypes[lookAheadTypes.size()-1];	// after $, all the rest can only be $
							lookAheadIndices[k] = lookAheadTypes.size()-1;
						}
						break;
					}
					// else, overflowed. Go to the next digit and increment it
				}
			}
			if (p_LLK_table.find(possibleLookUp) == p_LLK_table.end())		// now possibleLookUp contains $, so this is the case of unexpected end of file!
			{
				if (first)
					OPEN_IFS;
				else
					CLOSE_IFS;
				lastOpen = p_k_in_grammar-1;
				src << _indent(p_k_in_grammar) << "snprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: expected to match %s, but encountered end of file\\n\",\n" <<
					_indent(p_k_in_grammar) << "\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str());\n" <<
					_indent(p_k_in_grammar) << "em += temp;\n" <<
					_indent(p_k_in_grammar) << "// TODO: Your choice of actions are:\n" <<
					_indent(p_k_in_grammar) << "// 1. stop parsing\n" <<
					_indent(p_k_in_grammar) << "      em += \"Could not recover from last error!\\n\";    // Note: The default is to fail, so if you handle\n" <<
					_indent(p_k_in_grammar) << "      recovered = false;                               // the error properly, remove these two lines\n" <<
					_indent(p_k_in_grammar) << "// or handle the case by a combination of some of these actions:\n" <<
					_indent(p_k_in_grammar) << "// 2. skip the nonterminal (simply keep recovered true), and possibly fake some input\n" <<
					_indent(p_k_in_grammar) << "//    l.shLInsertFakeToken(\"some_type\");               // add a \"value\" parameter if you want to generate the token yourself\n";
			}
			if (!first)		// if first, means there was no gaps at all!
			{
				for (int k = p_k_in_grammar-1; k >= 0; --k)
					src << _indent(k) << "}\n" <<
						_indent(k) << "else\n" <<
						_indent(k) << "{\n" <<
						_indent(k) << "\tsnprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: unhandled error, expected to match %s, but found %s\\n\",\n" <<
						_indent(k) << "\t\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());\n" <<
						_indent(k) << "\tem += temp;\n" <<
						_indent(k) << "}\n";
				src << "\t\t}\n";
			}
		}
		if (thereWasNone)
			src << "\t\tsnprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: unhandled error, expected to match %s, but found %s\\n\",\n"
				"\t\t\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());\n"
				"\t\tem += temp;\n";
		else
			src << "\t\telse\n"
				"\t\t{\n"
				"\t\t\tsnprintf(temp, MAX_MESSAGE_SIZE, \"%s:%d:%d: error: unhandled error, expected to match %s, but found %s\\n\",\n"
				"\t\t\t\t\tl.shLFileName().c_str(), l.shLLine(), l.shLColumn(), cur.c_str(), la_str.c_str());\n"
				"\t\t\tem += temp;\n"
				"\t\t}\n";
		src << "\t}\n";
		src << "\tif (!is_warning)\n"
			"\t\tSTOP_EVALUATION;\n"
			"\treturn recovered;\n"
			"}\n";
	}
	if (p_LRK_diagramBuilt)
	{
		// TODO
	}
	hdr << endl << "#endif" << endl;
	return SH_P_SUCCESS;
}
