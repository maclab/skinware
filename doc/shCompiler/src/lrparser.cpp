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
#include <parser.h>
#include "grammar.h"
#include "nonstd.h"

using namespace std;

//#define SH_DEBUG
//#define SH_DETAILED_DEBUG

static void _noFunctionForActionRoutine(shLexer &, shParser &){}
static void _noFunctionForLRKErrorProductionRule(stack<shParser::shLRKStackElement> &, shLexer &, string &, shParser::shRule &){}
static bool _noFunctionForLRKPhraselevelErrorHandler(stack<shParser::shLRKStackElement> &,
		shLexer &, string &, shKLimitedList<string> &, shKLimitedList<string> &){return false;}

#define PRINT_RULE(r) \
	do { \
		if ((r) >= 0)\
		{\
			char p_temp[100];\
			sprintf(p_temp, "*** Rule %d is: ", (r));\
			p_errorMessage += p_temp;\
			p_errorMessage += p_ruleByCode[(r)].head;\
			p_errorMessage += " ->";\
			for (list<string>::const_iterator i = p_ruleByCode[(r)].body.begin(), end = p_ruleByCode[(r)].body.end(); i != end; ++i)\
				p_errorMessage += " "+*i;\
			p_errorMessage += " ;\n";\
		}\
	} while (0)

int shParser::shPReadLRKGrammar(const shLexer &lexer, const char *grammarFile, int k, bool newGrammar)
{
	if (!lexer.p_initialized)
	{
		p_errorMessage = "Lexer not initialized\n";
		return SH_P_LEXER_NOT_INITIALIZED;
	}
	for (list<shDFA>::const_iterator i = lexer.p_typeDFAs.begin(), end = lexer.p_typeDFAs.end(); i != end; ++i)
		p_tokenTypes.insert(i->name);
	for (set<string>::const_iterator i = lexer.p_keywords.begin(), end = lexer.p_keywords.end(); i != end; ++i)
		p_tokenTypes.insert(*i);
	p_k_in_grammar = k;
	p_ruleCount = 0;
	p_nonterminalCount = 0;
	p_LRK_EPRHandleFunction.clear();
	p_LRK_pleh = _noFunctionForLRKPhraselevelErrorHandler;
	p_ruleByCode.clear();
	p_rulesByNonterminalCode.clear();
	p_ruleActionRoutine.clear();
	p_nonterminalCodes.clear();
	p_nonterminals.clear();
	shLexer l;
	shParser p;
	l.shLInitToGrammarFile();
	p.shPInitToGrammarFile();
	if (l.shLOpenFile(FIX_PATH(grammarFile).c_str()) == SH_L_FILE_NOT_FOUND)
	{
		char temp[1001];
		snprintf(temp, 1000, "Could not open grammar file: %s\n", FIX_PATH(grammarFile).c_str());
		p_errorMessage = temp;
		return SH_P_FILE_NOT_FOUND;
	}
	shcompiler_GF_init();
	p.shPParseLLK(l);			// if parse error, the phrase level error handler would set the error so no need to check the output of parser
	cout << p.shPError();
	if (shcompiler_GF_error() != SH_P_SUCCESS)
		return shcompiler_GF_error();
	list<shRule> rules = shcompiler_GF_getRules();
	shcompiler_GF_cleanup();
	list<string> extraNonTerminals;		// when an action routine is met that is not at the end of rule, its added here so later a rule would be generated for it
	for (list<shRule>::const_iterator i = rules.begin(), end = rules.end(); i != end; ++i)
	{
		shRule rule;
		if (p_tokenTypes.find(i->head) != p_tokenTypes.end())		// TODO: when writing PLEH, give the token types to shcompiler_GF_init so that PLEH would give this error (with line number etc)
										// no need for PRINT_RULE then
		{
			char temp[1000];
			sprintf(temp, "A terminal cannot be replaced! error in rule number: %d\n", p_ruleCount);
			p_errorMessage = temp;
			PRINT_RULE(p_ruleCount-1);
			return SH_P_GRAMMAR_ERROR;
		}
		if (p_nonterminalCodes.find(i->head) == p_nonterminalCodes.end())
		{
			p_nonterminals.push_back(i->head);
			p_nonterminalCodes[i->head] = p_nonterminalCount++;
		}
		rule.head = i->head;
		for (list<string>::const_iterator j = i->body.begin(), end2 = i->body.end(); j != end2; ++j)
			if ((*j)[0] == '@')		// An action routine which must be on the far right of the rule.  If not, a nonterminal will be created that expands to it
			{
				list<string>::const_iterator next = j;
				++next;
				if (next != end2)
				{
					extraNonTerminals.push_back(*j);
					rule.body.push_back("("+*j+")");			// replace it in the rule by a nonterminal
				}
				else
				{
					p_ruleActionRoutine[p_ruleCount] = *j;
					p_actionRoutine[*j] = _noFunctionForActionRoutine;
				}
			}
			else if ((*j)[0] == '!')
				p_ruleEPRHandler[*j].insert(p_ruleCount);
			else	// A nonterminal or a terminal
			{
				if (p_nonterminalCodes.find(*j) == p_nonterminalCodes.end() && p_tokenTypes.find(*j) == p_tokenTypes.end())
				{
					p_nonterminals.push_back(*j);
					p_nonterminalCodes[*j] = p_nonterminalCount++;
				}
				rule.body.push_back(*j);
			}
		p_ruleByCode[p_ruleCount] = rule;
		p_rulesByNonterminalCode[p_nonterminalCodes[rule.head]].insert(p_ruleCount);
		p_LRK_EPRHandleFunction[p_ruleCount] = _noFunctionForLRKErrorProductionRule;
		++p_ruleCount;
	}
	for (list<string>::const_iterator i = extraNonTerminals.begin(), end = extraNonTerminals.end(); i != end; ++i)
	{
		string helperNonTerminal = "("+*i+")";
		if (p_nonterminalCodes.find(helperNonTerminal) == p_nonterminalCodes.end())
		{
			p_nonterminals.push_back(helperNonTerminal);					// add the new nonterminal (if not previously defined)
			p_nonterminalCodes[helperNonTerminal] = p_nonterminalCount++;
			shRule r;									// create a new rule for it in the form (@ar) -> ; with @ar as its assigned action routine
			r.head = helperNonTerminal;
			p_ruleByCode[p_ruleCount] = r;							// add it to rules
			p_rulesByNonterminalCode[p_nonterminalCodes[r.head]].insert(p_ruleCount);
			p_LRK_EPRHandleFunction[p_ruleCount] = _noFunctionForLRKErrorProductionRule;
			p_ruleActionRoutine[p_ruleCount] = *i;						// set its action routine
			p_actionRoutine[*i] = _noFunctionForActionRoutine;
			++p_ruleCount;
		}
	}
	if (p_ruleCount == 0)					// There were no rules in the file
	{
		p_errorMessage = "There are no rules in the given file\n";
		return SH_P_GRAMMAR_ERROR;
	}
	shRule initialRule;
	initialRule.head = "";
	initialRule.body.push_back(p_nonterminals[0]);
	p_ruleByCode[p_ruleCount] = initialRule;
#ifdef SH_DEBUG
	cout << "Rules are:" << endl;
	for (int i = 0; i < p_ruleCount; ++i)
	{
		cout << "\t" << i <<": ";
		cout << p_ruleByCode[i].head << " ->";
		for (list<string>::const_iterator iter = p_ruleByCode[i].body.begin(), end = p_ruleByCode[i].body.end(); iter != end; ++iter)
			cout << " " << *iter;
		cout << endl;
	}
	cout << "Rules per nonterminal:" << endl;
	for (int i = 0; i < p_nonterminalCount; ++i)
	{
		cout << "Nonterminal #" << i << " (" << p_nonterminals[i] << "):";
		for (set<int>::const_iterator iter = p_rulesByNonterminalCode[i].begin(), end = p_rulesByNonterminalCode[i].end(); iter != end; ++iter)
			cout << " " << *iter;
		cout << endl;
	}
#endif
	bool valid = true;
	p_errorMessage = "";
	for (int i = 0; i < p_nonterminalCount; ++i)
		if (p_rulesByNonterminalCode[i].size() == 0)
		{
			p_errorMessage += "No rule found for nonterminal: "+p_nonterminals[i]+"\n";
			valid = false;
		}
	if (!valid)
		return SH_P_GRAMMAR_ERROR;
	string cachePath = "";
	string grammarName = "";
	const char *lastSlash;
	for (lastSlash = grammarFile+strlen(grammarFile); lastSlash != grammarFile; --lastSlash)
		if (*lastSlash == '/')
			break;
	for (const char *p = grammarFile; p != lastSlash; ++p)
		cachePath += *p;
	if (lastSlash == grammarFile)		// there was no / at all
		cachePath += '.';
	cachePath += "/cache";
	for (const char *p = lastSlash; *p; ++p)
		grammarName += *p;
	(void)makeDir(cachePath);		// if could not create path (return value not zero), silently continue
	return p_shPPrepareLRKParse(FIX_PATH(cachePath+grammarName+".LRKtable").c_str(), !newGrammar);
}

