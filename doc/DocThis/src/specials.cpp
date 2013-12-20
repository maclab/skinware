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

#include <fstream>
#include <shcompiler.h>
#include "specials.h"
#include "error_print.h"

using namespace std;

string fixHtmlAnchor(const string &s);
string fixTexAnchor(const string &s);

void writeGlobalsPart(const char *title, const string &filename, const list<docIdentifier> &symbols, ofstream *htmlout, ofstream *texout, bool is_func)
{
	if (symbols.size() == 0)
		return;
	if (htmlout)
		*htmlout << "\t\t<h4 class=\"globals\">" << title << "</h4>" << endl <<
			"\t\t\t<ul class=\"globals\">" << endl;
	if (texout)
		*texout << "\\subsubsection*{" << title << "}" << endl <<
			"\\begin{itemize}" << endl;
	for (list<docIdentifier>::const_iterator i = symbols.begin(), end = symbols.end(); i != end; ++i)
	{
		if (htmlout)
			*htmlout << "\t\t\t\t<li class=\"globals\"><a class=\"globals\" href=\"" << filename << ".html#" <<
				fixHtmlAnchor(i->anchor) << "\"><code class=\"globals\">" << i->name.html << (is_func?"()":"") << "</code></a>"<< endl;
		if (texout)
			*texout << "\\item \\code{\\ulhyperref{" << fixTexAnchor(filename) << " " << fixTexAnchor(i->anchor) << "}{" << i->name.tex << (is_func?"()":"") << "}}" << endl;
	}
	if (htmlout)
		*htmlout << "\t\t\t</ul>" << endl;
	if (texout)
		*texout << "\\end{itemize}" << endl;
}

void writeGlobals(const list<docInfo> &docs, ofstream *htmlout, ofstream *texout)
{
	for (list<docInfo>::const_iterator i = docs.begin(), end = docs.end(); i != end; ++i)
	{
		if (i != docs.begin())
			if (htmlout)
				*htmlout << "\t<div class=\"globalsHr\"></div>" << endl;
		if (htmlout)
			*htmlout << "\t<h2 class=\"globals\"><a class=\"globalsHeading\" href=\"" << i->filename << ".html\">";
		if (texout)
			*texout << "\\subsection*{\\texorpdfstring{\\ulhyperref{" << fixTexAnchor(i->filename) << "}{";
		switch (i->type)
		{
		case CLASS:
			if (htmlout)
				*htmlout << "<code class=\"inHeading\">" << i->name.html << "</code></a> Class";
			if (texout)
				*texout << "\\code{" << i->name.tex << "}}}{" << i->name.tex << "} Class";
			break;
		case STRUCT:
			if (htmlout)
				*htmlout << "<code class=\"inHeading\">" << i->name.html << "</code></a> Structure";
			if (texout)
				*texout << "\\code{" << i->name.tex << "}}}{" << i->name.tex << "} Structure";
			break;
		case FUNCTIONS:
			if (htmlout)
				*htmlout << "<code class=\"inHeading\">" << i->name.html << "</code></a> Functions";
			if (texout)
				*texout << "\\code{" << i->name.tex << "}}}{" << i->name.tex << "} Functions";
			break;
		case VARIABLES:
			if (htmlout)
				*htmlout << "<code class=\"inHeading\">" << i->name.html << "</code></a> Variables";
			if (texout)
				*texout << "\\code{" << i->name.tex << "}}}{" << i->name.tex << "} Variables";
			break;
		case API:
			if (htmlout)
				*htmlout << i->name.html << "</a>";
			if (texout)
				*texout << i->name.tex << "}}{" << i->name.tex << "}";
			break;
		case INDEX:
		case IOFILE:
		case CONSTANTS:
		case EXAMPLE:
		case OTHER:
		case GLOBALS:
		default:
			printMessage("Internal Error: Unexpected document type: %d", (int)i->type);
			break;
		}
		if (htmlout)
			*htmlout << "</h2>" << endl;
		if (texout)
			*texout << "}" << endl;
		writeGlobalsPart("Macros", i->filename, i->macros, htmlout, texout, true);
		writeGlobalsPart("Types", i->filename, i->globalTypes, htmlout, texout, false);
		writeGlobalsPart("Internal Types", i->filename, i->types, htmlout, texout, false);
		writeGlobalsPart("Member Variables", i->filename, i->variables, htmlout, texout, false);
		writeGlobalsPart("Member Functions", i->filename, i->functions, htmlout, texout, true);
		writeGlobalsPart("Global Variables", i->filename, i->globalVariables, htmlout, texout, false);
		writeGlobalsPart("Global Functions", i->filename, i->globalFunctions, htmlout, texout, true);
	}
}

