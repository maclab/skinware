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

#ifndef PARSER_H_BY_CYCLOPS
#define PARSER_H_BY_CYCLOPS

#include <map>
#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <stack>
#include <klist.h>
#include <lexer.h>

using namespace std;

#define SH_P_SUCCESS 0
#define SH_P_FILE_NOT_FOUND -1
#define SH_P_NOT_ENOUGH_MEMORY -2
#define SH_P_LEXER_NOT_INITIALIZED -3
#define SH_P_GRAMMAR_ERROR -4
#define SH_P_AMBIGUITY -5
#define SH_P_LEFT_RECURSION -6				// Only with LLK
#define SH_P_PARSE_ERROR -7
#define SH_P_RULES_NOT_LOADED -8
#define SH_P_NO_SUCH_RULE -9

#define SH_P_AMBIGUITY_AS_ERROR 0
#define SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST 1
#define SH_P_AMBIGUITY_RESOLVE_ACCEPT_LAST 2
#define SH_P_AMBIGUITY_RESOLVE_ACCEPT_SHIFT 3		// Only with LRK
#define SH_P_AMBIGUITY_RESOLVE_ACCEPT_REDUCE 4		// Only with LRK

class shParser
{
public:
	// Common or similar
	struct shLLKStackElement
	{
		string terminalOrNonterminal;									// Either a terminal or a nonterminal that needs to be matched
		shLLKStackElement() {}
		shLLKStackElement(const string &t) { terminalOrNonterminal = t; }
		shLLKStackElement(const char *t) { terminalOrNonterminal = t; }
	};
	struct shLRKStackElement
	{
		string terminalOrNonterminal;									// Either a terminal or a nonterminal that needs to be matched
		int state;											// The current state in p_LRK_diagram
		shLRKStackElement() {}
		shLRKStackElement(const string &t, int s) { terminalOrNonterminal = t; state = s; }
		shLRKStackElement(const char *t, int s) { terminalOrNonterminal = t; state = s; }
	};
	struct shRule
	{
		string head;											// The nonterminal (T) on the left of T -> ...
		list<string> body;										// The sequence of terminals, nonterminals and possibly action routines on the right of T -> ...
														// If body is empty, it shows T -> lambda
		shRule() {}
		shRule(const string &h, const list<string> &b) { head = h; body = b; }
	};

	typedef void (*shPActionRoutineFunction)(shLexer &, shParser &);					// Action routines take as argument references to the lexer and this parser. The lexer can then be
														// queried for current token/type. Other uses for lexer includes getting the line number, or setting
														// whether WHITE_SPACEs should be ignored or not. For parser, no use yet, but later
														// there may be options tunable during parse (so, forward compatibility). Or even now, to get the
														// error string and append a message to it.
														// NOTE: The action routine can safely call shLIgnoreWhiteSpace, the parser look-ahead will be rebuilt
														// if necessary. I didn't write this check/rebuild for error-production rules and phrase-level error handler
														// though, because I am not convinced that it is needed. With the phrase-level error handler, this can be
														// handled by the handler itself if needed. With error-production rules, well, they are there to allow accepting
														// of some **parse** errors, and not change the behaviour of lexer (which usually is due to semantical reasons)
	typedef void (*shPLLK_EPRHandler)(stack<shLLKStackElement> &, shLexer &, string &, shRule &);		// Error production rules' handlers take as argument the parse stack, the lexer,
	typedef void (*shPLRK_EPRHandler)(stack<shLRKStackElement> &, shLexer &, string &, shRule &);		// p_errorMessage and the rule itself.
														// The error production rules are there for parser not to fail, yet error to be generated and
														// handled. That is why you cannot return fail or success. You could (and most of the times must)
														// set a flag for your own semantics analysis to make sure you don't compile the erroneous input
	typedef bool (*shPLLK_PLErrorHandler)(stack<shLLKStackElement> &, shLexer &, string &, string &,	// Phrase level error handler takes as argument the parse stack, the lexer,
			shKLimitedList<string> &, shKLimitedList<string> &);					// p_errorMessage, (stack top for LLK parse), lookAhead and lookAheadTokens
	typedef bool (*shPLRK_PLErrorHandler)(stack<shLRKStackElement> &, shLexer &, string &,			// returns false if error was unrecoverable
			shKLimitedList<string> &, shKLimitedList<string> &);					// See shKLimitedList (klist.h) for details of usage of lookAhead. In short, call k_elements to
														// get the elements of the lookAhead. If you need the types of the matched tokens, look at the
														// k_elements of lookAheadTokens with matching indices. Try shPGeneratePLErrorHandler to get an already
														// generated phrase level error handler and work on each case individually
protected:
	set<string> p_tokenTypes;										// Types of accepted tokens
	vector<string> p_nonterminals;										// List of nonterminals in the grammar
	map<string, int> p_nonterminalCodes;									// A map from nonterminal name to index in p_nonterminals
	map<int, shRule> p_ruleByCode;										// A map from rule code to the rule itself
	map<int, set<int> > p_rulesByNonterminalCode;								// A map from nonterminal code to codes of the rules it expands to
	map<string, shPActionRoutineFunction> p_actionRoutine;							// A map which maps action routine names to functions. By default, it maps each name to an empty function