// This function, recurses on the LR(k) grammar and finds out
// different combinations reachable by the symbol or sentence with length k
// It uses a map to memoize the recursion
// Computes partial first for left-recursive grammars
set<list<string> > shParser::p_findLRKFirst(list<string> sen, int k, bool shallEnd)
{
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "checking for:";
	for (list<string>::const_iterator iter = sen.begin(), end = sen.end(); iter != end; ++iter)
		cout << " " << *iter;
	cout << " with length " << k << " which must " << (shallEnd?"end":"not necessarily end") << endl;
#endif
	p_findFirstData data(sen, k, shallEnd);
	set<list<string> > answer;
	list<string> x;
	set<list<string> > only_x;
	only_x.insert(x);
	if (sen.size() == 0)
		if (k == 0)					// Then found all and should return
			return only_x;				// an empty list. This means that the sequences that lead to this sentence will be accepted
		else						// Then no sentence could be built with length at least k
			return answer;				// an empty answer. This means that the sequences that lead to this sentence will be discarded
	else
		if (k == 0)
		{
			if (shallEnd)				// Then found all terminals but the sentence is not finished
			{
				for (list<string>::const_iterator iter = sen.begin(), end = sen.end(); iter != end; ++iter)
					if (p_tokenTypes.find(*iter) != p_tokenTypes.end() || *iter == "" || !p_nullable[p_nonterminalCodes[*iter]])
						return answer;	// an empty answer. This means that the sequences that lead to this sentence will be discarded
				return only_x;			// All followings are nullable, so return an empty list
			}
			else					// Then found all and we don't have to end it
				return only_x;			// an empty list. This means that the sequences that lead to this sentence will be accepted
		}
	if (p_firstDFSVisited.find(data) != p_firstDFSVisited.end() && p_firstDFSVisited[data] == true)
		return p_first[data];
	p_firstDFSVisited[data] = true;
	if (p_firstComplete.find(data) != p_firstComplete.end() && p_firstComplete[data] == true)
		return p_first[data];
	if (p_tokenTypes.find(sen.front()) != p_tokenTypes.end() || sen.front() == "")
	{
		string s = sen.front();
		sen.pop_front();
		set<list<string> > temp = p_findLRKFirst(sen, k-1, shallEnd);
		sen.push_back(s);
		for (set<list<string> >::const_iterator iter = temp.begin(), end = temp.end(); iter != end; ++iter)
		{
			x = *iter;
			x.push_front(s);
			if (p_first[data].find(x) == p_first[data].end())
			{
				p_first[data].insert(x);
				p_firstDFSVisited[data] = false;
				p_moreFirstFound = true;
			}
		}
	}
	else if (sen.size() == 1)
	{
		set<int> to = p_rulesByNonterminalCode[p_nonterminalCodes[sen.front()]];
		for (set<int>::const_iterator i = to.begin(), end = to.end(); i != end; ++i)
		{
			set<list<string> > temp = p_findLRKFirst(p_ruleByCode[*i].body, k, shallEnd);
			for (set<list<string> >::const_iterator iter = temp.begin(), end2 = temp.end(); iter != end2; ++iter)
				if (p_first[data].find(*iter) == p_first[data].end())
				{
					p_first[data].insert(*iter);
					p_firstDFSVisited[data] = false;
					p_moreFirstFound = true;
				}
		}
	}
	else
	{
		for (int i = 0; i <= k; ++i)
		{
			list<string> temp;
			temp.push_back(sen.front());
			set<list<string> > s1 = p_findLRKFirst(temp, i, (i!=k)?true:shallEnd);
			if (s1.size() != 0)
			{
				temp = sen;
				temp.pop_front();
				set<list<string> > s2 = p_findLRKFirst(temp, k-i, shallEnd);
				for (set<list<string> >::const_iterator iter1 = s1.begin(), end = s1.end(); iter1 != end; ++iter1)
					for (set<list<string> >::const_iterator iter2 = s2.begin(), end2 = s2.end(); iter2 != end2; ++iter2)
					{
						list<string> result = *iter1;
						for (list<string>::const_iterator iter = iter2->begin(), end3 = iter2->end(); iter != end3; ++iter)
							result.push_back(*iter);
						if (p_first[data].find(result) == p_first[data].end())
						{
							p_first[data].insert(result);
							p_firstDFSVisited[data] = false;
							p_moreFirstFound = true;
						}
					}
			}
		}
	}
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "returning answer..." << endl;
#endif
	if (p_firstCompleteFromNowOn)
		p_firstComplete[data] = true;
	return p_first[data];
}

