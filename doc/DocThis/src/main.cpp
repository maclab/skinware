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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <list>
#include <dirent.h>
#ifdef _WIN32
  #include <windows.h>
  #include <limits.h>
  #include <direct.h>
  #define makeDir(dir) _mkdir(dir)
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
  #define makeDir(dir) mkdir(dir, 0777)		// permission would be the maximum given to running process
#endif
#include <shcompiler.h>
#include "semantics.h"
#include "specials.h"
#include "error_print.h"
#include "originals.h"

#define DOC_THIS_STRINGIFY_(x) #x
#define DOC_THIS_STRINGIFY(x) DOC_THIS_STRINGIFY_(x)

#define DOC_THIS_VERSION_MAJOR		0
#define DOC_THIS_VERSION_MINOR		3
#define DOC_THIS_VERSION_REVISION	0
#define DOC_THIS_VERSION_BUILD		638
#define DOC_THIS_VERSION		DOC_THIS_STRINGIFY(DOC_THIS_VERSION_MAJOR)"."\
					DOC_THIS_STRINGIFY(DOC_THIS_VERSION_MINOR)"."\
					DOC_THIS_STRINGIFY(DOC_THIS_VERSION_REVISION)"."\
					DOC_THIS_STRINGIFY(DOC_THIS_VERSION_BUILD)

int copyFile(const char *in, const char *out)
{
	FILE *fin = fopen(in, "r");
	if (!fin)
		return -1;
	FILE *fout = fopen(out, "w");
	if (!fout)
	{
		fclose(fin);
		return -1;
	}
	char *buf = new char[BUFSIZ];
	if (!buf)
	{
		fclose(fin);
		fclose(fout);
		return -1;
	}
	size_t n;
	int ret = 0;
	while ((n = fread(buf, sizeof(char), BUFSIZ, fin)) > 0)
		if (fwrite(buf, sizeof(char), n, fout) != n)
		{
			ret = -1;
			break;
		}
	fclose(fin);
	fclose(fout);
	delete[] buf;
	return ret;
}

static shLexer lexer;
static shParser parser;

static char homePath[PATH_MAX+1];

static bool scanningForGlobals = false;

//////////////////// Lexer callback function
// If no token can be matched, handles the case
////////////////////////////////////////////
static void noTokenMatchFunction(char *fc, int *lb, int *f, const char *file, int *l, int *c)
{
	if (docType() != INVALID && !scanningForGlobals)
	{
		int n, n2;
		char msg[100];
		sprintf(msg, "no token can be matched, probably because of an invalid character (is it '%n", &n);
		if (fc[*f] == '\n')
			sprintf(msg+n, "\\n%n", &n2);
		else if (fc[*f] == '\r')
			sprintf(msg+n, "\\r%n", &n2);
		else if (fc[*f] == '\t')
			sprintf(msg+n, "\\t%n", &n2);
		else if (fc[*f] >= ' ' && fc[*f] < 127)
			sprintf(msg+n, "%c%n", fc[*f], &n2);
		else
			sprintf(msg+n, "\\%d%n", (int)fc[*f], &n2);
		sprintf(msg+n+n2, "'?)");
		printMessage(*l, *c, "%s", msg);
	}
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
	stopEvaluation();
}

//////////////////// Parser callback function
// Phrase level error handler
/////////////////////////////////////////////
static bool phraseLevelErrorHandler(stack<shParser::shLLKStackElement> &s, shLexer &l, string &em, string &cur,
		shKLimitedList<string> &la, shKLimitedList<string> &lat)
{
	if (docType() == INVALID)
		return false;
	string lastr;
	for (list<string>::iterator iter = la.k_elements().begin(), end = la.k_elements().end(); iter != end; ++iter)
		lastr += " " + ((*iter == "")?string("$"):*iter);
	char temp[1001];
	sprintMessage(temp, 1000, l.shLLine(), l.shLColumn(), "error: Expected to match %s but found:%s", cur.c_str(), lastr.c_str());
	em += temp;
	sprintMessage(temp, 1000, "Could not recover from last error");
	em += temp;
	stopEvaluation();
	return false;
}

string removeSpace(const string &s);

struct filenameWithType		// temporary struct for sorting files according to their types when writing tex main files
{
	string filename;
	docTypeEnum type;
	bool operator <(const filenameWithType &rhs)
	{
		if (type < rhs.type)
			return true;
		if (rhs.type < type)
			return false;
		if (filename < rhs.filename)
			return true;
		if (rhs.filename < filename)
			return false;
		return false;
	}
};

