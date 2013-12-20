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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "init.h"

skink_sensor_layer_size			the_layers_count					= 0;
layer_data				*the_layers						= NULL;
module_data				*the_modules						= NULL;
patch_data				*the_patches						= NULL;

bool					the_proc_file_deleted					= true;
char					*the_read_buffer;
static uint32_t				_read_buffer_size					= 0;
static bool				_file_read_error					= false;
static uint32_t				_data_handling_index					= 0;
static struct semaphore			_file_handling_mutex1,
					_file_handling_mutex2;
static bool				_read_buffer_cut_in_the_end				= false;

#define					sem_is_locked(x)					((x).count <= 0)

#define					IS_WHITE_SPACE(x)					((x) == ' ' || (x) == '\n' || (x) == '\r'\
												|| (x) == '\t' || (x) < ' ')
#define					GET							_get_next_int()
#define					GET_STR(x)						_get_next_string((x), SKINK_MAX_NAME_LENGTH)

static uint32_t _get_next_int(void)
{
	uint32_t ret = 0;
	uint32_t cut_number_from_last_write = 0;
	int length = 0;
	bool wait_for_next = false;
	int i;
	do
	{
		if (down_interruptible(&_file_handling_mutex2))		// Wait for data to come
		{
			SKINK_LOG("Stage 1: Data acquisition interrupted by signal...aborting!");
			_file_read_error = true;
			return 0;
		}
		if (_file_read_error == 1)
		{
			up(&_file_handling_mutex2);
			return 0;
		}
		// FOR DEBUG:
		if (wait_for_next && _data_handling_index != 0)
			SKINK_LOG("Stage 1: ERROR: wait_for_next flag is raised while data handling index is not 0");
		if (wait_for_next && (the_read_buffer[_data_handling_index] > '9' || the_read_buffer[_data_handling_index] < '0'))
			// turns out the number wasn't really cut!
		{
			wait_for_next = false;
			up(&_file_handling_mutex2);		// Continue handling current data
			return cut_number_from_last_write;
		}
		// Read next int
		if (sscanf(the_read_buffer + _data_handling_index, "%u%n", &ret, &length) != 1)
		{
			SKINK_LOG("Stage 1: Error reading from buffer \"%s\"", the_read_buffer + _data_handling_index);
			// Some error in reading
			_file_read_error = true;
			up(&_file_handling_mutex2);
			return 0;
		}
		_data_handling_index += length;
		if (wait_for_next)		// If the number was cut (and the next buffer begins with number), append them to make complete number
		{
			wait_for_next = false;
			for (i = 0; i < length; ++i)
				cut_number_from_last_write *= 10;
			ret += cut_number_from_last_write;
		}
		if (_data_handling_index >= _read_buffer_size)
		{
			up(&_file_handling_mutex1);		// No more data, allow "write" function to obtain more data
			// NOTE: If the buffer ends in a number, it is assumed that the number is cut between two buffers and the module
			// would wait for the next write buffer.  However, if that is indeed the last input number, then there would be no buffers
			// being written.  Therefore, in the input, there MUST be a trailing whitespace.
			if (_read_buffer_cut_in_the_end)
			{
				cut_number_from_last_write = ret;
				wait_for_next = true;
				_read_buffer_cut_in_the_end = false;
			}
		}
		else
			up(&_file_handling_mutex2);		// Continue handling current data
	} while (wait_for_next);
//	SKINK_LOG("Read %d from input", ret);
	return ret;
}