void shParser::p_closure(p_LRK_state &state)
{
	queue<p_ruleWithDot> Q;
	for (p_LRK_state::const_iterator iter = state.begin(), end = state.end(); iter != end; ++iter)
		Q.push(iter->first);
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "Finding closure for state:" << endl;
#define p_PRINT_STATEp_
	for (p_LRK_state::const_iterator i = state.begin(), end = state.end(); i != end; ++i)
	{
		shRule rule = p_ruleByCode[i->first.rule];
		cout << rule.head << " ->";
		int x = 0;
		for (list<string>::const_iterator iter = rule.body.begin(), end2 = rule.body.end(); iter != end2; ++iter, ++x)
		{
			if (x == i->first.dotPosition)
				cout << " .";
			cout << " " << *iter;
		}
		if (i->first.dotPosition == (int)rule.body.size())
			cout << " .";
		cout << " ------ with follows:" << endl;
		for (set<list<string> >::const_iterator iter = i->second.begin(), end2 = i->second.end(); iter != end2; ++iter)
		{
			cout << "\t";
			for (list<string>::const_iterator it = iter->begin(), end3 = iter->end(); it != end3; ++it)
				if (*it == "")
					cout << " $";
				else
					cout << *it << " ";
			cout << endl;
		}
	}
#endif
	while (!Q.empty())
	{
		p_ruleWithDot ruleWithDot = Q.front();
		Q.pop();
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
		cout << "Rule: " << ruleWithDot.rule << " with dot at position: " << ruleWithDot.dotPosition << endl;
#endif
		shRule rule = p_ruleByCode[ruleWithDot.rule];
		if ((int)rule.body.size() == ruleWithDot.dotPosition)			// Nothing after the dot
			continue;
		for (int i = 0; i < ruleWithDot.dotPosition; ++i)
			rule.body.pop_front();
		if (p_tokenTypes.find(rule.body.front()) != p_tokenTypes.end())	// It was a terminal!
			continue;
		set<list<string> > follows = state[ruleWithDot];
		set<list<string> > newFollows;
		list<string> afterTheNonterminal = rule.body;
		afterTheNonterminal.pop_front();
		for (set<list<string> >::const_iterator iter = follows.begin(), end = follows.end(); iter != end; ++iter)
		{
			list<string> temp = afterTheNonterminal;
			for (list<string>::const_iterator i = iter->begin(), end2 = iter->end(); i != end2; ++i)
				temp.push_back(*i);
			set<list<string> > addingFollows = p_findLRKFirst(temp, p_k_in_grammar, false);
			for (set<list<string> >::const_iterator i = addingFollows.begin(), end2 = addingFollows.end(); i != end2; ++i)
				newFollows.insert(*i);
		}
		set<int> rulesForThisNonterminal = p_rulesByNonterminalCode[p_nonterminalCodes[rule.body.front()]];
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
		cout << "Rules for this nonterminal:";
		for (set<int>::const_iterator iter = rulesForThisNonterminal.begin(), end = rulesForThisNonterminal.end(); iter != end; ++iter)
			cout << " " << *iter;
		cout << endl;
#endif
		for (set<int>::const_iterator iter = rulesForThisNonterminal.begin(), end = rulesForThisNonterminal.end(); iter != end; ++iter)
		{
			bool newFollowFound = false;
			// insert dot in the beginning of RHS
			p_ruleWithDot newRuleWithDot(*iter, 0);
			for (set<list<string> >::const_iterator i = newFollows.begin(), end2 = newFollows.end(); i != end2; ++i)
				if (state[newRuleWithDot].find(*i) == state[newRuleWithDot].end())
				{
					state[newRuleWithDot].insert(*i);
					newFollowFound = true;
				}
			if (newFollowFound)
				Q.push(newRuleWithDot);
		}
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
		cout << "State is updated to:" << endl;
		for (p_LRK_state::const_iterator i = state.begin(), end = state.end(); i != end; ++i)
		{
			shRule rule = p_ruleByCode[i->first.rule];
			cout << rule.head << " ->";
			int x = 0;
			for (list<string>::const_iterator iter = rule.body.begin(), end2 = rule.body.end(); iter != end2; ++iter, ++x)
			{
				if (x == i->first.dotPosition)
					cout << " .";
				cout << " " << *iter;
			}
			if (i->first.dotPosition == (int)rule.body.size())
				cout << " .";
			cout << " ------ with follows:" << endl;
			for (set<list<string> >::const_iterator iter = i->second.begin(), end2 = i->second.end(); iter != end2; ++iter)
			{
				cout << "\t";
				for (list<string>::const_iterator it = iter->begin(), end3 = iter->end(); it != end3; ++it)
					if (*it == "")
						cout << " $";
					else
						cout << *it << " ";
				cout << endl;
			}
		}
#endif
	}
}