int main(int argc, char **argv)
{
	bool tokensRebuild = false;
	bool parseTableRebuild = false;
	bool buildHtml = false;
	bool buildTex = false;
	bool buildTexMain = false;
	bool buildTexMainFromDir = false;
	bool rewriteCss = false;
	bool verbose = false;
	bool copyCss = false;
	bool homeSpecified = false;
	bool outputVersion = false;
	bool buildSummary = true;
	bool buildDetails = true;
	bool compactTexMain = false;
	bool verbosePdfLatex = false;
	bool ignoreOptions = false;
	string home;
	list<string> files;
	string texMainSourceDir;
	string noticeFile;
	// Read arguments
	for (int i = 1; i < argc; ++i)
	{
		if (ignoreOptions)
			files.push_back(string(argv[i]));
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			cout << "Usage: " << argv[0] << " [options] <files>" << endl << endl;
			cout << "Options are:" << endl << endl;
			cout << "--help or -h\n  This message will be printed" << endl << endl;
			cout << "---rebuild-all\n  All dfas are rebuilt as well as the parse table" << endl <<
				"---rebuild-parse\n  Only the parse table is rebuilt" << endl << endl;
			cout << "--version\n  Output current version of DocThis!" << endl <<
				"--home directory\n  Identify home directory of DocThis!" << endl <<
				"--verbose\n  Make DocThis! more verbose" << endl <<
				"--css-default\n  Backup current style.css file and overwrite style.css with the default styles" << endl <<
				"--\n  Treat the rest of the arguments as files even if they look like options" << endl << endl;
			cout << "-no-summary\n  Don't generate summary of identifiers being documented" << endl <<
				"-only-summary\n  Only generate summary, and ignore the detailed explanation of identifiers" << endl <<
				"-texmain-compact\n  Make the TeX documentation more compact" << endl <<
				"-verbose-pdflatex\n  Show output of pdflatex. Use during testing, remove in release." << endl << endl;
			cout << "+html\n  Generate output in HTML format" << endl <<
				"+tex\n  Generate output in TeX format" << endl <<
				"+texmain\n  Create a main TeX file that includes TeX files created from input files" << endl <<
				"+texmain-all directory\n  Create a TeX file that includes all TeX files that can be generated from" << endl <<
				"  documentation source files in `directory`" << endl <<
				"+notices file\n  Add a list of notice definitions to replace the keywords after NOTICE" << endl <<
				"+css\n  Copy the style.css file to `generated/html` folder" << endl << endl;
			cout << "The `---*` options are used under abnormal conditions, such as regenerating" << endl <<
				"lexer and parser tables which are generally used only in the development" << endl <<
				"process of DocThis!. If you have found a lexical or parsing bug, true that" << endl <<
				"you could try to fix it by manipulating the files and rebuilding the cache," << endl <<
				"but, it is recommended to report the bug so it could be universally fixed." << endl << endl;
			cout << "The `--*` options are miscellaneous options that don't affect the generated" << endl <<
				"output, at least not directly. `--home` specifies home directory of DocThis!" << endl <<
				"which is used in case the `DOC_THIS_HOME` environment variable is not defined" << endl <<
				"and the DocThis! executable is moved from its original home. `--version`" << endl <<
				"outputs the program version. `--css-default` overwrites the CSS files in" << endl <<
				"DocThis! home with the default ones. Use this only if you have changed the" << endl <<
				"original CSS files and you need to recover them." << endl << endl;
			cout << "The `-*` options are those that manipulate the output of DocThis! By default," << endl <<
				"DocThis! generates both summaries and detailed explanations of identifiers" << endl <<
				"being documented. With `-no-summary`, summaries are excluded and with" << endl <<
				"`-only-summary`, the detailed explanations are excluded. By default, DocThis!" << endl <<
				"creates a main TeX file including the generated files in a loose fashion. To" << endl <<
				"get a more compact TeX document, `-texmain-compact` can be used. To avoid" << endl <<
				"cluttering the output, the output of `pdflatex` is redirected to"
				" to "
#ifdef _WIN32
				"NUL." << endl <<
#else
				"/dev/null." << endl <<
#endif
				"During implementation of the documentation however, you most likely want to see" << endl <<
				"this output for errors. Use `-verbose-pdflatex` to enable output of `pdflatex`." << endl << endl;
			cout << "The `+*` options control what DocThis! will generate. `+html` and `+tex`" << endl <<
				"options make DocThis! generate html and TeX outputs from every input file." << endl <<
				"Other options are more specialized. `+texmain` creates a TeX file that includes" << endl <<
				"results of input files and `+texmain-all directory` creates a TeX file that" << endl <<
				"includes possible TeX files generated from all source files in directory." << endl <<
				"`+texmain` places the files in their output in the order they appear in the" << endl <<
				"input arguments. `+texmain-all` on the other hand, places index first (if any)," << endl <<
				"then places with alphabetic order: classes, structures, functions, variables," << endl <<
				"constants, I/O files, examples and finally others. `+texmain` and" << endl <<
				"`+texmain-all` also create a Makefile for your convenience." << endl <<
				"`+notices` introduces a file from which NOTICE defintions are taken." << endl << endl;
			cout << "The program creates folders: `generated`, `generated/html` and `generated/tex` for" << endl <<
				"output and places output files in the corresponding folder." << endl << endl;
			cout << "The DocThis! program automatically recovers its home directory. It assumes" << endl <<
				"however, that the \"grammar\" directory of DocThis! as well as its .css files" << endl <<
				"are located next to its executable. If that is not the case, you can set the" << endl <<
				"environment variable `DOC_THIS_HOME` to point to the directory containing the" << endl <<
				"\"grammar\" directory. Alternatively, using `--home directory` option can" << endl <<
				"override this." << endl << endl;
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[i], "--") == 0)
			ignoreOptions = true;
		else if (strcmp(argv[i], "---rebuild-all") == 0)
		{
			tokensRebuild = true;
			parseTableRebuild = true;
		}
		else if (strcmp(argv[i], "---rebuild-parse") == 0)
			parseTableRebuild = true;
		else if (strcmp(argv[i], "--home") == 0)
		{
			++i;
			if (i < argc)
			{
				home = argv[i];
				homeSpecified = true;
			}
			else
				cout << "--home must be followed by a directory. Use --help option to see usage." << endl <<
					"Ignoring option" << endl;
		}
		else if (strcmp(argv[i], "--css-default") == 0)
			rewriteCss = true;
		else if (strcmp(argv[i], "--version") == 0)
			outputVersion = true;
		else if (strcmp(argv[i], "--verbose") == 0)
			verbose = true;
		else if (strcmp(argv[i], "-no-summary") == 0)
		{
			buildDetails = true;
			buildSummary = false;
		}
		else if (strcmp(argv[i], "-only-summary") == 0)
		{
			buildSummary = true;
			buildDetails = false;
		}
		else if (strcmp(argv[i], "-texmain-compact") == 0)
			compactTexMain = true;
		else if (strcmp(argv[i], "-verbose-pdflatex") == 0)
			verbosePdfLatex = true;
		else if (strcmp(argv[i], "+html") == 0)
			buildHtml = true;
		else if (strcmp(argv[i], "+tex") == 0)
			buildTex = true;
		else if (strcmp(argv[i], "+texmain") == 0)
		{
			buildTexMain = true;
			buildTexMainFromDir = false;
		}
		else if (strcmp(argv[i], "+texmain-all") == 0)
		{
			buildTexMainFromDir = true;
			buildTexMain = false;
			++i;
			if (i < argc)
				texMainSourceDir = argv[i];
			else
			{
				cout << "+texmain-all must be followed by a directory. Use --help option to see usage." << endl <<
					"Assuming +texmain" << endl;
				buildTexMainFromDir = false;
				buildTexMain = true;
			}
		}
		else if (strcmp(argv[i], "+notices") == 0)
		{
			++i;
			if (i < argc)
				noticeFile = argv[i];
			else
				cout << "+notices must be followed by a file. Use --help option to see usage." << endl;
		}
		else if (strcmp(argv[i], "+css") == 0)
			copyCss = true;
		else
			files.push_back(string(argv[i]));
	}
	// Finished reading arguments
	// Step 1. Get home!
	if (!homeSpecified)
	{
		char *home_p = getenv("DOC_THIS_HOME");
		if (!home_p)
		{
#ifdef _WIN32
			int bytes = GetModuleFileName(NULL, homePath, sizeof(homePath)-1);
#else
			char exelink[50];
			sprintf(exelink, "/proc/%d/exe", getpid());
			int bytes = readlink(exelink, homePath, sizeof(homePath)-1);
#endif
			if(bytes <= 0)
			{
				cout << "Failed to retrieve home directory of DocThis!. Use --help option to see alternatives." << endl;
				return EXIT_FAILURE;
			}
			for (--bytes; bytes >= 0; --bytes)
				if (homePath[bytes] == '/' || homePath[bytes] == '\\')
				{
					homePath[bytes] = '\0';
					break;
				}
#ifdef _WIN32
				else if (homePath[bytes] == ':')	// In windows, file name may also be something like: C:file.exe
				{
					homePath[bytes+1] = '\0';
					break;
				}
#endif
			if (bytes < 0)		// could not find /, which means the file name didn't contain any directory!
				home = ".";	// assume . (it shouldn't happen though!)
			else
				home = homePath;
		}
		else
		{
			home = home_p;
			if (home.length() == 0)
			{
				cout << "Environment variable DOC_THIS_HOME contains no data. Use --help option to see usage." << endl;
				return EXIT_FAILURE;
			}
		}
	}
	int home_length = home.length();
	if (home[home_length-1] != '/' && home[home_length-1] != '\\')
		home += '/';
	// Step 2. Stuff that can be done without anything known except home. They don't generate output either
	if (rewriteCss)
	{
		ofstream fout((home+"style.css").c_str());
		if (!fout.is_open())
		{
			cout << "Failed to open style.css file for resetting . Do you have permission for writing in the home directory of DocThis!?" << endl <<
				"Or did you move the DocThis! executable from its home? (detected home is: " << home << ")" << endl <<
				"Use --help option to see how to fix this." << endl;
		}
		else
		{
			fout << getCss();
			fout.close();
		}
	}
	if (outputVersion)
		cout << "DocThis! "DOC_THIS_VERSION << endl;
	// Step 3. Initializing, and possibly rebuilding, lexer and parser tables
	if (lexer.shLInitialize((home+"grammar/letters.in").c_str(),
				(home+"grammar/tokens.in").c_str(),
				(home+"grammar/keywords.in").c_str(), tokensRebuild) != SH_L_SUCCESS)
	{
		cout << "Did you move the DocThis! executable from its home? Use --help option to see how to fix this." << endl;
		return EXIT_FAILURE;
	}
	// Get the parser to read the grammar and check for errors in the grammar or incompatibility with grammar type
	parser.shPAmbiguityResolution(SH_P_AMBIGUITY_RESOLVE_ACCEPT_FIRST);
	switch (parser.shPReadLLKGrammar(lexer, (home+"grammar/grammar.in").c_str(), 1, parseTableRebuild))
	{
		case SH_P_FILE_NOT_FOUND:
		case SH_P_LEXER_NOT_INITIALIZED:
		case SH_P_GRAMMAR_ERROR:
		case SH_P_AMBIGUITY:
		case SH_P_NOT_ENOUGH_MEMORY:
		case SH_P_LEFT_RECURSION:
			cout << parser.shPError();
			return EXIT_FAILURE;
		default:
			if (verbose)
				cout << parser.shPError();
			break;
	}
	// Step 4. From here on, everything is related to generated output
	if (files.size() == 0 && !buildTexMainFromDir && !copyCss && !rewriteCss && !outputVersion && !tokensRebuild && !parseTableRebuild)
	{
		cout << "error: No source files specified. Use --help option to see usage." << endl;
		return EXIT_FAILURE;
	}
	if (files.size() > 0 && !buildHtml && !buildTex)
	{
		cout << "warning: No target output format specified. Assuming HTML." << endl;
		buildHtml = true;
	}
	// making directories could fail (return value non-zero), but that could be because the directory exists.  Therefore, return value of makeDir is ignored
	if (buildHtml || buildTex || copyCss || buildTexMain || buildTexMainFromDir)
		(void)makeDir("generated");
	if (buildHtml || copyCss)
	{
		(void)makeDir("generated/html");
		if (copyCss)
			copyFile((home+"/style.css").c_str(), "generated/html/style.css");
	}
	if (buildTex || buildTexMain || buildTexMainFromDir)
	{
		(void)makeDir("generated/tex");
		if (buildTexMainFromDir || buildTexMain)
		{
			ofstream fout("generated/tex/Makefile.obj");	// This file will look like this:
			fout << ".PHONY: objects" << endl;		// .PHONY: objects
			fout << "objects:";				// objects: somefile some_other_file
			ofstream fout2("generated/tex/Makefile.cvt");	// This file will contain individual rules
			ofstream fout3("generated/tex/Makefile.cln");	// This file will look like this:
			fout3 << ".PHONY: objects_clean" << endl;	// .PHONY: objects_clean
			fout3 << "objects_clean:" << endl;		// objects_clean:
									// 	-rm -f somefile
									//	-rm -f some_other_file
		}
	}
	if (files.size() == 0 && (buildTexMain || buildHtml || buildTex))	// With other options, it doesn't matter if no file is specified
	{
		cout << "error: No source files specified. Use --help option to see usage." << endl;
		return EXIT_FAILURE;
	}
	// Give lexer callbacks
	lexer.shLAddNoMatchErrorFunction(noTokenMatchFunction);
	// Give parser callbacks
	parser.shPSetLLK_PLErrorHandler(phraseLevelErrorHandler);
	parser.shPAddActionRoutine("@docType", docType);
	parser.shPAddActionRoutine("@setDocName", setDocName);
	parser.shPAddActionRoutine("@fieldType", fieldType);
	parser.shPAddActionRoutine("@setNextPrevType", setNextPrevType);
	parser.shPAddActionRoutine("@fieldValue", fieldValue);
	parser.shPAddActionRoutine("@seealsoValue", seealsoValue);
	parser.shPAddActionRoutine("@pop", pop);
	parser.shPAddActionRoutine("@push", push);
	parser.shPAddActionRoutine("@pushEmpty", pushEmpty);
	parser.shPAddActionRoutine("@pushCodeWord", pushCodeWord);
	parser.shPAddActionRoutine("@pushLinkKeyword", pushLinkKeyword);
	parser.shPAddActionRoutine("@pushCode", pushCode);
	parser.shPAddActionRoutine("@pushEmphasize", pushEmphasize);
	parser.shPAddActionRoutine("@pushStrong", pushStrong);
	parser.shPAddActionRoutine("@pushHTML", pushHTML);
	parser.shPAddActionRoutine("@pushAnchor", pushAnchor);
	parser.shPAddActionRoutine("@pushNoLink", pushNoLink);
	parser.shPAddActionRoutine("@pushYesLink", pushYesLink);
	parser.shPAddActionRoutine("@pushInline", pushInline);
	parser.shPAddActionRoutine("@pushIndent", pushIndent);
	parser.shPAddActionRoutine("@pushNewLine", pushNewLine);
	parser.shPAddActionRoutine("@memberOrGlobal", memberOrGlobal);
	parser.shPAddActionRoutine("@pushMacroDeclaration", pushMacroDeclaration);
	parser.shPAddActionRoutine("@pushVariableDeclaration", pushVariableDeclaration);
	parser.shPAddActionRoutine("@pushTypeDeclaration", pushTypeDeclaration);
	parser.shPAddActionRoutine("@pushFunctionDeclaration", pushFunctionDeclaration);
	parser.shPAddActionRoutine("@pushInputDeclaration", pushInputDeclaration);
	parser.shPAddActionRoutine("@pushOutputDeclaration", pushOutputDeclaration);
	parser.shPAddActionRoutine("@pushConstGroupDeclaration", pushConstGroupDeclaration);
	parser.shPAddActionRoutine("@pushConstDeclaration", pushConstDeclaration);
	parser.shPAddActionRoutine("@pushImportance", pushImportance);
	parser.shPAddActionRoutine("@pushMightHaveSpace", pushMightHaveSpace);
	parser.shPAddActionRoutine("@pushArg", pushArg);
	parser.shPAddActionRoutine("@pushHasValue", pushHasValue);
	parser.shPAddActionRoutine("@pushDefaultValue", pushDefaultValue);
	parser.shPAddActionRoutine("@pushParagraphAndItems", pushParagraphAndItems);
	parser.shPAddActionRoutine("@concat", concat);
	parser.shPAddActionRoutine("@concatKeepsSpace", concatKeepsSpace);
	parser.shPAddActionRoutine("@concatIgnoresSpace", concatIgnoresSpace);
	parser.shPAddActionRoutine("@concatStoreState", concatStoreState);
	parser.shPAddActionRoutine("@concatRestoreState", concatRestoreState);
	parser.shPAddActionRoutine("@concatDefaultValue", concatDefaultValue);
	parser.shPAddActionRoutine("@concatParagraphAndItems", concatParagraphAndItems);
	parser.shPAddActionRoutine("@inLink", inLink);
	parser.shPAddActionRoutine("@notInLink", notInLink);
	parser.shPAddActionRoutine("@closeItemsUntil", closeItemsUntil);
	parser.shPAddActionRoutine("@headingBegin", headingBegin);
	parser.shPAddActionRoutine("@headingEnd", headingEnd);
	parser.shPAddActionRoutine("@itemBegin", itemBegin);
	parser.shPAddActionRoutine("@itemEnd", itemEnd);
	parser.shPAddActionRoutine("@paragraphEnd", paragraphEnd);
	parser.shPAddActionRoutine("@codeBlockBegin", codeBlockBegin);
	parser.shPAddActionRoutine("@codeBlockEnd", codeBlockEnd);
	parser.shPAddActionRoutine("@hrEnd", hrEnd);
	parser.shPAddActionRoutine("@importantCodeEnd", importantCodeEnd);
	parser.shPAddActionRoutine("@definitionBegin", definitionBegin);
	parser.shPAddActionRoutine("@definitionEnd", definitionEnd);
	parser.shPAddActionRoutine("@makeLink", makeLink);
	parser.shPAddActionRoutine("@readWhiteSpace", readWhiteSpace);
	parser.shPAddActionRoutine("@ignoreWhiteSpace", ignoreWhiteSpace);
	parser.shPAddActionRoutine("@setOverview", setOverview);
	parser.shPAddActionRoutine("@setElementExplanation", setElementExplanation);
	parser.shPAddActionRoutine("@setShortExplanation", setShortExplanation);
	parser.shPAddActionRoutine("@setNotice", setNotice);
	parser.shPAddActionRoutine("@checkCompleteness", checkCompleteness);
	parser.shPAddActionRoutine("@increaseIndent1", increaseIndent1);
	parser.shPAddActionRoutine("@decreaseIndent1", decreaseIndent1);
