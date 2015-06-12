ACLOCAL_AMFLAGS = -I m4

DT = docthis
DOCDIR = $(top_srcdir)/doc
FILES = \
        generated/html/index.html \
        generated/html/constants.html \
        generated/html/globals.html \
        generated/html/skin.html \
        generated/html/skin_sensor.html \
        generated/html/skin_module.html \
        generated/html/skin_patch.html \
        generated/html/skin_sensor_type.html \
        generated/html/skin_writer.html \
        generated/html/skin_reader.html \
        generated/html/skin_driver.html \
        generated/html/skin_user.html \
        generated/html/skin_writer_attr.html \
        generated/html/skin_reader_attr.html \
        generated/html/skin_driver_attr.html \
        generated/html/skin_user_attr.html \
        generated/html/skin_writer_callbacks.html \
        generated/html/skin_reader_callbacks.html \
        generated/html/skin_driver_callbacks.html \
        generated/html/skin_user_callbacks.html \
        generated/html/skin_writer_statistics.html \
        generated/html/skin_reader_statistics.html \
        generated/html/skin_driver_details.html \
        generated/html/skin_callback.html \
        generated/html/skin_hook.html \
        generated/html/Skin.html \
        generated/html/SkinSensor.html \
        generated/html/SkinModule.html \
        generated/html/SkinPatch.html \
        generated/html/SkinSensorType.html \
        generated/html/SkinWriter.html \
        generated/html/SkinReader.html \
        generated/html/SkinDriver.html \
        generated/html/SkinUser.html \
        generated/html/SkinWriterAttr.html \
        generated/html/SkinReaderAttr.html \
        generated/html/SkinDriverAttr.html \
        generated/html/SkinUserAttr.html \
        generated/html/SkinWriterCallbacks.html \
        generated/html/SkinReaderCallbacks.html \
        generated/html/SkinDriverCallbacks.html \
        generated/html/SkinUserCallbacks.html \
        generated/html/SkinWriterStatistics.html \
        generated/html/SkinReaderStatistics.html \
        generated/html/SkinDriverDetails.html \
        generated/html/SkinCallback.html \
        generated/html/SkinHook.html \
        generated/html/MeanPressureonSkin.html
API_FILES = \
            $(DOCDIR)/skin \
            $(DOCDIR)/skin_sensor \
            $(DOCDIR)/skin_module \
            $(DOCDIR)/skin_patch \
            $(DOCDIR)/skin_sensor_type \
            $(DOCDIR)/skin_writer \
            $(DOCDIR)/skin_reader \
            $(DOCDIR)/skin_driver \
            $(DOCDIR)/skin_user \
            $(DOCDIR)/skin_writer_attr \
            $(DOCDIR)/skin_reader_attr \
            $(DOCDIR)/skin_driver_attr \
            $(DOCDIR)/skin_user_attr \
            $(DOCDIR)/skin_writer_callbacks \
            $(DOCDIR)/skin_reader_callbacks \
            $(DOCDIR)/skin_driver_callbacks \
            $(DOCDIR)/skin_user_callbacks \
            $(DOCDIR)/skin_writer_statistics \
            $(DOCDIR)/skin_reader_statistics \
            $(DOCDIR)/skin_driver_details \
            $(DOCDIR)/Skin \
            $(DOCDIR)/SkinSensor \
            $(DOCDIR)/SkinModule \
            $(DOCDIR)/SkinPatch \
            $(DOCDIR)/SkinSensorType \
            $(DOCDIR)/SkinWriter \
            $(DOCDIR)/SkinReader \
            $(DOCDIR)/SkinDriver \
            $(DOCDIR)/SkinUser \
            $(DOCDIR)/SkinWriterAttr \
            $(DOCDIR)/SkinReaderAttr \
            $(DOCDIR)/SkinDriverAttr \
            $(DOCDIR)/SkinUserAttr \
            $(DOCDIR)/SkinWriterCallbacks \
            $(DOCDIR)/SkinReaderCallbacks \
            $(DOCDIR)/SkinDriverCallbacks \
            $(DOCDIR)/SkinUserCallbacks \
            $(DOCDIR)/SkinWriterStatistics \
            $(DOCDIR)/SkinReaderStatistics \
            $(DOCDIR)/SkinDriverDetails

EXTRA_DIST = \
             $(DOCDIR)/index \
             $(DOCDIR)/constants \
             $(DOCDIR)/globals \
             $(DOCDIR)/skin_callback \
             $(DOCDIR)/skin_hook \
             $(DOCDIR)/SkinCallback \
             $(DOCDIR)/SkinHook \
             $(DOCDIR)/MeanPressureonSkin \
             $(API_FILES)

if HAVE_DOCTHIS
V_DOCTHIS = $(V_DOCTHIS_@AM_V@)
V_DOCTHIS_ = $(V_DOCTHIS_@AM_DEFAULT_V@)
V_DOCTHIS_0 = @echo "  DOCTHIS " $@;
V_DOCTHIS_1 =

all-local:
	@$(MAKE) tex-base-files
	@$(MAKE) files
if HAVE_PDFLATEX
	@$(MAKE) -C generated/tex
endif

.PHONY: tex-base-files
tex-base-files: generated/tex/documentation.tex
	@:
generated/tex/documentation.tex:
	$(V_DOCTHIS)$(DT) +texmain-all $(DOCDIR)/

.PHONY: files
files: $(FILES)
	@:

# Note: I can't find a way around the repetition without using GNU extensions
DT_CMD = $(V_DOCTHIS)$(DT) $< +html +tex
generated/html/index.html: $(DOCDIR)/index
	$(DT_CMD) +css
generated/html/constants.html: $(DOCDIR)/constants
	$(DT_CMD)
generated/html/skin.html: $(DOCDIR)/skin
	$(DT_CMD)
generated/html/skin_sensor.html: $(DOCDIR)/skin_sensor
	$(DT_CMD)
generated/html/skin_module.html: $(DOCDIR)/skin_module
	$(DT_CMD)
generated/html/skin_patch.html: $(DOCDIR)/skin_patch
	$(DT_CMD)
generated/html/skin_sensor_type.html: $(DOCDIR)/skin_sensor_type
	$(DT_CMD)
generated/html/skin_writer.html: $(DOCDIR)/skin_writer
	$(DT_CMD)
generated/html/skin_reader.html: $(DOCDIR)/skin_reader
	$(DT_CMD)
generated/html/skin_driver.html: $(DOCDIR)/skin_driver
	$(DT_CMD)
generated/html/skin_user.html: $(DOCDIR)/skin_user
	$(DT_CMD)
generated/html/skin_writer_attr.html: $(DOCDIR)/skin_writer_attr
	$(DT_CMD)
generated/html/skin_reader_attr.html: $(DOCDIR)/skin_reader_attr
	$(DT_CMD)
generated/html/skin_driver_attr.html: $(DOCDIR)/skin_driver_attr
	$(DT_CMD)
generated/html/skin_user_attr.html: $(DOCDIR)/skin_user_attr
	$(DT_CMD)
generated/html/skin_writer_callbacks.html: $(DOCDIR)/skin_writer_callbacks
	$(DT_CMD)
generated/html/skin_reader_callbacks.html: $(DOCDIR)/skin_reader_callbacks
	$(DT_CMD)
generated/html/skin_driver_callbacks.html: $(DOCDIR)/skin_driver_callbacks
	$(DT_CMD)
generated/html/skin_user_callbacks.html: $(DOCDIR)/skin_user_callbacks
	$(DT_CMD)
generated/html/skin_writer_statistics.html: $(DOCDIR)/skin_writer_statistics
	$(DT_CMD)
generated/html/skin_reader_statistics.html: $(DOCDIR)/skin_reader_statistics
	$(DT_CMD)
generated/html/skin_driver_details.html: $(DOCDIR)/skin_driver_details
	$(DT_CMD)
generated/html/skin_callback.html: $(DOCDIR)/skin_callback
	$(DT_CMD)
generated/html/skin_hook.html: $(DOCDIR)/skin_hook
	$(DT_CMD)
generated/html/Skin.html: $(DOCDIR)/Skin
	$(DT_CMD)
generated/html/SkinSensor.html: $(DOCDIR)/SkinSensor
	$(DT_CMD)
generated/html/SkinModule.html: $(DOCDIR)/SkinModule
	$(DT_CMD)
generated/html/SkinPatch.html: $(DOCDIR)/SkinPatch
	$(DT_CMD)
generated/html/SkinSensorType.html: $(DOCDIR)/SkinSensorType
	$(DT_CMD)
generated/html/SkinWriter.html: $(DOCDIR)/SkinWriter
	$(DT_CMD)
generated/html/SkinReader.html: $(DOCDIR)/SkinReader
	$(DT_CMD)
generated/html/SkinDriver.html: $(DOCDIR)/SkinDriver
	$(DT_CMD)
generated/html/SkinUser.html: $(DOCDIR)/SkinUser
	$(DT_CMD)
generated/html/SkinWriterAttr.html: $(DOCDIR)/SkinWriterAttr
	$(DT_CMD)
generated/html/SkinReaderAttr.html: $(DOCDIR)/SkinReaderAttr
	$(DT_CMD)
generated/html/SkinDriverAttr.html: $(DOCDIR)/SkinDriverAttr
	$(DT_CMD)
generated/html/SkinUserAttr.html: $(DOCDIR)/SkinUserAttr
	$(DT_CMD)
generated/html/SkinWriterCallbacks.html: $(DOCDIR)/SkinWriterCallbacks
	$(DT_CMD)
generated/html/SkinReaderCallbacks.html: $(DOCDIR)/SkinReaderCallbacks
	$(DT_CMD)
generated/html/SkinDriverCallbacks.html: $(DOCDIR)/SkinDriverCallbacks
	$(DT_CMD)
generated/html/SkinUserCallbacks.html: $(DOCDIR)/SkinUserCallbacks
	$(DT_CMD)
generated/html/SkinWriterStatistics.html: $(DOCDIR)/SkinWriterStatistics
	$(DT_CMD)
generated/html/SkinReaderStatistics.html: $(DOCDIR)/SkinReaderStatistics
	$(DT_CMD)
generated/html/SkinDriverDetails.html: $(DOCDIR)/SkinDriverDetails
	$(DT_CMD)
generated/html/SkinCallback.html: $(DOCDIR)/SkinCallback
	$(DT_CMD)
generated/html/SkinHook.html: $(DOCDIR)/SkinHook
	$(DT_CMD)
generated/html/MeanPressureonSkin.html: $(DOCDIR)/MeanPressureonSkin
	$(DT_CMD)
generated/html/globals.html: $(DOCDIR)/globals $(API_FILES)
	$(V_DOCTHIS)$(DT) $< +html +tex

clean-local:
	-rm -rf generated
endif
