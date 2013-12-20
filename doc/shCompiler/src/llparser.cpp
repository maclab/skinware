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
static void _noFunctionForLLKErrorProductionRule(stack<shParser::shLLKStackElement> &, shLexer &, string &, shParser::shRule &){}
static bool _noFunctionForLLKPhraselevelErrorHandler(stack<shParser::shLLKStackElement> &,
		shLexer &, string &, string &, shKLimitedList<string> &, shKLimitedList<string> &){return false;}

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

int shParser::shPReadLLKGrammar(const shLexer &lexer, const char *grammarFile, int k, bool newGrammar)
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
	p_LLK_EPRHandleFunction.clear();
	p_LLK_pleh = _noFunctionForLLKPhraselevelErrorHandler;
	p_ruleByCode.clear();
	p_ruleByCodeWithActionRoutine.clear();
	p_rulesByNonterminalCode.clear();
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
	for (list<shRule>::const_iterator i = rules.begin(), end = rules.end(); i != end; ++i)
	{
		shRule rule;
		shRule ruleWithActionRoutine;
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
		ruleWithActionRoutine.head = i->head;
		for (list<string>::const_iterator j = i->body.begin(), end2 = i->body.end(); j != end2; ++j)
			if ((*j)[0] == '@')
			{
				ruleWithActionRoutine.body.push_back(*j);
				p_actionRoutine[*j] = _noFunctionForActionRoutine;
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
				ruleWithActionRoutine.body.push_back(*j);
			}
		p_ruleByCode[p_ruleCount] = rule;
		p_ruleByCodeWithActionRoutine[p_ruleCount] = ruleWithActionRoutine;
		p_rulesByNonterminalCode[p_nonterminalCodes[rule.head]].insert(p_ruleCount);
		p_LLK_EPRHandleFunction[p_ruleCount] = _noFunctionForLLKErrorProductionRule;
		++p_ruleCount;
	}
	if (p_ruleCount == 0)					// There were no rules in the file
	{
		p_errorMessage = "There are no rules in the given file\n";
		return SH_P_GRAMMAR_ERROR;
	}
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
	return p_shPPrepareLLKParse(FIX_PATH(cachePath+grammarName+".LLKtable").c_str(), !newGrammar);
}

// This function, recurses on the LL(k) grammar and finds out
// different combinations reachable by the symbol or sentence with length k
// It uses a map to memoize the recursion
// The grammar must be ensured not to be left recursive first
set<list<string> > shParser::p_findLLKFirst(list<string> sen, int k, bool shallEnd)
{
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "checking for:";
	for (list<string>::const_iterator iter = sen.begin(), end = sen.end(); iter != end; ++iter)
		cout << " " << *iter;
	cout << " with length " << k << " which must " << (shallEnd?"end":"not necessarily end") << endl;
#endif
	set<list<string> > answer;
	list<string> x;
	set<list<string> > only_x;
	only_x.insert(x);
	if (sen.size() == 0 || sen.front() == "")		// if seeing $, that means all the rest are $ (that is after the text has finished)
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
				return only_x;			// All followings are nullable, so return an empty list. This means that the sequences
								// that lead to this sentence will be accepted
			}
			else					// Then found all and we don't have to end it
				return only_x;			// an empty list
		}
	if (p_tokenTypes.find(sen.front()) != p_tokenTypes.end())
	{
		string s = sen.front();
		sen.pop_front();
		set<list<string> > temp = p_findLLKFirst(sen, k-1, shallEnd);
		for (set<list<string> >::const_iterator iter = temp.begin(), end = temp.end(); iter != end; ++iter)
		{
			x = *iter;
			x.push_front(s);
			answer.insert(x);
		}
		sen.push_front(s);
		p_first[p_findFirstData(sen, k, shallEnd)] = answer;
	}
	else if (p_first.find(p_findFirstData(sen, k, shallEnd)) != p_first.end())
	{
		return p_first[p_findFirstData(sen, k, shallEnd)];
	}
	else if (sen.size() == 1)
	{
		set<int> to = p_rulesByNonterminalCode[p_nonterminalCodes[sen.front()]];
		for (set<int>::const_iterator i = to.begin(), end = to.end(); i != end; ++i)
		{
			set<list<string> > temp = p_findLLKFirst(p_ruleByCode[*i].body, k, shallEnd);
			for (set<list<string> >::const_iterator iter = temp.begin(), end2 = temp.end(); iter != end2; ++iter)
				answer.insert(*iter);
		}
		p_first[p_findFirstData(sen, k, shallEnd)] = answer;
	}
	else
	{
		for (int i = 0; i <= k; ++i)
		{
			list<string> temp;
			temp.push_back(sen.front());
			set<list<string> > s1 = p_findLLKFirst(temp, i, (i!=k)?true:shallEnd);
			if (s1.size() != 0)
			{
				temp = sen;
				temp.pop_front();
				set<list<string> > s2 = p_findLLKFirst(temp, k-i, shallEnd);
				for (set<list<string> >::const_iterator iter1 = s1.begin(), end = s1.end(); iter1 != end; ++iter1)
					for (set<list<string> >::const_iterator iter2 = s2.begin(), end2 = s2.end(); iter2 != end2; ++iter2)
					{
						list<string> result = *iter1;
						for (list<string>::const_iterator iter = iter2->begin(), end3 = iter2->end(); iter != end3; ++iter)
							result.push_back(*iter);
						answer.insert(result);
					}
			}
		}
		p_first[p_findFirstData(sen, k, shallEnd)] = answer;
	}
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
	cout << "returning answer..." << endl;