/*	parser.shPAddActionRoutine("@increaseIndent2", increaseIndent2);
	parser.shPAddActionRoutine("@decreaseIndent2", decreaseIndent2);*/
	parser.shPAddActionRoutine("@increaseIndent3", increaseIndent3);
	parser.shPAddActionRoutine("@decreaseIndent3", decreaseIndent3);
	parser.shPAddActionRoutine("@increaseIndent4", increaseIndent4);
	parser.shPAddActionRoutine("@decreaseIndent4", decreaseIndent4);
	parser.shPAddActionRoutine("@increaseIndent5", increaseIndent5);
	parser.shPAddActionRoutine("@decreaseIndent5", decreaseIndent5);
	parser.shPAddActionRoutine("@addToGlobalList", addToGlobalList);
	parser.shPAddActionRoutine("@documentHeader", documentHeader);
	parser.shPAddActionRoutine("@documentBottom", documentBottom);
	parser.shPAddActionRoutine("@writeDocument", writeDocument);
	list<string> texdocs;
	string texdocumentTitle;
	string texdocumentAuthor;
	string texdocumentVersion;
	set<string> texdocumentKeywords;
	formattedText gdocumentHeader;
	formattedText gdocumentBottom;
	string gdocumentVersion;
	bool allSuccessful = true;
	bool firstTime = true;
	for (list<string>::iterator file = files.begin(), end = files.end(); file != end; ++file)
	{
		scanningForGlobals = false;
		resetSemantics();
		outputFormats(buildHtml, buildTex, buildSummary, buildDetails);
		if (noticeFile != "" && readNotices(noticeFile) && firstTime)
			cout << "Could not open notice file " << noticeFile << endl;
		firstTime = false;
		// Do the actual parsing
		int ret = lexer.shLOpenFile(file->c_str());
		if (ret == SH_L_FILE_NOT_FOUND)
		{
			cout << "Could not open file " << *file << endl;
			allSuccessful = false;
			continue;
		}
		else if (ret == SH_L_NOT_ENOUGH_MEMORY)
		{
			cout << "Not enough memory!" << endl;
			allSuccessful = false;
			continue;
		}
		else if (ret == SH_L_FAIL)
		{
			cout << "Not a readable file: " << *file << endl;
			cout << "Was this a binary file?" << endl;
			allSuccessful = false;
			continue;
		}
		currentFileName(lexer.shLFileName());
		bool success = true;
		switch (parser.shPParseLLK(lexer))
		{
			case SH_P_FILE_NOT_FOUND:
				cout << parser.shPError();
				success = false;
				allSuccessful = false;
				break;
			case SH_P_RULES_NOT_LOADED:
				cout << parser.shPError();
				success = false;
				allSuccessful = false;
				break;
			case SH_P_PARSE_ERROR:
				if (docType() == INVALID)
					printMessage("Not a DocThis! file");
				success = false;
				allSuccessful = false;
			case SH_P_SUCCESS:
				cout << parser.shPError();
				if (!noError())
					allSuccessful = false;
				if (buildTexMain)
				{
					if (docType() == INDEX || (docType() != INVALID && texdocumentTitle == ""))
										// find the name/author of index document,
										// or if non existent the first name/author encountered
					{
						texdocumentTitle = docTitle().plain;
						texdocumentAuthor = docAuthor();
					}
					if (docType() != INVALID)
					{
						if (texdocumentVersion == "")
							texdocumentVersion = docVersion();
						else if (docVersion() != "" && texdocumentVersion != docVersion())
						{
							printMessage("error: Mismatch in version among files. Previously, version was: %s, but in this file it is: %s",
									texdocumentVersion.c_str(), docVersion().c_str());
							allSuccessful = false;
							if (docVersion() < texdocumentVersion)
								texdocumentVersion = docVersion();
						}
						list<string> keywords = docKeywords();
						for (list<string>::const_iterator i = keywords.begin(), end = keywords.end(); i != end; ++i)
							texdocumentKeywords.insert(*i);
					}
					texdocs.push_back(removeSpace(docName().plain));
				}
				if (success && docType() == GLOBALS)
				{
					gdocumentHeader = docHeader();
					gdocumentBottom = docBottom();
					gdocumentVersion = docVersion();
					list<docGlobalItem> globalsFiles = docGlobalsList();
					list<docInfo> gdocs;
					string globalsDir = "";
					for (int i = file->size()-1; i >= 0; --i)
						if ((*file)[i] == '/' || (*file)[i] == '\\')
						{
							globalsDir = file->substr(0, i+1);
							break;
						}
					if (globalsDir == "")		// if it was in the current directory
						globalsDir = "./";
					for (list<docGlobalItem>::const_iterator i = globalsFiles.begin(), end = globalsFiles.end(); i != end; ++i)
					{
						scanningForGlobals = true;
						resetSemantics();
						outputFormats(false, false, false, false);
						if (lexer.shLOpenFile((globalsDir+i->filename).c_str()) != SH_L_SUCCESS)
						{
							printMessage(i->line, i->column, "error: %s: Could not open file", (globalsDir+i->filename).c_str());
							allSuccessful = false;
							continue;
						}
						parser.shPParseLLK(lexer);	// Doesn't matter if parse error, as long as the document has proper header
						if (docType() == INVALID)	// Not a DocThis! file. Ignore it
						{
							printMessage(i->line, i->column, "error: %s: Not a valid DocThis! file", (globalsDir+i->filename).c_str());
							allSuccessful = false;
							continue;
						}
						docInfo doc;
						doc.name = docName();
						doc.filename = removeSpace(doc.name.plain);
						doc.type = docType();
						switch (doc.type)
						{
						case CLASS:
						case STRUCT:
						case FUNCTIONS:
						case VARIABLES:
						case API:
							docGlobals(doc.macros, doc.types, doc.globalTypes, doc.variables, doc.globalVariables, doc.functions, doc.globalFunctions);
							if (doc.macros.size() == 0 && doc.types.size() == 0 && doc.globalTypes.size() == 0
									&& doc.variables.size() == 0 && doc.globalVariables.size() == 0
									&& doc.functions.size() == 0 && doc.globalFunctions.size() == 0)
							{
								printMessage(i->line, i->column, "error: %s: File does not contain any program identifiers", (globalsDir+i->filename).c_str());
								allSuccessful = false;
							}
							else
							{
								gdocs.push_back(doc);
								if (docVersion() != "" && gdocumentVersion != docVersion())
								{
									printMessage(i->line, i->column, "error: %s: Mismatch in version among files. Globals document version is: %s,"
											" but in this file it is: %s", (globalsDir+i->filename).c_str(),
											gdocumentVersion.c_str(), docVersion().c_str());
									allSuccessful = false;
								}
							}
							break;
						case INDEX:
						case IOFILE:
						case CONSTANTS:
						case EXAMPLE:
						case OTHER:
							printMessage(i->line, i->column, "error: %s: Global idenentifiers cannot exist in this file type", (globalsDir+i->filename).c_str());
							allSuccessful = false;
							break;
						case GLOBALS:
							printMessage(i->line, i->column, "error: \"globals\" cannot appear in the list of globals");
							allSuccessful = false;
							break;
						default:
							break;
						}
					}
					ofstream htmlout, texout;
					bool couldOpenHtml = false, couldOpenTex = false;
					if (buildHtml)
					{
						htmlout.open("generated/html/globals.html");
						if (htmlout.is_open())
							couldOpenHtml = true;
					}
					if (buildTex)
					{
						texout.open("generated/tex/globals.tex");
						if (texout.is_open())
							couldOpenTex = true;
					}
					if (couldOpenHtml)
						htmlout << gdocumentHeader.html;
					if (couldOpenTex)
						texout << gdocumentHeader.tex;
					writeGlobals(gdocs, couldOpenHtml?&htmlout:'\0', couldOpenTex?&texout:'\0');
					if (couldOpenHtml)
						htmlout << gdocumentBottom.html << endl;
					if (couldOpenTex)
						texout << gdocumentBottom.tex << endl;
					if (!couldOpenHtml || !couldOpenTex)
					{
						printMessage("error: Error writing output: Could not open file(s)");
						allSuccessful = false;
					}
				}
				break;
			default:
				printMessage("internal error: Unknown parser return value");
				allSuccessful = false;
				break;
		}
	}
	int dir_length = texMainSourceDir.length();
	if (texMainSourceDir[dir_length-1] != '/' && texMainSourceDir[dir_length-1] != '\\')
		texMainSourceDir += '/';
	if (buildTexMainFromDir)
	{
		DIR *dir = opendir(texMainSourceDir.c_str());
		if (dir == '\0')
		{
			cout << "Could not open " << texMainSourceDir << " as a directory" << endl;
			allSuccessful = false;
		}
		else
		{
			list<filenameWithType> texdocsWithType;
			struct dirent *entry;
			while ((entry = readdir(dir)) != '\0')
			{
				resetSemantics();
				outputFormats(false, false, false, false);
#ifdef _DIRENT_HAVE_D_TYPE
				if (entry->d_type != DT_REG)		// if not a regular file
					continue;
#endif
				if (lexer.shLOpenFile((texMainSourceDir+entry->d_name).c_str()) != SH_L_SUCCESS)
					continue;
				currentFileName(lexer.shLFileName());
				parser.shPParseLLK(lexer);	// Doesn't matter if parse error, as long as the document has proper header
				if (docType() == INVALID)	// Not a DocThis! file. Ignore it
					continue;
				if (docType() == INDEX || texdocumentTitle == "")
									// find the name/author of index document,
									// or if non existent the first name/author encountered
				{
					texdocumentTitle = docTitle().plain;
					texdocumentAuthor = docAuthor();
				}
				if (texdocumentVersion == "")
					texdocumentVersion = docVersion();
				else if (docVersion() != "" && texdocumentVersion != docVersion())
				{
					printMessage("error: Mismatch in version among files. Previously, version was: %s, but in this file it is: %s",
							texdocumentVersion.c_str(), docVersion().c_str());
					allSuccessful = false;
					if (docVersion() < texdocumentVersion)
						texdocumentVersion = docVersion();
				}
				list<string> keywords = docKeywords();
				for (list<string>::const_iterator i = keywords.begin(), end = keywords.end(); i != end; ++i)
					texdocumentKeywords.insert(*i);
				filenameWithType ft;
				ft.filename = removeSpace(docName().plain);
				ft.type = docType();
				texdocsWithType.push_back(ft);
			}
			closedir(dir);
			texdocsWithType.sort();
			for (list<filenameWithType>::const_iterator i = texdocsWithType.begin(), end = texdocsWithType.end(); i != end; ++i)
				texdocs.push_back(i->filename);
		}
	}
	if (buildTexMain || buildTexMainFromDir)
	{
		if (texdocumentTitle == "" || texdocumentAuthor == "" || texdocumentVersion == "")
		{
			cout << "error: Cannot determine documentation name, author or version because none of the files could be "
				"compiled far enough to retrieve these information" << endl;
			if (texdocumentTitle == "")
				texdocumentTitle = "UNKNOWN TITLE";
			if (texdocumentAuthor == "")
				texdocumentAuthor = "UNKNOWN AUTHOR";
			if (texdocumentVersion == "")
				texdocumentVersion = "UNKNOWN VERSION";
			allSuccessful = false;
		}
		if (!writeTexExtra(texdocs, texdocumentTitle, texdocumentAuthor, texdocumentVersion, texdocumentKeywords, compactTexMain, verbosePdfLatex))
		{
			currentFileName("generated/tex/documentation.tex");
			cout << "error: Error writing output: Could not open file(s)" << endl;
			allSuccessful = false;
		}
	}
	return allSuccessful?EXIT_SUCCESS:EXIT_FAILURE;
}