int shParser::p_shPPrepareLRKParse(const char *tableFile, bool load)
{
	p_LRK_diagramBuilt = false;
	ifstream tin(FIX_PATH(tableFile).c_str());
	if (load && tin.is_open())
	{
#ifdef SH_DEBUG
		cout << "Reading LRK table from file" << endl;
#endif
		string line;
		string nonterminal;
		int start, end;
		p_LRK_transition trns;
		int t;
		int c;
		while (tin >> start >> t >> end >> c)
		{
			trns = (p_LRK_transition)t;
			getline(tin, line);
			istringstream sin(line);
			list<string> tokens;
			for (int i = 0; i < c; ++i)
			{
				string tmp;
				sin >> tmp;					// If it was $, then "" would be read ;)
				tokens.push_back(tmp);
			}
			p_LRK_diagram[p_LRK_diagram_lookup(start, tokens)] = p_LRK_diagram_data(trns, end);
		}
	}
	else
	{
#ifdef SH_DEBUG
		cout << "Rebuilding LRK table from grammar" << endl;
#endif
		if (tin.is_open())
			tin.close();
		p_first.clear();
		p_follow.clear();
		p_firstComplete.clear();
		p_nullable = new bool[p_nonterminalCount];
		if (!p_nullable)
		{
			p_errorMessage = "Insufficient memory\n";
			return SH_P_NOT_ENOUGH_MEMORY;
		}
		memset(p_nullable, 0, sizeof(bool)*p_nonterminalCount);
		p_firstCompleteFromNowOn = false;
		bool foundMoreNullables;
		do
		{
			foundMoreNullables = false;
			for (map<int, shRule>::const_iterator it = p_ruleByCode.begin(), end = p_ruleByCode.end(); it != end; ++it)
			{
				if (p_nullable[p_nonterminalCodes[it->second.head]])
					continue;
				bool nullable = true;
				for (list<string>::const_iterator iter = it->second.body.begin(), end2 = it->second.body.end(); iter != end2; ++iter)
					if (p_tokenTypes.find(*iter) != p_tokenTypes.end() ||
							!p_nullable[p_nonterminalCodes[*iter]])
					{
						nullable = false;
						break;
					}
				if (nullable)
				{
					p_nullable[p_nonterminalCodes[it->second.head]] = true;
					foundMoreNullables = true;
				}
			}
		} while (foundMoreNullables);
#ifdef SH_DEBUG
		for (int i = 0; i < p_nonterminalCount; ++i)
			cout << p_nonterminals[i] << (p_nullable[i]?" IS nullable":" is not nullable") << endl;
#endif
		p_findFirstData d;
		// Compute and store firsts
		for (int k = 1; k <= p_k_in_grammar; ++k)
		{
			for (vector<string>::const_iterator iter = p_nonterminals.begin(), end = p_nonterminals.end(); iter != end; ++iter)
			{
				d.sentence.clear();
				d.sentence.push_back(*iter);
				d.k = k;
				d.shallEnd = true;
				p_first[d] = set<list<string> >();
				d.shallEnd = false;
				p_first[d] = set<list<string> >();
			}
			p_moreFirstFound = true;
			while (p_moreFirstFound)
			{
				p_moreFirstFound = false;
				for (vector<string>::const_iterator iter = p_nonterminals.begin(), end = p_nonterminals.end(); iter != end; ++iter)
				{
					list<string> nonterminal;
					nonterminal.push_back(*iter);
					p_firstDFSVisited.clear();
					set<list<string> > res = p_findLRKFirst(nonterminal, k, true);
					d.sentence = nonterminal;
					d.shallEnd = true;
					for (set<list<string> >::const_iterator it = res.begin(), end2 = res.end(); it != end2; ++it)
						if (p_first[d].find(*it) == p_first[d].end())
						{
							p_first[d].insert(*it);
							p_moreFirstFound = true;
						}
					p_firstDFSVisited.clear();
					p_findLRKFirst(nonterminal, k, false);
					d.shallEnd = false;
					for (set<list<string> >::const_iterator it = res.begin(), end2 = res.end(); it != end2; ++it)
						if (p_first[d].find(*it) == p_first[d].end())
						{
							p_first[d].insert(*it);
							p_moreFirstFound = true;
						}
				}
			}
			for (vector<string>::const_iterator iter = p_nonterminals.begin(), end = p_nonterminals.end(); iter != end; ++iter)
			{
				list<string> temp;
				temp.push_back(*iter);
				p_firstComplete[p_findFirstData(temp, k, false)] = true;
				p_firstComplete[p_findFirstData(temp, k, true)] = true;
			}
		}
		p_firstCompleteFromNowOn = true;
#ifdef SH_DEBUG
		cout << "Firsts:" << endl;
		for (map<p_findFirstData, set<list<string> > >::const_iterator iter = p_first.begin(), end = p_first.end(); iter != end; ++iter)
		{
			for (list<string>::const_iterator i = iter->first.sentence.begin(), end2 = iter->first.sentence.end(); i != end2; ++i)
				cout << *i << " ";
			cout << "with k = " << iter->first.k << " which must " << (iter->first.shallEnd?"end":"not necessarily end") << endl;
			for (set<list<string> >::const_iterator i = iter->second.begin(), end2 = iter->second.end(); i != end2; ++i)
			{
				cout << "\t";
				for (list<string>::const_iterator it = i->begin(), end3 = i->end(); it != end3; ++it)
					if (*it == "")
						cout << " Error-Error!!";
					else
						cout << " " << *it;
				cout << endl;
			}
			cout << endl;
		}
#endif
		// Prepare to compute follows
		p_parentAndAfter.clear();
		for (int i = 0; i < p_ruleCount; ++i)
		{
			list<string> follow = p_ruleByCode[i].body;
			for (list<string>::const_iterator iter = p_ruleByCode[i].body.begin(), end = p_ruleByCode[i].body.end(); iter != end; ++iter)
			{
				follow.pop_front();
				if (p_tokenTypes.find(*iter) != p_tokenTypes.end())
					continue;
				p_parentAndAfter[*iter].insert(p_parentAndAfterData(p_ruleByCode[i].head, follow));
			}
		}
		p_followGraphAdjList.clear();
		for (int k = 0; k <= p_k_in_grammar; ++k)
			for (vector<string>::const_iterator nont = p_nonterminals.begin(), end = p_nonterminals.end(); nont != end; ++nont)
				p_findFollowDependency(*nont, k);
		bool loopExists;
		do
		{
			loopExists = false;
			for (int k = 0; k <= p_k_in_grammar; ++k)
				for (vector<string>::const_iterator nont = p_nonterminals.begin(), end = p_nonterminals.end(); nont != end; ++nont)
				{
					p_followFoundLoop = false;
					p_followDFSVisited.clear();
					p_removeCycles(pair<string, int>(*nont, k));
					loopExists = loopExists||p_followFoundLoop;
				}
		} while (loopExists);
		p_followDFSVisited.clear();
		p_updateParentAndAfter();
		// Compute and store follows
		for (int k = 1; k <= p_k_in_grammar; ++k)
			for (int i = 0; i < p_nonterminalCount; ++i)
				p_findFollow(p_nonterminals[i], k);
#ifdef SH_DEBUG
		cout << "Follows:" << endl;
		for (int i = 0; i < p_nonterminalCount; ++i)
		{
			cout << p_nonterminals[i] << " has follows:" << endl;
			for (int k = 1; k <= p_k_in_grammar; ++k)
			{
				cout << "\tWith length " << k << ":" << endl;
				pair<string, int> parent = p_findParent(pair<string, int>(p_nonterminals[i], k));
				if (parent != pair<string, int>(p_nonterminals[i], k))
				{
					cout << "\t\tSame as: " << parent.first << " with length " << parent.second << endl;
					continue;
				}
				for (set<list<string> >::const_iterator iter = p_follow[parent].begin(), end = p_follow[parent].end(); iter != end; ++iter)
				{
					cout << "\t\t";
					for (list<string>::const_iterator s = iter->begin(), end2 = iter->end(); s != end2; ++s)
						if (*s == "")
							cout << "$ ";
						else
							cout << *s << " ";
					cout << endl;
				}
			}
		}
#endif
		map<p_LRK_state, int> p_LRK_states;		// p_LRK_states maps a set of LR(k) items (which form a state in the diagram) to a number
		p_LRK_state state;
		set<list<string> > follows;
		list<string> allDollars;
		for (int i = 0; i < p_k_in_grammar; ++i)
			allDollars.push_back("");
		follows.insert(allDollars);
		state[p_ruleWithDot(p_ruleCount, 0)] = follows;
		p_closure(state);
		p_LRK_states[state] = 0;
		int p_stateCount = 1;
		bool ambiguous = false;
		queue<p_LRK_state> Q;
		Q.push(state);
		while (!Q.empty())
		{
			state = Q.front();
			Q.pop();
			int stateCode = p_LRK_states[state];
			map<p_LRK_state, int> tempStates;
			map<int, p_LRK_state> stateByCode;
			map<p_LRK_diagram_lookup, p_LRK_diagram_data> tempDiagram;
			tempStates[state] = 0;
			int tempStateCount = 1;
			for (p_LRK_state::const_iterator i = state.begin(), end = state.end(); i != end; ++i)
			{
				p_ruleWithDot ruleWithDot = i->first;
				shRule rule = p_ruleByCode[ruleWithDot.rule];
				if ((int)rule.body.size() == ruleWithDot.dotPosition)	// Nothing after the dot
				{
					for (set<list<string> >::const_iterator iter = i->second.begin(), end2 = i->second.end(); iter != end2; ++iter)
						if (p_LRK_diagram.find(p_LRK_diagram_lookup(stateCode, *iter)) != p_LRK_diagram.end() &&
								p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] !=
								p_LRK_diagram_data(REDUCE, ruleWithDot.rule))
						{
							char m[1000];
							sprintf(m, "Conflict in state %d with look-ahead", stateCode);
							p_errorMessage += m;
							for (list<string>::const_iterator ii = iter->begin(), end3 = iter->end(); ii != end3; ++ii)
								if (*ii == "")
									p_errorMessage += " $";
								else
									p_errorMessage += ' '+*ii;
							p_LRK_diagram_data other = p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)];
							sprintf(m, " which can either %s %d or reduce with rule %d\n",
									(other.transition == ACCEPT)?"lead to accept":
									(other.transition == REDUCE)?"reduce with rule":
									"shift and go to state", other.toStateOrRule,
									ruleWithDot.rule);
							p_errorMessage += m;
							if (other.transition == REDUCE)
								PRINT_RULE(other.toStateOrRule);
							PRINT_RULE(ruleWithDot.rule);
							if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_SHIFT)
								p_errorMessage += "Prioritizing shift.\n";
							else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_REDUCE)
							{
								p_errorMessage += "Prioritizing reduce.\n";
								if (ruleWithDot.rule == p_ruleCount)	// Accept
									ambiguous = true;
								else					// Reduce
									p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] = p_LRK_diagram_data(REDUCE, ruleWithDot.rule);
							}
							else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST)
								p_errorMessage += "Ignoring the later rule in this case.\n";
							else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST)
							{
								p_errorMessage += "Ignoring the former rule in this case.\n";
								if (ruleWithDot.rule == p_ruleCount)	// Accept
									p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] = p_LRK_diagram_data(ACCEPT, p_ruleCount);
								else					// Reduce
									p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] = p_LRK_diagram_data(REDUCE, ruleWithDot.rule);
							}
							else
								ambiguous = true;
						}
						else
							if (ruleWithDot.rule == p_ruleCount)	// Accept
								p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] = p_LRK_diagram_data(ACCEPT, p_ruleCount);
							else					// Reduce
								p_LRK_diagram[p_LRK_diagram_lookup(stateCode, *iter)] = p_LRK_diagram_data(REDUCE, ruleWithDot.rule);
					continue;
				}
				for (int x = 0; x < ruleWithDot.dotPosition; ++x)
					rule.body.pop_front();
				if (p_tokenTypes.find(rule.body.front()) == p_tokenTypes.end())	// It was a nonterminal!
				{
					list<string> nonterminalItself;
					nonterminalItself.push_back(rule.body.front());
					p_LRK_state newState;
					newState[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
					if (tempDiagram.find(p_LRK_diagram_lookup(0, nonterminalItself)) != tempDiagram.end())
					{
						tempStates.erase(stateByCode[tempDiagram[p_LRK_diagram_lookup(0, nonterminalItself)].toStateOrRule]);
						stateByCode[tempDiagram[p_LRK_diagram_lookup(0, nonterminalItself)].toStateOrRule]
							[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
						tempStates[stateByCode[tempDiagram[p_LRK_diagram_lookup(0, nonterminalItself)].toStateOrRule]] =
							tempDiagram[p_LRK_diagram_lookup(0, nonterminalItself)].toStateOrRule;
					}
					else
					{
						if (tempStates.find(newState) == tempStates.end())
						{
							tempStates[newState] = tempStateCount;
							stateByCode[tempStateCount++] = newState;
						}
						tempDiagram[p_LRK_diagram_lookup(0, nonterminalItself)] = p_LRK_diagram_data(REDUCE, tempStates[newState]);
					}
					continue;
				}
				follows = i->second;
				for (set<list<string> >::const_iterator iter = follows.begin(), end2 = follows.end(); iter != end2; ++iter)
				{
					list<string> temp = rule.body;
					for (list<string>::const_iterator it = iter->begin(), end3 = iter->end(); it != end3; ++it)
						temp.push_back(*it);
					set<list<string> > transitions = p_findLRKFirst(temp, p_k_in_grammar, false);
					for (set<list<string> >::const_iterator it = transitions.begin(), end3 = transitions.end(); it != end3; ++it)
					{
						p_LRK_state newState;
						newState[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
						if (tempDiagram.find(p_LRK_diagram_lookup(0, *it)) != tempDiagram.end())
							if (tempDiagram[p_LRK_diagram_lookup(0, *it)].transition != SHIFT)
							{
								char m[1000];
								sprintf(m, "Conflict in state %d with look-ahead", stateCode);
								p_errorMessage += m;
								for (list<string>::const_iterator ii = it->begin(), end4 = it->end(); ii != end4; ++ii)
									if (*ii == "")
										p_errorMessage += " $";
									else
										p_errorMessage += ' '+*ii;
								p_LRK_diagram_data other = tempDiagram[p_LRK_diagram_lookup(0, *it)];
								sprintf(m, " which can either %s or shift\n", (other.transition == ACCEPT)?"lead to accept":"reduce");
								p_errorMessage += m;
								if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_SHIFT)
								{
									p_errorMessage += "Prioritizing shift.\n";
									stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]
										[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
									tempStates[stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]] =
										tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule;
								}
								else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_REDUCE)
									p_errorMessage += "Prioritizing reduce.\n";
								else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST)
									p_errorMessage += "Ignoring the later rule in this case.\n";
								else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST)
								{
									p_errorMessage += "Ignoring the former rule in this case.\n";
									stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]
										[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
									tempStates[stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]] =
										tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule;
								}
								else
									ambiguous = true;
							}
							else
							{
								tempStates.erase(stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]);
								stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]
									[p_ruleWithDot(ruleWithDot.rule, ruleWithDot.dotPosition+1)] = i->second;
								tempStates[stateByCode[tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule]] =
									tempDiagram[p_LRK_diagram_lookup(0, *it)].toStateOrRule;
							}
						else
						{
							if (tempStates.find(newState) == tempStates.end())
							{
								tempStates[newState] = tempStateCount;
								stateByCode[tempStateCount++] = newState;
							}
							tempDiagram[p_LRK_diagram_lookup(0, *it)] = p_LRK_diagram_data(SHIFT, tempStates[newState]);
						}
					}
				}
			}
			map<int, int> tempStateToState;
			tempStateToState[0] = stateCode;
			for (map<p_LRK_diagram_lookup, p_LRK_diagram_data>::const_iterator iter = tempDiagram.begin(), end = tempDiagram.end();
					iter != end; ++iter)
			{
				int toStateCode = iter->second.toStateOrRule;
				if (tempStateToState.find(iter->second.toStateOrRule) == tempStateToState.end())
				{
					p_LRK_state toState = stateByCode[iter->second.toStateOrRule];
					p_closure(toState);
					if (p_LRK_states.find(toState) == p_LRK_states.end())
					{
						tempStateToState[iter->second.toStateOrRule] = toStateCode = p_stateCount;
						p_LRK_states[toState] = p_stateCount++;
						Q.push(toState);
					}
					else
						toStateCode = p_LRK_states[toState];
				}
				else
					toStateCode = tempStateToState[iter->second.toStateOrRule];
				if (p_LRK_diagram.find(p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)) != p_LRK_diagram.end() &&
						p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)] !=
						p_LRK_diagram_data(iter->second.transition, toStateCode))
				{
					char m[1000];
					sprintf(m, "Conflict in state %d with look-ahead", stateCode);
					p_errorMessage += m;
					for (list<string>::const_iterator ii = iter->first.lookAhead.begin(), end2 = iter->first.lookAhead.end();
							ii != end2; ++ii)
						if (*ii == "")
							p_errorMessage += " $";
						else
							p_errorMessage += ' '+*ii;
					p_LRK_diagram_data other = p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)];
					sprintf(m, " which can either %s %d or %s %d\n",
							(other.transition == ACCEPT)?"lead to accept":
							(other.transition == REDUCE)?"reduce with rule":"shift and go to state", other.toStateOrRule,
							(iter->second.transition == ACCEPT)?"lead to accept":
							(iter->second.transition == REDUCE)?"reduce with rule":"shift and go to state", toStateCode);
					p_errorMessage += m;
					if (other.transition == REDUCE)
						PRINT_RULE(other.toStateOrRule);
					if (iter->second.transition == REDUCE)
						PRINT_RULE(toStateCode);
					if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST)
						p_errorMessage += "Ignoring the later rule in this case.\n";
					else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST)
					{
						p_errorMessage += "Ignoring the former rule in this case.\n";
						p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)] = p_LRK_diagram_data(iter->second.transition, toStateCode);
					}
					else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_SHIFT)
					{
						p_errorMessage += "Prioritizing shift.\n";
						if (other.transition == SHIFT && iter->second.transition != SHIFT)
						{ /* previous value is good, do nothing */ }
						else if (other.transition != SHIFT && iter->second.transition == SHIFT)
							p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)] = p_LRK_diagram_data(iter->second.transition, toStateCode);
						else
							ambiguous = true;
					}
					else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_REDUCE)
					{
						p_errorMessage += "Prioritizing reduce.\n";
						if (other.transition == REDUCE && iter->second.transition != REDUCE)
						{ /* previous value is good, do nothing */ }
						else if (other.transition != REDUCE && iter->second.transition == REDUCE)
							p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)] = p_LRK_diagram_data(iter->second.transition, toStateCode);
						else
							ambiguous = true;
					}
					else
						ambiguous = true;
				}
				else
					p_LRK_diagram[p_LRK_diagram_lookup(stateCode, iter->first.lookAhead)] = p_LRK_diagram_data(iter->second.transition, toStateCode);
			}

		}