// max_length doesn't include ending '\0'.  Make sure str has max_length+1 space
static void _get_next_string(char *str, int max_length)
{
	uint32_t index = 0;
	int length = 0;
	int wait_for_next = false;
	if (max_length == 0)
	{
		str[0] = '\0';
		return;
	}
	do
	{
		if (down_interruptible(&_file_handling_mutex2))		// Wait for data to come
		{
			SKINK_LOG("Stage 1: Data acquisition interrupted by signal...aborting!");
			_file_read_error = true;
			return;
		}
		if (_file_read_error == 1)
		{
			up(&_file_handling_mutex2);
			return;
		}
		// FOR DEBUG:
		if (wait_for_next && _data_handling_index != 0)
			SKINK_LOG("Stage 1: ERROR: wait_for_next flag is raised while data handling index is not 0");
		if (wait_for_next && IS_WHITE_SPACE(the_read_buffer[_data_handling_index]))
			// turns out the string wasn't really cut!
		{
			wait_for_next = false;
			up(&_file_handling_mutex2);		// Continue handling current data
			return;
		}
		if (!wait_for_next)
		{
			if (index > 0)
			{
				SKINK_LOG("Stage 1: ERROR: wait_for_next flag is not raised while string index is bigger than 0"
						" and buffer is read anew");
				_file_read_error = true;
				return;
			}
			// Ignore whitespace
			while (_data_handling_index < _read_buffer_size && IS_WHITE_SPACE(the_read_buffer[_data_handling_index]))
				++_data_handling_index;
			if (_data_handling_index >= _read_buffer_size)
			{
				up(&_file_handling_mutex1);		// No more data, allow "write" function to obtain more data
				// NOTE: If the buffer ends in a number, it is assumed that the string is cut between two buffers and the module
				// would wait for the next write buffer.  However, if that is indeed the last input number, then there would be no buffers
				// being written.  Therefore, in the input, there MUST be a trailing whitespace.
				wait_for_next = true;
				_read_buffer_cut_in_the_end = false;
				continue;
			}
		}
		while (_data_handling_index < _read_buffer_size && !IS_WHITE_SPACE(the_read_buffer[_data_handling_index]))
		{
			if (length < max_length)
				str[length++] = the_read_buffer[_data_handling_index];
			++_data_handling_index;
		}
		if (_data_handling_index >= _read_buffer_size)
		{
			up(&_file_handling_mutex1);		// No more data, allow "write" function to obtain more data
			// NOTE: If the buffer ends in a number, it is assumed that the number is cut between two buffers and the module
			// would wait for the next write buffer.  However, if the string is indeed finished and is the last entry in the input,
			// then there would be no buffers being written.  Therefore, in the input, there MUST be a trailing whitespace.
			if (_read_buffer_cut_in_the_end)
			{
				wait_for_next = true;
				_read_buffer_cut_in_the_end = false;
			}
		}
		else
			up(&_file_handling_mutex2);		// Continue handling current data
	} while (wait_for_next);
	str[length] = '\0';		// it is already made sure that length doesn't exceed max_length.
					// Note that str MUST have space for max_length + 1 characters
//	SKINK_LOG("Read \"%s\" from input", str);
	return;
}

static int _file_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	if (down_interruptible(&_file_handling_mutex1))		// If data being processed, wait!
								// TODO: Not such a good idea to lock the write function, but
								// if the data handling is fast enough, it's ok
	{
		SKINK_LOG("Stage 1: /proc reader interrupted by signal...aborting!");
		// TODO: What if I return 0 and the user process can try again with no error? That is the next three lines safely commented out!
		_file_read_error = true;
		if (sem_is_locked(_file_handling_mutex2))
			up(&_file_handling_mutex2);
		return 0;
	}
	if (the_read_buffer == NULL)
	{
		the_read_buffer = vmalloc((count + 1) * sizeof(*the_read_buffer));	// the count + 1 in vmalloc is to add null-termination
		_read_buffer_size = count + 1;
	}
	else if (_read_buffer_size < count + 1)
	{
		vfree(the_read_buffer);
		the_read_buffer = vmalloc((count + 1) * sizeof(*the_read_buffer));	// the count + 1 in vmalloc is to add null-termination
		_read_buffer_size = count + 1;
	}
	if (the_read_buffer == NULL)
	{
		_file_read_error = true;
		_read_buffer_size = 0;
		SKINK_LOG("Stage 1: Could not enlarge buffer!");
		return -ENOMEM;
	}
	if (copy_from_user(the_read_buffer, buf, count))
	{
		_file_read_error = true;
		SKINK_LOG("Stage 1: There was an error copying data from user space!");
		return -EFAULT;
	}
	the_read_buffer[count] = '\0';