#endif
	return answer;
}

// returns true on left recursion
int shParser::p_checkNullable(int nonterminal)
{
	if (p_visited[nonterminal])
	{
#if defined(SH_DEBUG) && defined(SH_DETAILED_DEBUG)
		cout << nonterminal << " revisited" << endl;
#endif
		return nonterminal;
	}
	p_visited[nonterminal] = true;
	for (set<int>::const_iterator i = p_rulesByNonterminalCode[nonterminal].begin(), end = p_rulesByNonterminalCode[nonterminal].end(); i != end; ++i)
	{
		shRule rule = p_ruleByCode[*i];
		bool nullable = false;
		if (rule.body.size() == 0)
			nullable = true;
		else
		{
			nullable = true;
			for (list<string>::const_iterator iter = rule.body.begin(), end2 = rule.body.end(); iter != end2; ++iter)
			{
				if (p_tokenTypes.find(*iter) != p_tokenTypes.end())
				{
					nullable = false;
					break;
				}
				int nullableNonterminal;
				if ((nullableNonterminal = p_checkNullable(p_nonterminalCodes[*iter])) != -1)
				{
					p_nullable[nullableNonterminal] = true;
					return nullableNonterminal;
				}
				if (!p_nullable[p_nonterminalCodes[*iter]])
				{
					nullable = false;
					break;
				}
			}
		}
		p_nullable[nonterminal] |= nullable;
	}
	p_visited[nonterminal] = false;
	return -1;
}