#ifdef SH_DEBUG
		cout << "There exists " << p_stateCount << " state(s) in the CLR(k) diagram" << endl;
		for (map<p_LRK_state, int>::const_iterator it = p_LRK_states.begin(), end = p_LRK_states.end(); it != end; ++it)
		{
			cout << "state[" << it->second << "]:" << endl;
			for (p_LRK_state::const_iterator i = it->first.begin(), end2 = it->first.end(); i != end2; ++i)
			{
				shRule rule = p_ruleByCode[i->first.rule];
				cout << rule.head << " ->";
				int x = 0;
				for (list<string>::const_iterator iter = rule.body.begin(), end3 = rule.body.end(); iter != end3; ++iter, ++x)
				{
					if (x == i->first.dotPosition)
						cout << " .";
					cout << " " << *iter;
				}
				if (i->first.dotPosition == (int)rule.body.size())
					cout << " .";
				cout << " ------ with follows:" << endl;
				for (set<list<string> >::const_iterator iter = i->second.begin(), end3 = i->second.end(); iter != end3; ++iter)
				{
					cout << "\t";
					for (list<string>::const_iterator it = iter->begin(), end4 = iter->end(); it != end4; ++it)
						if (*it == "")
							cout << " $";
						else
							cout << *it << " ";
					cout << endl;
				}
			}
		}
