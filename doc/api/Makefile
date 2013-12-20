include ../../Makefile.config

DOC_THIS := ../DocThis/docthis
RM := rm -f
MKDIR := mkdir -p

SOURCE_FILES := skin_reader skin_object skin_service skin_sensor skin_sensor_type skin_region skin_sub_region skin_module skin_patch skin_service_manager\
		skin_X_iterator skin_task_statistics skin_rt
WITH_RESOURCE_FILES := CalibrationCache RegionalizationCache
ALL_FILES := $(SOURCE_FILES) $(WITH_RESOURCE_FILES) index constants skin_callbacks MeanPressureonSkin NamingConvention
HTML_DIR := generated/html
TEX_DIR := generated/tex
HTML_FILES := $(addsuffix .html, $(addprefix $(HTML_DIR)/, $(ALL_FILES)))
TEX_FILES := $(addsuffix .tex, $(addprefix $(TEX_DIR)/, $(ALL_FILES)))
SOURCE_FILES_DEF := $(addprefix src/, $(SOURCE_FILES))
WITH_RESOURCE_FILES_DEF := $(addprefix src/, $(WITH_RESOURCE_FILES))
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
	-$(MKDIR) $(HTML_DIR)/files
	-$(MKDIR) $(TEX_DIR)/files
	cp src/*.svg $(HTML_DIR)/files
	cp src/*.svg $(TEX_DIR)/files
ifeq ($(CONFIG_SKIN_DOC_PDF), y)
	@$(MAKE) --no-print-directory -C generated/tex
endif
$(HTML_DIR)/%.html $(TEX_DIR)/%.tex: src/% src/notices
	$(DOC_THIS) $< +html +tex +notices src/notices
$(HTML_DIR)/globals.html $(TEX_DIR)/globals.tex: src/globals $(SOURCE_FILES_DEF)
	$(DOC_THIS) $< +html +tex
$(TEX_MAKE_FILES): $(WITH_RESOURCE_FILES_DEF)
	-$(RM) $(addsuffix .tex, $(addprefix $(TEX_DIR)/, $(WITH_RESOURCE_FILES)))	# if Makefile.* are being built, rebuild all the files that include resources
	$(DOC_THIS) +texmain-all src $(VERBOSITY)
.PHONY: clean
clean:
	-$(RM) $(HTML_FILES) $(HTML_DIR)/globals.html
	-$(RM) $(TEX_FILES) $(TEX_DIR)/globals.tex
	-@$(MAKE) --no-print-directory -C generated/tex clean
	-$(RM) $(TEX_MAKE_FILES)
