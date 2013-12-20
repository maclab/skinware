include ../../Makefile.config

DOC_THIS := ../DocThis/docthis
RM := rm -f
MKDIR := mkdir -p

FILES := index
HTML_DIR := generated/html
TEX_DIR := generated/tex
HTML_FILES := $(addsuffix .html, $(addprefix $(HTML_DIR)/, $(FILES)))
TEX_FILES := $(addsuffix .tex, $(addprefix $(TEX_DIR)/, $(FILES)))
FILES_DEF := $(addprefix src/, $(FILES))
TEX_MAKE_FILES := $(addprefix $(TEX_DIR)/, documentation.tex Makefile Makefile.obj Makefile.cvt Makefile.cln)

ifeq ($(CONFIG_SKIN_DOC_PDFLATEX_VERBOSE), y)
  VERBOSITY := -verbose-pdflatex
else
  VERBOSITY :=
endif

.PHONY: all
all: $(HTML_FILES) $(TEX_FILES)
	$(DOC_THIS) +css
	-$(MKDIR) $(HTML_DIR)/files
	-$(MKDIR) $(TEX_DIR)/files
	cp src/*.svg $(HTML_DIR)/files
	cp src/*.svg $(TEX_DIR)/files
ifeq ($(CONFIG_SKIN_DOC_PDF), y)
	@$(MAKE) --no-print-directory -C generated/tex
endif
$(HTML_DIR)/%.html $(TEX_DIR)/%.tex: src/%
	$(DOC_THIS) $< +html +tex +texmain $(VERBOSITY)
.PHONY: clean
clean:
	-$(RM) $(HTML_FILES) $(TEX_FILES)
	-@$(MAKE) --no-print-directory -C generated/tex clean
	-$(RM) $(TEX_MAKE_FILES)