// builds the LL(k) table if load was false and stores it in tableFile. If load was true, loads table from tableFile
int shParser::p_shPPrepareLLKParse(const char *tableFile, bool load)
{
	p_LLK_tableBuilt = false;
	ifstream tin(FIX_PATH(tableFile).c_str());
	if (load && tin.is_open())
	{
#ifdef SH_DEBUG
		cout << "Reading LLK table from file" << endl;
#endif
		string line;
		string nonterminal;
		p_LLK_table_data tdata;
		while (tin >> nonterminal >> tdata.rule)
		{
			getline(tin, line);
			istringstream sin(line);
			list<string> tokens;
			for (int i = 0; i < p_k_in_grammar; ++i)
			{
				string tmp;
				sin >> tmp;					// If it was $, then "" would be read ;)
				tokens.push_back(tmp);
			}
			p_LLK_table[p_LLK_table_lookup(nonterminal, tokens)] = tdata;
		}
	}
	else
	{
#ifdef SH_DEBUG
		cout << "Rebuilding LLK table from grammar" << endl;
#endif
		if (tin.is_open())
			tin.close();
		p_nullable = new bool[p_nonterminalCount];
		p_visited = new bool[p_nonterminalCount];
		if (!p_nullable || !p_visited)
		{
			if (p_nullable)
				delete[] p_nullable;
			if (p_visited)
				delete[] p_visited;
			p_errorMessage = "Insufficient memory\n";
			return SH_P_NOT_ENOUGH_MEMORY;
		}
		memset(p_nullable, 0, p_nonterminalCount*sizeof(bool));
		for (int i = 0; i < p_nonterminalCount; ++i)
		{
			memset(p_visited, 0, p_nonterminalCount*sizeof(bool));
			int nullableNonterminal;
			if ((nullableNonterminal = p_checkNullable(i)) != -1)		// During nullability check, a left recursion occurred
			{
				delete[] p_visited;
				delete[] p_nullable;
				char temp[1001];
				snprintf(temp, 1000, "Grammar is left recursive (nonterminal: %s)\n", p_nonterminals[nullableNonterminal].c_str());
				p_errorMessage = temp;
				p_cleanupIntermediateData();
				return SH_P_LEFT_RECURSION;
			}
		}
#ifdef SH_DEBUG
		for (int i = 0; i < p_nonterminalCount; ++i)
			cout << p_nonterminals[i] << (p_nullable[i]?" IS nullable":" is not nullable") << endl;
#endif
		delete[] p_visited;
		p_first.clear();
		p_follow.clear();
#ifdef SH_DEBUG
		cout << "Firsts:" << endl;
		// This is to test correctness of p_findLLKFirst
		for (int k = 1; k <= p_k_in_grammar; ++k)
			for (int t = 0; t < p_nonterminalCount; ++t)
			{
				list<string> nonterminal;
				nonterminal.push_back(p_nonterminals[t]);
				p_findLLKFirst(nonterminal, k, false);
				p_findLLKFirst(nonterminal, k, true);
			}
		cout << endl;
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
		p_parentAndAfter.clear();
		for (int i = 0; i < p_ruleCount; ++i)
		{
			shRule follow = p_ruleByCode[i];
			for (list<string>::const_iterator iter = p_ruleByCode[i].body.begin(), end = p_ruleByCode[i].body.end(); iter != end; ++iter)
			{
				follow.body.pop_front();
				if (p_tokenTypes.find(*iter) != p_tokenTypes.end())
					continue;
				p_parentAndAfter[*iter].insert(p_parentAndAfterData(p_ruleByCode[i].head, follow.body));
			}
		}
#ifdef SH_DEBUG
		cout << "Parent and After:" << endl;
		// This is to test correctness of p_parentAndAfter		// TODO: add code to look at p_parentAndAfter2. It looks like it's the same!!
		for (int i = 0; i < p_nonterminalCount; ++i)
			for (set<p_parentAndAfterData>::const_iterator iter = p_parentAndAfter[p_nonterminals[i]].begin(),
					end = p_parentAndAfter[p_nonterminals[i]].end(); iter != end; ++iter)
			{
				cout << p_nonterminals[i] << " is derived from " << iter->parent << " and after it is:";
				for (list<string>::const_iterator it = iter->after.begin(), end2 = iter->after.end(); it != end2; ++it)
					cout << " " << *it;
				cout << endl;
			}
#endif
		p_followGraphAdjList.clear();
		for (int k = 0; k <= p_k_in_grammar; ++k)
			for (vector<string>::const_iterator nont = p_nonterminals.begin(), end = p_nonterminals.end(); nont != end; ++nont)
				p_findFollowDependency(*nont, k);
#ifdef SH_DEBUG
		cout << "Follow dependencies (before removing loops):" << endl;
		// This is to test correctness of p_findFollowDependency
		for (map<pair<string, int>, set<pair<string, int> > >::const_iterator iter = p_followGraphAdjList.begin(),
				end = p_followGraphAdjList.end(); iter != end; ++iter)
		{
			cout << "(" << iter->first.first << ", " << iter->first.second << ") is dependent on:" << endl;
			for (set<pair<string, int> >::const_iterator iter2 = iter->second.begin(), end2 = iter->second.end(); iter2 != end2; ++iter2)
				cout << "\t(" << iter2->first << ", " << iter2->second << ")" << endl;
		}
#endif
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
#ifdef SH_DEBUG
		cout << "Follow parents:" << endl;
		// This is to test correctness of p_removeCycles
		for (map<pair<string, int>, pair<string, int> >::const_iterator iter = p_followParent.begin(), end = p_followParent.end();
				iter != end; ++iter)
			cout << "Parent of (" << iter->first.first << ", " << iter->first.second << ") is (" << iter->second.first << ", "
				<< iter->second.second << ")" << endl;
		cout << "Follow dependencies (after removing loops):" << endl;
		for (map<pair<string, int>, set<pair<string, int> > >::const_iterator iter = p_followGraphAdjList.begin(),
				end = p_followGraphAdjList.end(); iter != end; ++iter)
		{
			if (p_findParent(iter->first) != iter->first)
				continue;
			cout << "(" << p_findParent(iter->first).first << ", " << p_findParent(iter->first).second << ") is dependent on:" << endl;
			for (set<pair<string, int> >::const_iterator iter2 = iter->second.begin(), end2 = iter->second.end(); iter2 != end2; ++iter2)
				cout << "\t(" << p_findParent(*iter2).first << ", " << p_findParent(*iter2).second << ")" << endl;
		}
#endif
		p_updateParentAndAfter();
#ifdef SH_DEBUG
		cout << "Parent and after 2:" << endl;
		for (int k = 0; k <= p_k_in_grammar; ++k)
			for (int i = 0; i < p_nonterminalCount; ++i)
				for (set<p_parentAndAfterData>::const_iterator iter = p_parentAndAfter2[pair<string, int>(p_nonterminals[i], k)].begin(),
						end = p_parentAndAfter2[pair<string, int>(p_nonterminals[i], k)].end(); iter != end; ++iter)
				{
					cout << "(" << p_nonterminals[i] << ", " << k << ") is derived from " << iter->parent << " and after it is:";
					for (list<string>::const_iterator it = iter->after.begin(), end2 = iter->after.end(); it != end2; ++it)
						cout << " " << ((*it == "")?string("$"):*it);
					cout << endl;
				}
#endif
#ifdef SH_DEBUG
		cout << "Follow:" << endl;
		// This is to test correctness of p_findFollow
		for (int k = 1; k <= p_k_in_grammar; ++k)
			for (int i = 0; i < p_nonterminalCount; ++i)
				p_findFollow(p_nonterminals[i], k);
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
		bool ambiguous = false;
		p_LLK_table.clear();
		for (map<int, shRule>::const_iterator rule = p_ruleByCode.begin(), end = p_ruleByCode.end(); rule != end; ++rule)
		{
			for (int k = 0; k < p_k_in_grammar; ++k)
			{
				set<list<string> > firstPart = p_findLLKFirst(rule->second.body, k, true);
				pair<string, int> parent = p_findParent(pair<string, int>(rule->second.head, p_k_in_grammar-k));
				set<list<string> > secondPart = p_findFollow(parent.first, parent.second);
				for (set<list<string> >::const_iterator l1 = firstPart.begin(), end2 = firstPart.end(); l1 != end2; ++l1)
					for (set<list<string> >::const_iterator l2 = secondPart.begin(), end3 = secondPart.end(); l2 != end3; ++l2)
					{
						list<string> res = *l1;
						for (list<string>::const_iterator s = l2->begin(), end4 = l2->end(); s != end4; ++s)
							res.push_back(*s);
						p_LLK_table_lookup tableEntry(rule->second.head, res);
						if (p_LLK_table.find(tableEntry) != p_LLK_table.end() && p_LLK_table[tableEntry].rule != rule->first)
						{
							char temp[100];
							p_errorMessage += "Ambiguity in grammar: ";
							p_errorMessage += rule->second.head;
							p_errorMessage += " with";
							for (list<string>::const_iterator i = res.begin(), end4 = res.end(); i != end4; ++i)
								if (*i == "")
									p_errorMessage += " $";
								else
									p_errorMessage += " "+*i;
							sprintf(temp, " implies use of rule %d while it also implies rule %d\n",
									p_LLK_table[tableEntry].rule, rule->first);
							p_errorMessage += temp;
							PRINT_RULE(p_LLK_table[tableEntry].rule);
							PRINT_RULE(rule->first);
							if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST)
								p_errorMessage += "Ignoring the later rule in this case.\n";
							else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST)
							{
								p_errorMessage += "Ignoring the former rule in this case.\n";
								p_LLK_table[tableEntry] = p_LLK_table_data(rule->first);
							}
							else
								ambiguous = true;			// ambiguity!
						}
						else
							p_LLK_table[tableEntry] = p_LLK_table_data(rule->first);
					}
			}
			// Now for k == p_k_in_grammar
			set<list<string> > wholePart = p_findLLKFirst(rule->second.body, p_k_in_grammar, false);
			for (set<list<string> >::const_iterator l = wholePart.begin(), end2 = wholePart.end(); l != end2; ++l)
			{
				p_LLK_table_lookup tableEntry(rule->second.head, *l);
				if (p_LLK_table.find(tableEntry) != p_LLK_table.end() && p_LLK_table[tableEntry].rule != rule->first)
				{
					char temp[100];
					p_errorMessage += "Ambiguity in grammar: ";
					p_errorMessage += rule->second.head;
					p_errorMessage += " with";
					for (list<string>::const_iterator i = l->begin(), end3 = l->end(); i != end3; ++i)
						if (*i == "")
							p_errorMessage += " $";
						else
							p_errorMessage += " "+*i;
					sprintf(temp, " implies use of rule %d while it also implies rule %d\n",
							p_LLK_table[tableEntry].rule, rule->first);
					p_errorMessage += temp;
					PRINT_RULE(p_LLK_table[tableEntry].rule);
					PRINT_RULE(rule->first);
					if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST)
						p_errorMessage += "Ignoring the later rule in this case.\n";
					else if (p_ambiguityResolution == SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST)
					{
						p_errorMessage += "Ignoring the former rule in this case.\n";
						p_LLK_table[tableEntry] = p_LLK_table_data(rule->first);
					}
					else
						ambiguous = true;			// ambiguity!
				}
				else
					p_LLK_table[tableEntry] = p_LLK_table_data(rule->first);
			}
		}
		delete[] p_nullable;
		p_cleanupIntermediateData();
		if (ambiguous)
			return SH_P_AMBIGUITY;