//	SKINK_LOG("Stage 1: Received input \"%s\" with size %d", the_read_buffer, count);
	_read_buffer_size = count;
	while (_read_buffer_size >= 1 && IS_WHITE_SPACE(the_read_buffer[_read_buffer_size - 1]))
		--_read_buffer_size;	// removing trailing white space
					// because otherwise the get next int/string function wouldn't understand it has read the last
					// int/string (the index would show there is more data, but that is just whitespace)
	if (_read_buffer_size == 0)	// all space
		_read_buffer_size = 1;	// keep one of them
	if (_read_buffer_size != count)
		_read_buffer_cut_in_the_end = false;
	else
		_read_buffer_cut_in_the_end = true;
	the_read_buffer[_read_buffer_size] = '\0';
	_data_handling_index = 0;
	if (_read_buffer_size != 0)
		up(&_file_handling_mutex2);
	return count;
}

#define TEST_INPUT\
	do {\
		if (_file_read_error)\
		{\
			free_data();\
			return -1;\
		}\
	} while (0)

static int _get_data(void)
{
	skink_module_size total_modules = 0;
	skink_patch_size total_patches = 0;
	uint32_t i, j, k;
	// Get the sensor layers count and allocate memory for the sensors, modules and patches
	the_layers_count = GET;
	TEST_INPUT;
	if (the_layers_count == 0)
	{
		SKINK_LOG("Stage 1: Error in input data - Cannot have 0 layers");
		return -1;
	}
	the_layers = vmalloc(the_layers_count * sizeof(*the_layers));
	if (the_layers == NULL)
	{
		SKINK_LOG("Stage 1: Could not allocate memory");
		return -1;
	}
	total_modules = 0;
	total_patches = 0;
	for (i = 0; i < the_layers_count; ++i)
	{
		GET_STR(the_layers[i].name);
		the_layers[i].acquisition_rate = GET;
		the_layers[i].sensors_count = GET;
		the_layers[i].modules_count = GET;
		the_layers[i].patches_count = GET;
		TEST_INPUT;
		if (the_layers[i].sensors_count == 0 || the_layers[i].modules_count == 0 || the_layers[i].patches_count == 0)
		{
			SKINK_LOG("Stage 1: Error in input data - Cannot have layer with 0 sensors, modules or patches");
			free_data();
			return -1;
		}
		the_layers[i].module_data_begin = total_modules;
		the_layers[i].patch_data_begin = total_patches;
		total_modules += the_layers[i].modules_count;
		total_patches += the_layers[i].patches_count;
		the_layers[i].extra = 0;
	}
	TEST_INPUT;
	the_modules = vmalloc(total_modules * sizeof(*the_modules));
	the_patches = vmalloc(total_patches * sizeof(*the_patches));
	if (the_modules == NULL || the_patches == NULL)
	{
		SKINK_LOG("Stage 1: Could not allocate memory");
		free_data();
		return -1;
	}
	for (i = 0; i < total_modules; ++i)
		the_modules[i].sensors_count = 0;
	for (i = 0; i < total_patches; ++i)
		the_patches[i].modules_count = 0;
	// Get the patches and module data
	for (i = 0; i < total_patches; ++i)
	{
		skink_module_size modules_count;
		skink_patch_size patch_layer;
		modules_count = GET;
		patch_layer = GET;
		TEST_INPUT;
		if (patch_layer >= the_layers_count)
		{
			SKINK_LOG("Stage 1: Error in input data - patch number %u (in the order of introduction) belongs to layer %u which is nonexistent",
					i, patch_layer);
			free_data();
			return -1;
		}
		for (j = 0; j < the_layers[patch_layer].patches_count; ++j)
			if (the_patches[the_layers[patch_layer].patch_data_begin + j].modules_count == 0)		// found the next undefined patch
			{
				the_patches[the_layers[patch_layer].patch_data_begin + j].modules_count = modules_count;
				break;
			}
		if (j == the_layers[patch_layer].patches_count)
		{
			SKINK_LOG("Stage 1: Error in input data - Total number of patches of layer %u is smaller than the"
					" number of patches specified for that layer!", patch_layer);
			free_data();
			return -1;
		}
		for (j = 0; j < the_layers[patch_layer].modules_count; ++j)
			if (the_modules[the_layers[patch_layer].module_data_begin + j].sensors_count == 0)		// found the next undefined module
			{
				if (modules_count + j > the_layers[patch_layer].modules_count)
				{
					SKINK_LOG("Stage 1: Error in input data - Total number of modules of layer %u is smaller than the"
							" number of modules specified for the patches of that layer!", patch_layer);
					free_data();
					return -1;
				}
				for (k = 0; k < modules_count; ++k)
				{
					skink_sensor_size sensors_count = GET;
					the_modules[the_layers[patch_layer].module_data_begin + j + k].sensors_count = sensors_count;
					the_layers[patch_layer].extra += sensors_count;
				}
				break;
			}
	}
	for (i = 0; i < the_layers_count; ++i)
		if (the_layers[i].sensors_count != the_layers[i].extra)
		{
			SKINK_LOG("Stage 1: Error in input data - Total number of sensors of layer %u doesn't match the"
					" number of sensors specified for the modules in the patches of that layer!", i);
			free_data();
			return -1;
		}
	return 0;
}