	stack<shLLKStackElement> p_LLK_parseStack;								// Parse stack for an LL(k) grammar
	stack<shLRKStackElement> p_LRK_parseStack;								// Parse stack for a CLR(k) grammar
	map<string, set<int> > p_ruleEPRHandler;								// maps an error-production-rule handler name to a set of rules it handles
	map<int, shPLLK_EPRHandler> p_LLK_EPRHandleFunction;							// maps a rule to an error production rule handler
	map<int, shPLRK_EPRHandler> p_LRK_EPRHandleFunction;							// if the rule was not an EPR, an empty function will be called
	shPLLK_PLErrorHandler p_LLK_pleh;									// Phrase level error handler for an LL(k) grammar
	shPLRK_PLErrorHandler p_LRK_pleh;									// Phrase level error handler for an LR(k) grammar
	int p_ambiguityResolution;										// The method of ambiguity resolution. One of SH_P_AMBIGUITY_*

	string p_errorMessage;											// In case of errors, p_errorMessage will be set to the message

	int p_ruleCount;											// Cache of p_ruleByCode.size() and p_ruleByCodeWithActionRoutine.size()
	int p_nonterminalCount;											// Cache of p_nonterminals.size() and p_nonterminalCodes.size()
	int p_k_in_grammar;											// K of the grammer, for example 1 if LL(1) or 3 if LR(3)
	// LLK specific
	struct p_LLK_table_data
	{
		int rule;											// The rule which would be used to expand a nonterminal based on the give lookAhead
		p_LLK_table_data(){}
		p_LLK_table_data(int r) { rule = r; }
	};
	struct p_LLK_table_lookup
	{
		string nonterminal;										// The nonterminal being expanded
		list<string> lookAhead;										// The k look-ahead tokens determining by which rule the nonterminal would be expanded
		p_LLK_table_lookup() {}
		p_LLK_table_lookup(const string &nont, const list<string> &l) { nonterminal = nont; lookAhead = l; }
		bool operator <(const p_LLK_table_lookup &RHS) const { if (nonterminal < RHS.nonterminal) return true; if (RHS.nonterminal < nonterminal) return false; return lookAhead < RHS.lookAhead; }
	};

	map<int, shRule> p_ruleByCodeWithActionRoutine;								// TODO: Try making LR(k) work with many action routines in the grammar. If so, then remove p_ruleActionRoutine
														// and use this for both
	map<p_LLK_table_lookup, p_LLK_table_data> p_LLK_table;							// An array showing which nonterminal with which k strings (look-ahead) must use which rule!
	// CLRK specific (CLR is Canonical LR)
	enum p_LRK_transition
	{
		SHIFT = 0,
		REDUCE,
		ACCEPT
	};
	struct p_LRK_diagram_data
	{
		p_LRK_transition transition;									// The transition to be done (SHIFT, REDUCE or ACCEPT
		int toStateOrRule;										// If SHIFTing, to which state the diagram would move to and if REDUCEing, Which rule to use for reduction
		p_LRK_diagram_data() {}
		p_LRK_diagram_data(p_LRK_transition t, int sr) { transition = t; toStateOrRule = sr; }
		bool operator !=(const p_LRK_diagram_data &RHS) const { return transition != RHS.transition || toStateOrRule != RHS.toStateOrRule; }
	};
	struct p_LRK_diagram_lookup
	{
		int state;											// Current state in the diagram
		list<string> lookAhead;										// The k look-ahead tokens, or 1 nonterminal determining the state transition/action
														// Note that the lists' size is either k or 1
		p_LRK_diagram_lookup() {}
		p_LRK_diagram_lookup(int s, const list<string> &l) { state = s; lookAhead = l; }
		bool operator <(const p_LRK_diagram_lookup &RHS) const { if (state < RHS.state) return true; if (RHS.state < state) return false; return lookAhead < RHS.lookAhead; }
	};

