/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKINK_DEVICES_H
#define SKINK_DEVICES_H

#ifdef __KERNEL__

#include "skink_common.h"
#include "skink_rt.h"
#include "skink_sensor_layer.h"
#include "skink_sensor.h"
#include "skink_module.h"
#include "skink_patch.h"

typedef void (*skink_device_details)(skink_sensor_layer *layer);		// Called number-of-layers times.  This function needs to fill in the following
										// data, given the layer id in layer->id_in_device
										// layer->name
										// layer->sensors_count
										// layer->modules_count
										// layer->patches_count
										// layer->number_of_buffers (it is set by default, but can be changed if desired)
										// layer->acquisition_rate
										// layer->modules[*].sensors_count
										// layer->patches[*].modules_count
										// Other fields will be automatically filled by the skin kernel

typedef void (*skink_device_busy)(bool busy);					// This function is called by the skin kernel to tell the device driver if it can
										// be unloaded or not.  If this function is called with 0, under NO circumstances
										// the module must unload.  This means that in the cleanup of the module, it MUST
										// wait for this module to be called with 1 and then proceed with unloading.

typedef int (*skink_device_acquire)(skink_sensor_layer *layer, skink_sensor_id *sensor_map, uint8_t buffer);
										// This function will be periodically called in a real-time context and thus it must
										// not call linux functions, including syscalls.  It can thus lock/unlock mutexes and
										// perform other real-time actions.  It also needs to be reentrant in case the device
										// driver has more than one layers, as this function will be called from different
										// threads running in parallel.
										// An important thing to note is that Skinware may rearrange the sensors of
										// the layer.  It is possible that the device driver has already created a map from
										// device hardware to skin patches/modules/sensors.  Upon acquiring the skin data, the device
										// driver needs to pass its stored skin sensor ids through sensor_map.
										// The acquired data needs to be filled in
										// layer->sensors[sensor_map[*]].response[buffer]
										// Data that are avaialable for use of the acquire function are:
										// layer->id_in_device
										// layer->name
										// layer->sensors_count
										// layer->modules_count
										// layer->patches_count
										// layer->acquisition_rate
										// layer->sensors[*].response[buffer]
										// layer->sensors[*].module
										// layer->modules[*].patch
										// layer->modules[*].sensors_begin/count
										// layer->patches[*].modules_begin/count
										// All other variables if existing, are internal to Skinware
										// and must be left UNTOUCHED

typedef struct skink_registration_data
{
	skink_sensor_layer_size		sensor_layers_count;
	skink_sensor_size		sensors_count;
	skink_module_size		modules_count;
	skink_patch_size		patches_count;
	skink_device_details		details_callback;
	skink_device_busy		busy_callback;
	skink_device_acquire		acquire_callback;
	const char			*device_name;			// name will be copied, so it can be freed if necessary.
										// Only the first SKINK_MAX_NAME_LENGTH characters are considered though
} skink_registration_data;

int skink_device_register(skink_registration_data *data, bool *revived);	// Returns:
										// SKINK_IN_USE: If data->device_name was being registered that was
										//   already in use
										// SKINK_TOO_EARLY: If the registration phase has not been started, the driver
										//   must wait and retry
										// SKINK_TOO_LATE: If the registration phase has passed, no more new network
										//   drivers can be added
										// SKINK_BAD_DATA: Returned if provided data are invalid
										// If provided, revived will tell if data->device_name corresponded to a
										//   paused device.  This means that this device is being restarted.  Note that if
										//   this value is set, then the details function of the device driver will not be
										//   called again, so if there are any internal structures that need to be built,
										//   the driver needs to build them outside details function.  That is, the device
										//   driver needs to be operatable even if details is never called.
										// If the operation is successful (or device is revived), the return value will be
										// non-negative which is the device id.  This id should be used with the rest of
										// the functions.
int skink_device_register_nonblocking(skink_registration_data *data,		// The nonblocking version of skink_device_register.  This function may return
					bool *revived);				// SKINK_TOO_EARLY if it is too early to register.  The driver must wait and try again
int skink_device_pause(unsigned int dev_id);					// Blocks until pause is possible.  Returns SKINK_SUCCESS, SKINK_FAIL or SKINK_BAD_ID
int skink_device_pause_nonblocking(unsigned int dev_id);			// If devices are still being initialized, returns SKINK_TOO_EARLY
int skink_device_resume(unsigned int dev_id);					// Blocks until resume is possible.  Returns SKINK_SUCCESS, SKINK_FAIL or SKINK_BAD_ID
int skink_device_resume_nonblocking(unsigned int dev_id);			// If devices are still being initialized, returns SKINK_TOO_EARLY

void skink_copy_from_previous_buffer(skink_sensor_layer *layer);		// Some drivers may decide there is no new data.  In that case, they should call this
										// function so the the contents of the previous buffer would be copied to the current.
										// Otherwise, there could be visible jitter due to multi-buffering

/* The structure of the device driver is basically like this:
variable: is_busy

init_module:
- Initialize network
- Find number of layers, sensors, modules and patches
- Build necessary internal maps, for example from (hardware micro-controller, hardware object) to (layer, module)
- is_busy <- true.  Note that from here on, this variable will be managed by the skin kernel through busy and shouldn't be changed by the device driver
- Call skink_register(registration_data, &id)
  * If returned SKINK_TOO_EARLY, then sleep a little and try again
  * If returned SKINK_IN_USE, then try with a different device_name
  * If returned SKINK_TOO_LATE, then the registration phase of the skin kernel has passed and no more new device drivers are accepted
  * If returned SKINK_BAD_DATA, then the data the provided to register are not valid
  * Else:
    + If returned SKINK_REVIVED, then details won't be called
    + If returned SKINK_SUCCESS, then details will be called
    + Call skink_device_resume.  This should return SKINK_SUCCESS, unless some previous error was ignored in which case SKINK_FAIL will be returned

cleanup_module:
- First, make sure the call to skink_device_resume in init phase has returned.  Note that otherwise a race condition happens and the kernel may crash.
- After you are sure your call above to skink_device_resume has finished, Call skink_device_pause.
  * If returned SKINK_FAIL, then there was some previous error and the device driver is in fact not in use.
  * If returned SKINK_SUCCESS:
    + Wait until is_busy is false.  Note that it could already be false because of a previous call to skink_device_pause.
- Release network
- Unload module

details:
- According to the maps previously created, fill in each layer's data (see comments of this file's skink_device_details typedef)

busy:
- is_busy <- true or false based on argument

acquire:
- Get data of the layer from the network
- Put them in layer->sensors[sensor_map[*]].response[buffer]
- If for any reason decided to stop acquiring, call skink_device_pause.  Note that in this case, either the module should be removed and reloaded, or
  an external program needs to be notified (for example through /proc or /sys entries) to later cause the device driver to call skink_device_resume.
  Alternatively, a living thread in the device driver may take responsibility for calling skink_device_pause and skink_device_resume.
- return SKINK_SUCCESS or in case of failure for any reason, SKINK_FAIL
 */

#endif /* __KERNEL__ */

#endif