#ifdef SH_DEBUG
		cout << "LLK table:" << endl;
		// This to test correctness of p_LLK_table
		for (map<p_LLK_table_lookup, p_LLK_table_data>::const_iterator iter = p_LLK_table.begin(), end = p_LLK_table.end(); iter != end; ++iter)
		{
			cout << iter->first.nonterminal << " with";
			for (list<string>::const_iterator i = iter->first.lookAhead.begin(), end2 = iter->first.lookAhead.end(); i != end2; ++i)
				if (*i == "")
					cout << " $";
				else
					cout << " " << *i;
			cout << " implies use of rule " << iter->second.rule << endl;
		}
#endif
		ofstream tout(FIX_PATH(tableFile).c_str());
		for (map<p_LLK_table_lookup, p_LLK_table_data>::const_iterator iter = p_LLK_table.begin(), end = p_LLK_table.end(); iter != end; ++iter)
		{
			tout << iter->first.nonterminal << " " << iter->second.rule;
			for (list<string>::const_iterator i = iter->first.lookAhead.begin(), end2 = iter->first.lookAhead.end(); i != end2; ++i)
				// If it is $, do not write it, it will be obvious because of the line having less tokens!
				if (*i != "")
					tout << " " << *i;
			tout << endl;
		}
	}
	p_LLK_tableBuilt = true;
	return SH_P_SUCCESS;
}