int init_data(void)
{
	struct proc_dir_entry *proc_entry;
	int res = 0;
	// Acquire mutex
	sema_init(&_file_handling_mutex1, 1);
	sema_init(&_file_handling_mutex2, 1);
	while (down_interruptible(&_file_handling_mutex2))		// Locked at first (so _get_next_int would be locked)
		// This shouldn't really happen since the mutex has just been initialized to 1 and no body else is using it
		SKINK_LOG("Stage 1: Data initialization interrupted by signal");
	// Get initial memory
	the_read_buffer = vmalloc(4097 * sizeof(*the_read_buffer));	// the count + 1 in vmalloc is to add null-termination
	_read_buffer_size = 4096;
	if (the_read_buffer == NULL)
	{
		_read_buffer_size = 0;
		SKINK_LOG("Stage 1: Could not acquire memory!");
		return -1;
	}
	// Create /proc entry
	the_proc_file_deleted = true;
	proc_entry = create_proc_entry(PROCFS_ENTRY, 0200, NULL);
	if (proc_entry == NULL)
	{
		SKINK_LOG("Stage 1: Could not create /proc entry!");
		remove_proc_entry(PROCFS_ENTRY, NULL);
		vfree(the_read_buffer);
		the_read_buffer = NULL;
		return -1;
	}
	the_proc_file_deleted = false;
	proc_entry->write_proc = _file_write;
	proc_entry->mode = S_IFREG | S_IWUGO;
	proc_entry->uid = 0;
	proc_entry->gid = 0;
	proc_entry->size = 0;
	// Read data and fill in the structures
	res = _get_data();
	// Free resources
	if (!the_proc_file_deleted)
	{
		remove_proc_entry(PROCFS_ENTRY, NULL);
		the_proc_file_deleted = true;
	}
	vfree(the_read_buffer);
	the_read_buffer = NULL;
	return res;
}

void free_data(void)
{
	if (the_layers)
		vfree(the_layers);
	if (the_modules)
		vfree(the_modules);
	if (the_patches)
		vfree(the_patches);
	the_layers = NULL;
	the_modules = NULL;
	the_patches = NULL;
	the_layers_count = 0;
}