	map<int, string> p_ruleActionRoutine;									// maps a rule code to the action routine corresponding to it
														// as rules in LR(k) grammars can have at most one action routine (which is in the right end)
	map<p_LRK_diagram_lookup, p_LRK_diagram_data> p_LRK_diagram;						// A diagram showing which state with which k strings (look-ahead) should take which transition/action
public:
	// first, gets the possible tokens from lexer. note that the lexer needs to be already initialized
	// then reads the LL(k) grammar and stores it. calls p_shPPrepareLLKParse if no error. If new grammar then rebuilds table
	// else reads the table from file: grammarFile+".table"
	// return value:
	// SH_P_SUCCESS
	// SH_P_FILE_NOT_FOUND
	// SH_P_LEXER_NOT_INITIALIZED
	// SH_P_GRAMMAR_ERROR
	// SH_P_AMBIGUITY
	// SH_P_LEFT_RECURSION
	// -- The first nonterminal seen will be considered the start nonterminal
	int shPReadLLKGrammar(const shLexer &lexer, const char *grammarFile, int k, bool newGrammar = false);

	// first, gets the possible tokens from lexer. note that the lexer needs to be already initialized
	// reads the LR(K) grammar and stores it. calls shPPrepareLRKParse if no error. If new grammar then rebuilds table
	// else reads the table from file: grammarFile+".table"
	// adds a rule ""->S as the last rule to help CLR(K) parse but doesn't count it. It's code is p_ruleCount
	// return value:
	// SH_P_SUCCESS
	// SH_P_FILE_NOT_FOUND
	// SH_P_LEXER_NOT_INITIALIZED
	// SH_P_GRAMMAR_ERROR
	// SH_P_AMBIGUITY
	// -- The first nonterminal seen will be considered the start nonterminal
	int shPReadLRKGrammar(const shLexer &lexer, const char *grammarFile, int k, bool newGrammar = false);

	// creates two files: pleh_file+".h" and pleh_file+".cpp" which include an automatically generated phrase level error handler for the loaded grammar.
	// the .cpp file can then be used as a base for the error handler and edited for choice of actions and generation of meaningful errors
	// return values:
	// SH_P_SUCCESS
	// SH_P_FILE_NOT_FOUND
	// SH_P_RULES_NOT_LOADED
	int shPGeneratePLErrorHandler(const char *plehFile);

	// parses the input stored in lexer
	// return value:
	// SH_P_SUCCESS
	// SH_P_FILE_NOT_FOUND
	// SH_P_LEXER_NOT_INITIALIZED
	// SH_P_PARSE_ERROR
	// SH_P_RULES_NOT_LOADED
	int shPParseLLK(shLexer &lexer);

	// parses the input stored in lexer
	// return value:
	// SH_P_SUCCESS
	// SH_P_FILE_NOT_FOUND
	// SH_P_LEXER_NOT_INITIALIZED
	// SH_P_PARSE_ERROR
	// SH_P_RULES_NOT_LOADED
	int shPParseLRK(shLexer &lexer);

	void shPAddActionRoutine(const string &actionRoutineName, shPActionRoutineFunction func);

	// EPR stands for error production rule
	void shPAddLLK_EPRHandler(const string &EPRHandler, shPLLK_EPRHandler h);
	void shPAddLRK_EPRHandler(const string &EPRHandler, shPLRK_EPRHandler h);

	// PL stands for phrase level
	void shPSetLLK_PLErrorHandler(shPLLK_PLErrorHandler h);
	void shPSetLRK_PLErrorHandler(shPLRK_PLErrorHandler h);

	// Set behavior when ambiguity is encountered
	// Could receive SH_P_AMBIGUITY_*
	// If received LRK ambiguity resolutions while working with LLK grammar,
	// SH_P_AMBIGUITY_AS_ERROR is assumed.
	void shPAmbiguityResolution(int r);

	string shPError();

	void shPAppendError(const string &error);
	void shPAppendError(const char *error);
	void shPAppendError(const unsigned char *error);