#endif
#ifdef SH_DEBUG
		for (map<p_LRK_diagram_lookup, p_LRK_diagram_data>::const_iterator iter = p_LRK_diagram.begin(), end = p_LRK_diagram.end();
				iter != end; ++iter)
		{
			cout << "(" << iter->first.state << ",";
			for (list<string>::const_iterator i = iter->first.lookAhead.begin(), end2 = iter->first.lookAhead.end(); i != end2; ++i)
				if (*i == "")
					cout << " $";
				else
					cout << " " << *i;
			cout << ") => ";
			switch (iter->second.transition)
			{
				case SHIFT:
					cout << "S" << iter->second.toStateOrRule;
					break;
				case REDUCE:
					if (p_nonterminalCodes.find(iter->first.lookAhead.front()) != p_nonterminalCodes.end() &&
							iter->first.lookAhead.front() != "")
						cout << iter->second.toStateOrRule;
					else
						cout << "R" << iter->second.toStateOrRule;
					break;
				case ACCEPT:
					cout << "A";
			}
			cout << endl;
		}
#endif
		delete[] p_nullable;
		p_cleanupIntermediateData();
		if (ambiguous)
			return SH_P_AMBIGUITY;
		ofstream tout(FIX_PATH(tableFile).c_str());
		for (map<p_LRK_diagram_lookup, p_LRK_diagram_data>::const_iterator iter = p_LRK_diagram.begin(), end = p_LRK_diagram.end();
				iter != end; ++iter)
		{
			tout << iter->first.state << " " << iter->second.transition << " " << iter->second.toStateOrRule << " " <<
				iter->first.lookAhead.size();
			for (list<string>::const_iterator i = iter->first.lookAhead.begin(), end2 = iter->first.lookAhead.end(); i != end2; ++i)
				// If it is $, do not write it, it will be obvious because of the line having less tokens!
				if (*i != "")
					tout << " " << *i;
			tout << endl;
		}
	}
	p_LRK_diagramBuilt = true;
	return SH_P_SUCCESS;
}

