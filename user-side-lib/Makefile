include ../Makefile.config
include ../Makefile.common

KERNEL_HOME := ../kernel-module
CFLAGS += -I$(KERNEL_HOME) $(USER_EXTRA_CFLAGS) $(EXTRA_CFLAGS)
CXXFLAGS += -I$(KERNEL_HOME) $(USER_EXTRA_CXXFLAGS) $(EXTRA_CXXFLAGS)
INC_BUILD := ./increase_build
BUILD_FILE := build.txt
BUILD_DIR := build
STATIC_BUILD_DIR := build/static
SHARED_BUILD_DIR := build/shared

.PHONY: all
all: library auxiliary calibrator regionalizer services view pyskin rosskin
.PHONY: library auxiliary calibrator regionalizer services view pyskin rosskin
library:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin user library...$(DEFAULT_COLOR)\n"
	-@$(MKDIR) $(BUILD_DIR) > /dev/null 2>&1 || exit 0
	-@$(MKDIR) $(STATIC_BUILD_DIR) > /dev/null 2>&1 || exit 0
	-@$(MKDIR) $(SHARED_BUILD_DIR) > /dev/null 2>&1 || exit 0
	@$(MAKE) --no-print-directory libskin.a $(HIGHLIGHT)
	@$(MAKE) --no-print-directory libskin-$(USER_VERSION).so $(HIGHLIGHT)
  ifeq ($(CONFIG_SKIN_DEVELOPER_MODE), y)
	@$(MAKE) --no-print-directory $(INC_BUILD)
	@$(MAKE) --no-print-directory $(BUILD_FILE)
  endif
endif
auxiliary:
	@$(PRINTF) "$(STATUS_COLOR)Building auxiliary libraries...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C aux
calibrator:
	@$(PRINTF) "$(STATUS_COLOR)Building skin calibrator...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C calibrator $(HIGHLIGHT)
regionalizer:
	@$(PRINTF) "$(STATUS_COLOR)Building skin regionalizer...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C regionalizer $(HIGHLIGHT)
services:
	@$(MAKE) --no-print-directory -C services
view:
	@$(PRINTF) "$(STATUS_COLOR)Building skin visualizer...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C view $(HIGHLIGHT)
pyskin:
ifeq ($(CONFIG_SKIN_PYTHON), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin python bindings...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C pyskin $(HIGHLIGHT)
endif
rosskin:
ifeq ($(CONFIG_SKIN_ROSSKIN), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin ROS modules...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C rosskin $(HIGHLIGHT)
endif

LIB_OBJECTS := skin_object.o skin_region.o skin_sub_region.o skin_sensor.o skin_module.o skin_patch.o skin_reader.o \
		skin_services.o skin_sensor_type.o skin_log.o skin_iterators.o

libskin.a: $(addprefix $(STATIC_BUILD_DIR)/, $(LIB_OBJECTS))
	$(AR) $@ $^

libskin-$(USER_VERSION).so: $(addprefix $(SHARED_BUILD_DIR)/, $(LIB_OBJECTS))
	-$(RM) libskin*.so
	$(SHARED_LD) $(LIB_SHARED_LDFLAGS) -o $@ $^

COMMON_HEADERS := skin_common.h skin_internal.h skin_config.h skin_compiler.h skin_log.h skin_callbacks.h \
		skin_kernel.h skin_object.h skin_reader.h skin_services.h $(wildcard $(KERNEL_HOME)/*.h)
$(STATIC_BUILD_DIR)/%.o: %.c $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<
$(STATIC_BUILD_DIR)/%.o: %.cpp $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ -c $<
$(SHARED_BUILD_DIR)/%.o: %.c $(COMMON_HEADERS)
	@$(CC) $(CFLAGS) $(LIB_SHARED_CFLAGS) -o $@ -c $< > /dev/null 2>&1
$(SHARED_BUILD_DIR)/%.o: %.cpp $(COMMON_HEADERS)
	@$(CXX) $(CXXFLAGS) $(LIB_SHARED_CXXFLAGS) -o $@ -c $<

$(STATIC_BUILD_DIR)/skin_object.o      $(SHARED_BUILD_DIR)/skin_object.o     : skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h skin_sub_region.h skin_region.h
$(STATIC_BUILD_DIR)/skin_region.o      $(SHARED_BUILD_DIR)/skin_region.o     : skin_sensor.h skin_sub_region.h skin_region.h
$(STATIC_BUILD_DIR)/skin_sub_region.o  $(SHARED_BUILD_DIR)/skin_sub_region.o : skin_sensor.h skin_sub_region.h skin_region.h
$(STATIC_BUILD_DIR)/skin_sensor.o      $(SHARED_BUILD_DIR)/skin_sensor.o     : skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h skin_sub_region.h skin_region.h
$(STATIC_BUILD_DIR)/skin_module.o      $(SHARED_BUILD_DIR)/skin_module.o     : skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h
$(STATIC_BUILD_DIR)/skin_patch.o       $(SHARED_BUILD_DIR)/skin_patch.o      : skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h
$(STATIC_BUILD_DIR)/skin_sensor_type.o $(SHARED_BUILD_DIR)/skin_sensor_type.o: skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h
$(STATIC_BUILD_DIR)/skin_reader.o      $(SHARED_BUILD_DIR)/skin_reader.o     : skin_sensor.h skin_sensor_type.h
$(STATIC_BUILD_DIR)/skin_services.o    $(SHARED_BUILD_DIR)/skin_services.o   : skin_services_internal.h
$(STATIC_BUILD_DIR)/skin_log.o         $(SHARED_BUILD_DIR)/skin_log.o        :
$(STATIC_BUILD_DIR)/skin_iterators.o   $(SHARED_BUILD_DIR)/skin_iterators.o  : skin_sensor.h skin_sensor_type.h skin_module.h skin_patch.h skin_sub_region.h skin_region.h

$(BUILD_FILE): libskin.a
	$(INC_BUILD) $@
$(INC_BUILD): increase_build.c
	$(CC) -o $@ $< -lrt

.PHONY: clean
clean:
	-$(RM) $(BUILD_DIR)/*.o
	-$(RM) libskin.a libskin*.so
	-$(RM) $(INC_BUILD)
	-@$(MAKE) --no-print-directory -C calibrator clean
	-@$(MAKE) --no-print-directory -C regionalizer clean
	-@$(MAKE) --no-print-directory -C services clean
	-@$(MAKE) --no-print-directory -C view clean