bool writeTexExtra(const list<string> &files, const string &doctitle, const string &docauthor, const string &docversion, const set<string> &dockeywords,
		bool compact, bool verbosePdfLatex)
{
	ofstream mainout("generated/tex/documentation.tex");
	if (!mainout.is_open())
		return false;
	ofstream makefileout("generated/tex/Makefile");
	if (!makefileout.is_open())
	{
		mainout.close();
		return false;
	}
	mainout << "\\documentclass[11pt,a4paper]{article}\n"
		"\\usepackage{graphicx}\n"
		"\\usepackage{indentfirst}\n"
		"\\usepackage[usenames,dvipsnames]{xcolor}\n"
		"\\usepackage[framemethod=TikZ]{mdframed}\n"
		"\\usepackage{soul}\n"
		"\\usepackage{setspace}\n"
		"\\usepackage{microtype}\n"
		"\\DisableLigatures[-]{}\n"
		"\\usepackage{parskip}\n"
		"\\usepackage{listings}\n"
		"\\usepackage[T1]{fontenc}\n"
		"\\usepackage{lmodern}\n"
		"\\usepackage{fancyhdr}\n"
		"\\usepackage{hyperref}\n"
		"\\usepackage[all]{hypcap}\n"
		"\n"
		"\\setlength{\\emergencystretch}{20em}\n"
		"\n"
		"\\newcommand{\\code}[1]{\\textbf{\\texttt{#1}}}\n"
		"\\soulregister{\\code}{1}\n"
		"\\newcommand{\\notice}[1]{\\textit{#1}}\n"
		"\\newcommand{\\internalLinkColor}{MidnightBlue}\n"
		"\\newcommand{\\externalLinkColor}{Maroon}\n"
		"\\definecolor{unimportant}{rgb}{0.25,0.25,0.25}\n"
		"\\definecolor{somewhatImportant}{rgb}{0.0,0.2,0.4}\n"
		"\\definecolor{veryImportant}{rgb}{0.0,0.4,0.7}\n"
		"\\newcommand{\\unimportantCode}[1]{\\hypersetup{linkcolor=unimportant}\\textcolor{unimportant}{#1}\\hypersetup{linkcolor=\\internalLinkColor}}\n"
		"\\newcommand{\\somewhatImportantCode}[1]{\\hypersetup{linkcolor=somewhatImportant}\\textcolor{somewhatImportant}{#1}\\hypersetup{linkcolor=\\internalLinkColor}}\n"
		"\\newcommand{\\veryImportantCode}[1]{\\hypersetup{linkcolor=veryImportant}\\textcolor{veryImportant}{#1}\\hypersetup{linkcolor=\\internalLinkColor}}\n"
		"\\newcommand{\\ulhyperref}[2]{\\hyperref[#1]{\\ul{#2}}}\n"
		"\\newcommand{\\ulhref}[2]{\\href{#1}{\\ul{#2}}}\n"
		"\n"
		"\\pagestyle{fancy}\n"
		"\\fancyhf{}\n"
		"\n"
		"\\lstset{\n"
		"\tbasicstyle=\\footnotesize\\ttfamily,\n"
		"\tshowstringspaces=false,\n"
		"\tbreaklines=true,\n"
		"\tkeywordstyle={},\n"
		"\tframe=none,\n"
		"\tescapechar=\\&\n"
		"}\n"
		"\n"
		"\\hypersetup{\n"
		"\tpdftitle={" << doctitle << "},\n"
		"\tcolorlinks=true,\n"
		"\tlinkcolor=\\internalLinkColor,\n"
		"\turlcolor=\\externalLinkColor\n"
		"}\n"
		"\n"
		"\\newmdenv[roundcorner=5pt,linecolor=gray,innerleftmargin=10pt,innerrightmargin=10pt,innertopmargin=2pt,innerbottommargin=2pt]{codeblockframe}\n"
		"\n"
		"\\begin{document}\n"
		"\n"
		"\\setlength{\\parindent}{0pt}\n"
		"\n"
		"\\pagenumbering{alph}\n"
		"\n"
		"\\begin{titlepage}\n"
		"\\thispagestyle{empty}\n"
		"\\begin{center}\n"
		"\\vspace*{50mm}\n"
		"\\begin{spacing}{1}{\\Huge \\textbf{" << doctitle << "}\\\\[5mm]}\\end{spacing}\n"
		"{\\large " << docversion << "}\\\\[20mm]\n"
		"{\\Large \\textbf{Author:} " << docauthor << "}\\\\[50mm]\n"
		"\\textbf{keywords:} \\textit{";
	for (set<string>::const_iterator i = dockeywords.begin(), end = dockeywords.end(); i != end; ++i)
		mainout << (i == dockeywords.begin()?"":", ") << *i;
	mainout << "}\n"
		"\\end{center}\n"
		"\\null\\vfill\n"
		"\\end{titlepage}\n"
		"\n"
		"\\pagenumbering{arabic}\n"
		"\\fancyhead[LO]{\\nouppercase{\\leftmark}}\n"
		"\n";
	makefileout << "documentation.pdf: documentation.tex";
	for (list<string>::const_iterator file = files.begin(), end = files.end(); file != end; ++file)
	{
		if (compact)
			mainout << "\\input{";
		else
			mainout << "\\include{";
		mainout << *file << "}" << endl;
		makefileout << " " << *file << ".tex";
	}
	mainout << "\n\\end{document}" << endl;
	makefileout << "\n"
		"\t@$(MAKE) --no-print-directory objects\n"
		"\tpdflatex ";
	if (!verbosePdfLatex)
		makefileout << "-interaction batchmode ";
	makefileout << "documentation.tex";
	if (!verbosePdfLatex)
#ifdef _WIN32
		makefileout << " > NUL";
#else
		makefileout << " > /dev/null";
#endif
	makefileout << endl <<"\tpdflatex ";
	if (!verbosePdfLatex)
		makefileout << "-interaction batchmode ";
	makefileout << "documentation.tex";
	if (!verbosePdfLatex)
#ifdef _WIN32
		makefileout << " > NUL";
#else
		makefileout << " > /dev/null";
#endif
	makefileout << "\t\t# rerun to get the references right\n"
		"-include Makefile.obj\n"
		"-include Makefile.cvt\n"
		"-include Makefile.cln\n"
		".PHONY: clean\n"
		"clean:\n"
		"\t-rm -f documentation.pdf *.aux documentation.out\n"
		"\t-@$(MAKE) --no-print-directory objects_clean" << endl;
	return true;
}