int shParser::shPParseLRK(shLexer &lexer)
{
	if (!lexer.p_initialized)
	{
		p_errorMessage = "Lexer not initialized\n";
		return SH_P_LEXER_NOT_INITIALIZED;
	}
	if (!p_LRK_diagramBuilt)
	{
		p_errorMessage = "Rules not yet read!\n";
		return SH_P_RULES_NOT_LOADED;
	}
	shPReset();
	for (map<string, shPActionRoutineFunction>::const_iterator iter = p_actionRoutine.begin(), end = p_actionRoutine.end(); iter != end; ++iter)
		if (iter->second == _noFunctionForActionRoutine)
			p_errorMessage += "Warning: No function is assigned to action routine "+iter->first+"\n";
	for (map<string, set<int> >::const_iterator iter = p_ruleEPRHandler.begin(), end = p_ruleEPRHandler.end(); iter != end; ++iter)
		for (set<int>::const_iterator iter2 = iter->second.begin(), end2 = iter->second.end(); iter2 != end2; ++iter2)
			if (p_LRK_EPRHandleFunction[*iter2] == _noFunctionForLRKErrorProductionRule)
			{
				p_errorMessage += "Warning: No function is assigned to error production rule handler "+iter->first+"\n";
				break;
			}
	shKLimitedList<string> lookAhead(p_k_in_grammar, p_k_in_grammar, "");
	shKLimitedList<string> lookAheadTokens(p_k_in_grammar, p_k_in_grammar, "");
	lookAhead.useHistory = true;
	lookAheadTokens.useHistory = true;
	string type, token;
	lookAhead << type;			// To have at least one element in history so that if an action routine
	lookAheadTokens << token;		// was executed before any read operation, a "" be sent to it
	for (int i = 0; i < p_k_in_grammar; ++i)
	{
		p_LRK_parseStack.push(shLRKStackElement("", 0));
		token = lexer.shLPeekNextToken(type, i);
#ifdef SH_DEBUG
		cout << "read " << token << " from input which is of type: " << type << endl;
#endif
		lookAhead << type;
		lookAheadTokens << token;
	}
	while (true)
	{
		if (p_LRK_diagram.find(p_LRK_diagram_lookup(p_LRK_parseStack.top().state, lookAhead.k_elements())) == p_LRK_diagram.end())
		{
			if (!(*p_LRK_pleh)(p_LRK_parseStack, lexer, p_errorMessage, lookAhead, lookAheadTokens))
				return SH_P_PARSE_ERROR;
			else
				continue;
		}
		list<string> handle;
		shRule rule;
		list<string> nont;
		p_LRK_diagram_data transition = p_LRK_diagram[p_LRK_diagram_lookup(p_LRK_parseStack.top().state, lookAhead.k_elements())];
		p_LRK_diagram_data transitionWithNont;
		switch (transition.transition)
		{
		case ACCEPT:
			if (lookAheadTokens.k_elements().front() != "")
			{
				char temp[1001];
				snprintf(temp, 1000, "%s:%d:%d: warning: the parser has successfully finished, but there are more lines in the input file\n",
						lexer.shLFileName().c_str(), lexer.shLLine(), lexer.shLColumn());
				p_errorMessage += temp;
			}
			return SH_P_SUCCESS;
		case SHIFT:
			lexer.shLNextToken(type);			// This should be the one just matched.
			token = lexer.shLPeekNextToken(type, p_k_in_grammar-1);
#ifdef SH_DEBUG
			cout << "shifting " << lookAhead.k_elements().front() << endl;
			cout << "read " << token << " from input (line " << lexer.shLLine() << ") which is of type: " << type << endl;
#endif
			p_LRK_parseStack.push(shLRKStackElement(lookAhead.k_elements().front(), transition.toStateOrRule));
			lookAhead << type;
			lookAheadTokens << token;
			break;
		case REDUCE:
			rule = p_ruleByCode[transition.toStateOrRule];
			(*p_LRK_EPRHandleFunction[transition.toStateOrRule])(p_LRK_parseStack, lexer, p_errorMessage, rule);
			for (unsigned int i = 0; i < rule.body.size(); ++i)
			{
				handle.push_front(p_LRK_parseStack.top().terminalOrNonterminal);
				p_LRK_parseStack.pop();
			}
			for (list<string>::const_iterator ri = rule.body.begin(), hi = handle.begin(), end1 = rule.body.end(), end2 = handle.end();
					ri != end1 && hi != end2; ++ri, ++hi)
				if (*ri != *hi)
				{
					char temp[1001];
					sprintf(temp, "Parse error on line %d: Found handle:", lexer.shLLine());
					p_errorMessage += temp;
					for (list<string>::const_iterator iter = handle.begin(), end3 = handle.end(); iter != end3; ++iter)
						p_errorMessage += " "+((*iter == "")?string(" $"):*iter);
					p_errorMessage += " but the rule to reduce is: "+rule.head+" ->";
					for (list<string>::const_iterator iter = rule.body.begin(), end3 = rule.body.end(); iter != end3; ++iter)
						p_errorMessage += " "+((*iter == "")?string(" $"):*iter);
					p_errorMessage += "\nCould not recover from last error!\n";
					return SH_P_PARSE_ERROR;
				}
			if (p_ruleActionRoutine.find(transition.toStateOrRule) != p_ruleActionRoutine.end())
			{
				bool ignoredWhiteSpace = lexer.shLIgnoresWhiteSpace();
				(*p_actionRoutine[p_ruleActionRoutine[transition.toStateOrRule]])(lexer, *this);
				if (ignoredWhiteSpace != lexer.shLIgnoresWhiteSpace())		// Then the action routine has issued lexer.shLIgnoreWhiteSpace and thus
												// the look-aheads must be retaken
				{
					lookAhead.flushList();
					lookAheadTokens.flushList();
					for (int i = 0; i < p_k_in_grammar; ++i)
					{
						token = lexer.shLPeekNextToken(type, i);
#ifdef SH_DEBUG
						cout << "read " << token << " from input which is of type: " << type << endl;
#endif
						lookAhead << type;
						lookAheadTokens << token;
					}
				}
			}
			nont.clear();
			nont.push_back(rule.head);
			if (p_LRK_diagram.find(p_LRK_diagram_lookup(p_LRK_parseStack.top().state, nont)) == p_LRK_diagram.end())
			{
				char temp[1001];
				snprintf(temp, 1000, "Parse error on line %d:  No transition rule from state %d"
						" with nonterminal %s", lexer.shLLine(), p_LRK_parseStack.top().state, rule.head.c_str());
				p_errorMessage += temp;
				p_errorMessage += "\nCould not recover from last error!\n";
				return SH_P_PARSE_ERROR;
			}
#ifdef SH_DEBUG
			cout << "reducing to " << rule.head << endl;
#endif
			transitionWithNont = p_LRK_diagram[p_LRK_diagram_lookup(p_LRK_parseStack.top().state, nont)];
			p_LRK_parseStack.push(shLRKStackElement(rule.head, transitionWithNont.toStateOrRule));
			break;
		default:
			p_errorMessage += "Internal error!\nCould not recover from last error!\n";
			return SH_P_PARSE_ERROR;
		}
	}
	return SH_P_SUCCESS;
}

void shParser::shPAddLRK_EPRHandler(const string &EPRHandler, shPLRK_EPRHandler h)
{
	string handler;
	if (EPRHandler[0] != '!')
		handler = '!'+EPRHandler;
	else
		handler = EPRHandler;
	for (set<int>::const_iterator i = p_ruleEPRHandler[handler].begin(), end = p_ruleEPRHandler[handler].end(); i != end; ++i)
		p_LRK_EPRHandleFunction[*i] = h;
}

void shParser::shPSetLRK_PLErrorHandler(shPLRK_PLErrorHandler h)
{
	p_LRK_pleh = h;
}
