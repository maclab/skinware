include ../../Makefile.config

DOC_THIS := ../DocThis/docthis
RM := rm -f
MKDIR := mkdir -p

SOURCE_FILES := DataTypes skink_registration_data skink_device Callbacks skink_sensor_layer skink_sensor skink_module skink_patch skin_rt Template
ALL_FILES := index constants $(SOURCE_FILES)
HTML_DIR := generated/html
TEX_DIR := generated/tex
HTML_FILES := $(addsuffix .html, $(addprefix $(HTML_DIR)/, $(ALL_FILES)))
TEX_FILES := $(addsuffix .tex, $(addprefix $(TEX_DIR)/, $(ALL_FILES)))
SOURCE_FILES_DEF := $(addprefix src/, $(SOURCE_FILES))
ALL_FILES_DEF := $(addprefix src/, $(ALL_FILES))
TEX_MAKE_FILES := $(addprefix $(TEX_DIR)/, documentation.tex Makefile Makefile.obj Makefile.cvt Makefile.cln)

ifeq ($(CONFIG_SKIN_DOC_PDFLATEX_VERBOSE), y)
  VERBOSITY := -verbose-pdflatex
else
  VERBOSITY :=
endif

.PHONY: all
all: $(TEX_MAKE_FILES) $(HTML_FILES) $(HTML_DIR)/globals.html $(TEX_FILES) $(TEX_DIR)/globals.tex
	$(DOC_THIS) +css
ifeq ($(CONFIG_SKIN_DOC_PDF), y)
	@$(MAKE) --no-print-directory -C generated/tex
endif
$(HTML_DIR)/%.html $(TEX_DIR)/%.tex: src/%
	$(DOC_THIS) $< +html +tex
$(HTML_DIR)/globals.html $(TEX_DIR)/globals.tex: src/globals $(SOURCE_FILES_DEF)
	$(DOC_THIS) $< +html +tex
$(TEX_MAKE_FILES):
	$(DOC_THIS) +texmain-all src $(VERBOSITY)
.PHONY: clean
clean:
	-$(RM) $(HTML_FILES) $(HTML_DIR)/globals.html
	-$(RM) $(TEX_FILES) $(TEX_DIR)/globals.tex
	-@$(MAKE) --no-print-directory -C generated/tex clean
	-$(RM) $(TEX_MAKE_FILES)