int shParser::shPParseLLK(shLexer &lexer)
{
	if (!lexer.p_initialized)
	{
		p_errorMessage = "Lexer not initialized\n";
		return SH_P_LEXER_NOT_INITIALIZED;
	}
	if (!p_LLK_tableBuilt)
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
			if (p_LLK_EPRHandleFunction[*iter2] == _noFunctionForLLKErrorProductionRule)
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
		p_LLK_parseStack.push(shLLKStackElement(""));
		token = lexer.shLPeekNextToken(type, i);
#ifdef SH_DEBUG
		cout << "read " << token << " from input which is of type: " << type << endl;
#endif
		lookAhead << type;
		lookAheadTokens << token;
	}
	p_LLK_parseStack.push(shLLKStackElement(p_nonterminals[0]));
	while (!p_LLK_parseStack.empty())
	{
		string current = p_LLK_parseStack.top().terminalOrNonterminal;
#ifdef SH_DEBUG
		cout << "Reading " << current << " from stack!" << endl;
#endif
		if (current == "" && lookAheadTokens.k_elements().front() != "")
		{
			char temp[1001];
			snprintf(temp, 1000, "%s:%d:%d: warning: the parser has successfully finished, but there are more lines in the input file\n",
					lexer.shLFileName().c_str(), lexer.shLLine(), lexer.shLColumn());
			p_errorMessage += temp;
		}
		p_LLK_parseStack.pop();
		if (current[0] == '@')
		{
			bool ignoredWhiteSpace = lexer.shLIgnoresWhiteSpace();
			(*p_actionRoutine[current])(lexer, *this);
			if (ignoredWhiteSpace != lexer.shLIgnoresWhiteSpace())		// Then the action routine has issued lexer.shLIgnoreWhiteSpace
											// and thus the look-aheads must be retaken
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
		else if (p_tokenTypes.find(current) != p_tokenTypes.end())
		{
			// Then a terminal must be matched
			if (lookAhead.k_elements().front() != current)
			{
				if (!(*p_LLK_pleh)(p_LLK_parseStack, lexer, p_errorMessage,
							current, lookAhead, lookAheadTokens))
					return SH_P_PARSE_ERROR;
				else
					continue;
			}
			lexer.shLNextToken(type);			// This should be the one just matched.
			token = lexer.shLPeekNextToken(type, p_k_in_grammar-1);
#ifdef SH_DEBUG
			cout << "read " << token << " from input which is of type: " << type << endl;
#endif
			lookAhead << type;
			lookAheadTokens << token;
		}
		else if (p_nonterminalCodes.find(current) != p_nonterminalCodes.end())
		{
			p_LLK_table_data ruleToUse;
			if (p_LLK_table.find(p_LLK_table_lookup(current, lookAhead.k_elements())) != p_LLK_table.end())
			{
				ruleToUse = p_LLK_table[p_LLK_table_lookup(current, lookAhead.k_elements())];
				if (ruleToUse.rule >= p_ruleCount)
				{
					p_errorMessage += "Bug in LLK table!\nCould not recover from last error!\n";
					return SH_P_PARSE_ERROR;
				}
				// Use p_ruleByCodeWithActionRoutine to include action routines!
				shRule rule = p_ruleByCodeWithActionRoutine[ruleToUse.rule];
				(*p_LLK_EPRHandleFunction[ruleToUse.rule])(p_LLK_parseStack, lexer, p_errorMessage, rule);
				for (list<string>::const_reverse_iterator iter = rule.body.rbegin(), end = rule.body.rend(); iter != end; ++iter)
					p_LLK_parseStack.push(shLLKStackElement(*iter));
			}
			else
			{
				if (!(*p_LLK_pleh)(p_LLK_parseStack, lexer, p_errorMessage,
							current, lookAhead, lookAheadTokens))
					return SH_P_PARSE_ERROR;
				else
					continue;
			}
		}
		else
		{
			if (current != "")
			{
				p_errorMessage += "Error in parse stack: found \""+current+"\" on parse stack which is invalid!\n"
					"Could not recover from last error!\n";
				return SH_P_PARSE_ERROR;
			}
		}
	}
	return SH_P_SUCCESS;
}

void shParser::shPAddLLK_EPRHandler(const string &EPRHandler, shPLLK_EPRHandler h)
{
	string handler;
	if (EPRHandler[0] != '!')
		handler = '!'+EPRHandler;
	else
		handler = EPRHandler;
	for (set<int>::const_iterator i = p_ruleEPRHandler[handler].begin(), end = p_ruleEPRHandler[handler].end(); i != end; ++i)
		p_LLK_EPRHandleFunction[*i] = h;
}

void shParser::shPSetLLK_PLErrorHandler(shPLLK_PLErrorHandler h)
{
	p_LLK_pleh = h;
}