	// Resets the parser without removing parse data. That is after a call to this function, the
	// parser can be used to parse another program. Upon a call to shPParse*, this is automatically
	// called.
	void shPReset();

	shParser()
	{
		p_LLK_tableBuilt = false;
		p_LRK_diagramBuilt = false;
		p_ambiguityResolution = SH_P_AMBIGUITY_AS_ERROR;
	}
private:
	struct p_findFirstData
	{
		list<string> sentence;										// Finds the first k elements of a sentence. For example, if sentence is a nonterminal T,
		int k;												// the firsts of T (with length k) are found. If a sentence t T R x is given, where T and R are
		bool shallEnd;											// nonterminals while t and x are terminals,
		p_findFirstData(){}										//  t + possible productions of T with length n + possible productions of R with length k-n-2 + x
		p_findFirstData(const list<string> &s, int kk, bool e) { sentence = s; k = kk; shallEnd = e; }	// are found. If shallEnd is false, then if sentence generated k terminals followed by NOT nullable nonterminals,
														// it would still select the first k terminals as the first of the sentence. If it is true, then the then
														// k terminals could be followed only by nullable nonterminals. In fact, shallEnd = true is used to generate
														// productions of a sentence with length k as opposed to its firsts.
		bool operator <(const p_findFirstData &RHS) const { if (k < RHS.k) return true; if (RHS.k < k) return false;
			if (!shallEnd && RHS.shallEnd) return true; if (shallEnd && !RHS.shallEnd) return false; return sentence < RHS.sentence; }
	};
	struct p_parentAndAfterData
	{
		string parent;											// parent is the root of the disjoint set this nonterminal belongs to. Whatever comes after the parent,
														// can come after the nonterminal, too because of a rule such as Parnet -> ... Nonterminal
		list<string> after;										// a list of terminals that come after this nonterminal. Note that, always a set<p_parentAndAfterData> is
														// used where the parent could be repeated for as many afters are found for the nonterminal.
		p_parentAndAfterData() {}
		p_parentAndAfterData(const string &p, const list<string> &a) { parent = p; after = a; }
		bool operator <(const p_parentAndAfterData &RHS) const { if (parent < RHS.parent) return true; if (RHS.parent < parent) return false; return after < RHS.after; }
	};
	struct p_ruleWithDot
	{
		int rule;
		int dotPosition;
		p_ruleWithDot() {}
		p_ruleWithDot(int r, int d) { rule = r; dotPosition = d; }
		bool operator <(const p_ruleWithDot &RHS) const { if (rule < RHS.rule) return true; if (RHS.rule < rule) return false; return dotPosition < RHS.dotPosition; }
	};

	typedef map<p_ruleWithDot, set<list<string> > > p_LRK_state;						// A set of (rule with dot, follow) for example (T->x.Ey, xyz) if CLR(3). The follows of a p_ruleWithDot are grouped
														// together as a set of follows. This, although defined as map, is actually a set.
														// I defined it as a map so that from p_ruleWithDot I can get the follows
														// rule: "" -> .S (that is (p_ruleCount, 0)) is added initialy to start making the diagram and also making state 0 unique
														// the state with rule "" -> S. (that is (p_ruleCount, 1)) is the accept state
														// Note: These follows are the follows of the state and not the follows of nonterminals

	bool *p_nullable;											// Whether a nonterminal is nullable or not. Used for finding firsts
	bool *p_visited;											// For the DFS that finds nullability
	map<p_findFirstData, set<list<string> > > p_first;							// maps a sentence to its possible firsts (k tokens in the beginning of whatever can be produced by the sentence)

	map<string, set<p_parentAndAfterData> > p_parentAndAfter;						// A map which maps a nonterminal to a list. Each element of this list shows that
														// this nonterminal can be derived from which nonterminal and what sentence would come after it
	map<pair<string, int>, set<p_parentAndAfterData> > p_parentAndAfter2;					// When follow dependencies are found and cycles removed, some of the (nonterminal, number_of_tokens_to_follow)
														// may be grouped together. Note that one nonterminal could belong to two different groups based on the number of
														// tokens expected to come after it (TODO: That's what I thought then. But the more I think about it, the more I am
														// convinced that the groups would look the same no matter how many tokens are expected afterwards. If that is the
														// case, then this can change to map<string, ...> like p_parentAndAfter). This array tells the group this
														// nonterminal belongs to based on the number of expected follows, as well as its possible "after"s. These
														// "after"s are the same "after"s of p_parentAndAfter except the parent has accumulated "after"s of its group.
														// The parent is in fact what is queried for "after"s and never the non-parents.
	map<pair<string, int>, set<list<string> > > p_follow;							// Get a set of follows of length k of a nonterminal
	map<pair<string, int>, set<pair<string, int> > > p_followGraphAdjList;					// An adj. list of follow dependencies: "nonterminal_a_wanting_i_tokens" is dependent on
														// "nonterminal_b_wanting_j_tokens"s
	map<pair<string, int>, pair<string, int> > p_followParent;						// Used for disjoint set of (nonterminal,k) which gives the set which a nonterminal with expected follow
														// of length k belongs to
	map<p_findFirstData, bool> p_firstComplete;								// Only used by LR(k)
	map<p_findFirstData, bool> p_firstDFSVisited;								// Visited flag for DFS on p_first, only used by LR(k)
	map<pair<string, int>, bool> p_followDFSVisited;							// Visited flag for DFS on p_followGraphAdjList

	bool p_moreFirstFound;											// For LR(k) only. Since finding first in LR(k) can be partial, it may need multiple passes
	bool p_firstCompleteFromNowOn;										// For LR(k) only. Since finding first in LR(k) can be partial, this is set to mark the end of multiple passes
	bool p_followFoundLoop;											// p_removeCycles function found a loop
	bool p_LLK_tableBuilt;											// If false, LLK table is not yet built
	bool p_LRK_diagramBuilt;										// If false, LRK diagram is not yet built


	set<list<string> > p_findLLKFirst(list<string> sen, int k, bool shallEnd = false);			// These function, recurse on the LL(k) or LR(k) grammar and find out different combinations reachable by sentence
	set<list<string> > p_findLRKFirst(list<string> sen, int k, bool shallEnd = false);			// with length k. If shallEnd, then find complete productions as opposed to firsts. See comments of p_findFirstData
														// It uses a map to memoize the recursion (p_first)
														// The grammar must be ensured not to be left recursive first if finding LL(k)
														// Computes partial first for left-recursive grammars if findin LRK
	void p_findFollowDependency(string nonterminal, int s);							// If there is a rule such as T -> .. R, then follows of R with length s are dependent on follows of T with length s
														// If the rule was T -> .. R sentence, then follows of R with length s are dependent on follows of T with length k if
														// sentence can produce list of terminals with length s-k
	pair<string ,int> p_findParent(pair<string, int> node);							// Helper function for disjoint set of (nonterminal,k) which gives the set which a nonterminal with expected follow
														// of length k belongs to
	pair<bool, pair<string, int> > p_removeCycles(pair<string, int> node);					// Make the p_followGraphAdjList a DAG
	void p_updateParentAndAfter();										// Accumulate "after" data from p_parentAndAfter in p_parentAndAfter2 once the follow adjacency graph is dagified
														// The dagifying process groups some nonterminals together and therefore the "after" of each of them combined,
														// will be present in the "after" of their parent in p_parentAndAfter2
	set<list<string> > p_findFollow(string nonterminal, int s);						// This function, recurses on the LL(k) or LR(k) grammar and finds out different follows of a nonterminal with length s
														// It uses a map to memorize the recursion (p_follow), it also uses p_findLLKFirst function
														// The grammar must be ensured not to be left recursive first
														// Follow dependencies must be found before and its graph must be DAGgified first if not.
	int p_checkNullable(int nonterminal);									// returns nonterminal code on left recursion (only for LL(k) grammars) or -1 if not left recursive
	void p_closure(p_LRK_state &state);									// find the closure of a CLR(k) state
	int p_shPPrepareLLKParse(const char *tableFile, bool load = false);					// builds the LL(k) table if load was false and stores it in tableFile. If load was true, loads table from tableFile
														// return value:
														// SH_P_SUCCESS
														// SH_P_AMBIGUITY
														// SH_P_LEFT_RECURSION
	int p_shPPrepareLRKParse(const char *tableFile, bool load = false);					// builds the CLR(k) diagram if load was false and stores it in tableFile. If load was true, loads table from tableFile
														// return value:
														// SH_P_SUCCESS
														// SH_P_AMBIGUITY
	void p_cleanupIntermediateData();									// clear intermediate variables, such as p_first, p_follow etc

	void shPInitToTokensFile();
	void shPInitToGrammarFile();
	friend class shLexer;
};

#endif
