/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include <Python.h>
#include <pthread.h>
#include <skin.h>

#define define_struct(s)				\
typedef struct _skin_##s				\
{							\
	PyObject_HEAD					\
	skin_##s *orig;					\
	bool newed;					\
	bool was_shared;				\
} _skin_##s;

#define define_pyfree(s)						\
static void _skin_##s##_pyfree(void *self)				\
{									\
	_skin_##s *obj = (_skin_##s *)self;				\
	if (obj->orig)							\
	{								\
		if (obj->newed)						\
			delete obj->orig;				\
		obj->orig = NULL;					\
	}								\
}

#define define_lock_pyfree(s, f)					\
static void _skin_##s##_pyfree(void *self)				\
{									\
	_skin_##s *obj = (_skin_##s *)self;				\
	if (obj->orig && obj->newed)					\
	{								\
		if (obj->was_shared)					\
			skin_##f##_unshare_and_delete(obj->orig);	\
		else							\
			skin_##f##_delete(obj->orig);			\
	}								\
	obj->orig = NULL;						\
}

#define define_pynew(s)									\
static PyObject *_skin_##s##_pynew(PyTypeObject *type, PyObject *args, PyObject *kwds)	\
{											\
	_skin_##s *obj;									\
	obj = (_skin_##s *)type->tp_alloc(type, 0);					\
	if (obj == NULL)								\
		return PyErr_NoMemory();						\
	obj->orig = new skin_##s;							\
	if (obj->orig == NULL)								\
		return PyErr_NoMemory();						\
	obj->newed = true;								\
	return (PyObject *)obj;								\
}

/* helper names to increase regularity for easier macro invocation */
#define skin_rt_sem_get_shared(x, y) skin_rt_get_shared_semaphore(y)
#define skin_rt_mutex_get_shared(x, y) skin_rt_get_shared_mutex(y)
#define skin_rt_rwlock_get_shared(x, y) skin_rt_get_shared_rwlock(y)

#define lock_create(s, f, ws, wn, ...)							\
do											\
{											\
	_skin_##s *obj;									\
	obj = (_skin_##s *)type->tp_alloc(type, 0);					\
	if (obj == NULL)								\
		return PyErr_NoMemory();						\
	obj->orig = skin_##f(NULL, ##__VA_ARGS__);					\
	if (obj->orig == NULL)								\
		return PyErr_NoMemory();						\
	obj->was_shared = ws;								\
	obj->newed = wn;								\
	return (PyObject *)obj;								\
} while (0)

#define define_lock_pynew(s, f)								\
static PyObject *_skin_##s##_pynew(PyTypeObject *type, PyObject *args, PyObject *kwds)	\
{											\
	int param;									\
	const char *name;								\
	if (PyArg_ParseTuple(args, "is", &param, &name))				\
		lock_create(s, f##_init_and_share, true, true, param, name);		\
	else if (PyArg_ParseTuple(args, "s", &name))					\
		lock_create(s, f##_get_shared, true, false, name);			\
	else if (!PyArg_ParseTuple(args, "i", &param))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#s"([int], [const char *])");\
		return NULL;								\
	}										\
	else										\
		lock_create(s, f##_init, false, true, param);				\
}

#define define_pyinit(s)								\
static int _skin_##s##_pyinit(_skin_##s *obj, PyObject *args, PyObject *kwd)		\
{											\
	return 0;									\
}

#define define_pytype(s) static PyTypeObject _skin_##s##_pytype = {PyObject_HEAD_INIT(NULL)};

#define define_class(s)		\
define_struct(s)		\
define_pytype(s)		\
define_pyfree(s)		\
define_pynew(s)			\
define_pyinit(s)

#define define_lock(s, f)	\
define_struct(s)		\
define_pytype(s)		\
define_lock_pyfree(s, f)	\
define_lock_pynew(s, f)		\
define_pyinit(s)

/* initial definitions */
define_class(object)
define_class(reader)
define_struct(service)
define_pytype(service)

static void _skin_service_pyfree(void *self)
{
	_skin_service *obj = (_skin_service *)self;
	if (obj->orig)
	{
		obj->orig->disconnect();
		if (obj->newed)
			delete obj->orig;
		obj->orig = NULL;
	}
}

define_pynew(service)
define_pyinit(service)
define_class(service_manager)
define_class(sensor_type)
define_class(sensor)
define_class(module)
define_class(patch)
define_class(sub_region)
define_class(region)
define_class(task_statistics)
define_lock(rt_semaphore, rt_sem)
define_lock(rt_mutex, rt_mutex)

typedef struct _skin_rt_task
{
	PyObject_HEAD
	skin_rt_task_id id;
	skin_rt_task *orig;
	PyObject *callback;
	long callback_data;
	long callback_result;
} _skin_rt_task;

define_pytype(rt_task)

static void _skin_rt_task_pyfree(void *self)
{
	_skin_rt_task *obj = (_skin_rt_task *)self;
	if (obj->orig)
		skin_rt_user_task_delete(obj->orig);
	obj->orig = NULL;
	obj->id = 0;
	Py_XDECREF(obj->callback);
	obj->callback = NULL;
}

static PyObject *_skin_rt_task_pynew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	_skin_rt_task *obj;
	obj = (_skin_rt_task *)type->tp_alloc(type, 0);
	if (obj == NULL)
		return PyErr_NoMemory();
	obj->orig = NULL;
	obj->id = 0;
	obj->callback = NULL;
	return (PyObject *)obj;
}

define_pyinit(rt_task)
define_struct(rt_rwlock)
define_pytype(rt_rwlock)
define_lock_pyfree(rt_rwlock, rt_rwlock)

static PyObject *_skin_rt_rwlock_pynew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	int get_shared;
	const char *name;
	if (PyArg_ParseTuple(args, "si", &name, &get_shared))
		if (get_shared)
			lock_create(rt_rwlock, rt_rwlock_get_shared, true, false, name);
		else
			lock_create(rt_rwlock, rt_rwlock_init_and_share, true, true, name);
	else if (PyArg_ParseTuple(args, "s", &name))
		lock_create(rt_rwlock, rt_rwlock_init_and_share, true, true, name);
	else
		lock_create(rt_rwlock, rt_rwlock_init, false, true);
}

define_pyinit(rt_rwlock)

static PyObject *_exc_partial = NULL;
static PyObject *_exc_fail = NULL;
static PyObject *_exc_no_file = NULL;
static PyObject *_exc_no_mem = NULL;
static PyObject *_exc_file_incomplete = NULL;
static PyObject *_exc_file_parse_error = NULL;
static PyObject *_exc_file_invalid = NULL;
static PyObject *_exc_lock_timeout = NULL;
static PyObject *_exc_lock_not_acquired = NULL;
static PyObject *_exc_bad_data = NULL;
static PyObject *_exc_bad_name = NULL;
static PyObject *_exc_bad_id = NULL;

static PyObject *_exc_rt_fail = NULL;
static PyObject *_exc_rt_invalid = NULL;
static PyObject *_exc_rt_no_mem = NULL;
static PyObject *_exc_rt_lock_timeout = NULL;
static PyObject *_exc_rt_sync_mech_error = NULL;
static PyObject *_exc_rt_lock_not_acquired = NULL;

static int _set_exception(int err)
{
	switch (err)
	{
	case SKIN_PARTIAL:
		PyErr_SetString(_exc_partial, "Operation was only partially successful");
		break;
	case SKIN_FAIL:
		PyErr_SetString(_exc_fail, "Operation failed");
		break;
	case SKIN_NO_FILE:
		PyErr_SetString(_exc_no_file, "No such file");
		break;
	case SKIN_NO_MEM:
		PyErr_SetString(_exc_no_mem, "Out of memory");
		break;
	case SKIN_FILE_INCOMPLETE:
		PyErr_SetString(_exc_file_incomplete, "Unexpected end of file");
		break;
	case SKIN_FILE_PARSE_ERROR:
		PyErr_SetString(_exc_file_parse_error, "Parse error");
		break;
	case SKIN_FILE_INVALID:
		PyErr_SetString(_exc_file_invalid, "Mismatch between file and skin");
		break;
	case SKIN_LOCK_TIMEOUT:
		PyErr_SetString(_exc_lock_timeout, "Lock timed out");
		break;
	case SKIN_LOCK_NOT_ACQUIRED:
		PyErr_SetString(_exc_lock_not_acquired, "Lock not acquired");
		break;
	case SKIN_BAD_DATA:
		PyErr_SetString(_exc_bad_data, "Bad data as argument");
		break;
	case SKIN_BAD_NAME:
		PyErr_SetString(_exc_bad_name, "Bad name as argument");
		break;
	case SKIN_BAD_ID:
		PyErr_SetString(_exc_bad_id, "Bad id as argument");
		break;
	default:
		break;
	}
	return err == SKIN_SUCCESS?0:-1;
}

static int _set_rt_exception(int err)
{
	switch (err)
	{
	case SKIN_RT_FAIL:
		PyErr_SetString(_exc_rt_fail, "Operation failed");
		break;
	case SKIN_RT_NO_MEM:
		PyErr_SetString(_exc_rt_no_mem, "Out of memory");
		break;
	case SKIN_RT_INVALID:
		PyErr_SetString(_exc_rt_invalid, "Invalid operation");
		break;
	case SKIN_RT_TIMEOUT:
		PyErr_SetString(_exc_rt_lock_timeout, "Lock timed out");
		break;
	case SKIN_RT_SYNC_MECHANISM_ERROR:
		PyErr_SetString(_exc_rt_sync_mech_error, "Error in synchronization");
		break;
	case SKIN_RT_LOCK_NOT_ACQUIRED:
		PyErr_SetString(_exc_rt_lock_not_acquired, "Lock not acquired");
		break;
	default:
		break;
	}
	return err == SKIN_SUCCESS?0:-1;
}

#define define_func_void(s, f)								\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	obj->orig->f();									\
	Py_RETURN_NONE;									\
}

#define define_func_noarg(s, f)								\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	int ret = obj->orig->f();							\
	if (_set_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_func_onearg(s, f, t, format)						\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t")");			\
		return NULL;								\
	}										\
	int ret = obj->orig->f(a);							\
	if (_set_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_func_twoarg(s, f, t1, t2, format)					\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t1 a;										\
	t2 b;										\
	if (!PyArg_ParseTuple(args, format, &a, &b))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t1", "#t2")");		\
		return NULL;								\
	}										\
	int ret = obj->orig->f(a, b);							\
	if (_set_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_func_twoarg_obj_not(s, f, t1, t2, format)				\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t1 a;										\
	t2 b;										\
	if (!PyArg_ParseTuple(args, format, &a, &b))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t1", "#t2")");		\
		return NULL;								\
	}										\
	int ret = obj->orig->f(a->orig, b);						\
	if (_set_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_func_threearg_detailed(s, f, t1, at1, t2, at2, t3, at3, format)		\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t1 a;										\
	t2 b;										\
	t3 c;										\
	if (!PyArg_ParseTuple(args, format, &a, &b, &c))				\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t1", "#t2", "#t3")");	\
		return NULL;								\
	}										\
	int ret = obj->orig->f((at1)a, (at2)b, (at3)c);					\
	if (_set_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_func_threearg(s, f, t1, t2, t3, format)					\
	define_func_threearg_detailed(s, f, t1, t1, t2, t2, t3, t3, format)

#define define_func_getone(s, f, t)							\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	_skin_##t *field;								\
	field = (_skin_##t *)_skin_##t##_pytype.tp_alloc(&_skin_##t##_pytype, 0);	\
	if (field == NULL)								\
		return PyErr_NoMemory();						\
	field->orig = obj->orig->f();							\
	return Py_BuildValue("O", field);						\
}

#define define_func_getpointer_notfunc(s, f)						\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	return Py_BuildValue("O", PyCObject_FromVoidPtr(obj->orig->f, NULL));		\
}

#define define_func_onearg_getbool(s, f, t, format)					\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t")");			\
		return NULL;								\
	}										\
	int ret = obj->orig->f(a);							\
	if (ret < 0)									\
	{										\
		_set_exception(ret);							\
		return NULL;								\
	}										\
	return Py_BuildValue("i", ret);							\
}

#define define_func_getnum(s, f)							\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	if (sizeof(obj->orig->f()) <= 4)						\
		return Py_BuildValue("i", obj->orig->f());				\
	else										\
		return Py_BuildValue("l", (unsigned long long)obj->orig->f());		\
}

#define define_func_getnum_notfunc(s, f)						\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	if (sizeof(obj->orig->f) <= 4)							\
		return Py_BuildValue("i", obj->orig->f);				\
	else										\
		return Py_BuildValue("l", (unsigned long long)obj->orig->f);		\
}

#define define_func_getstr(s, f)							\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	return Py_BuildValue("s", obj->orig->f());					\
}

#define define_func_getstr_notfunc(s, f)						\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	return Py_BuildValue("s", obj->orig->f);					\
}

#define define_func_getlist_detailed(s, f, c, fp, t, idt, sizet)			\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	skin_##t *array;								\
	_skin_##t *item;								\
	PyObject *list;									\
	idt i;										\
	sizet count = c;								\
	list = PyList_New(0);								\
	if (list == NULL)								\
		return PyErr_NoMemory();						\
	array = obj->orig->fp;								\
	for (i = 0; i < count; ++i)							\
	{										\
		PyObject *tmp;								\
		item = (_skin_##t *)_skin_##t##_pytype.tp_alloc(&_skin_##t##_pytype, 0);\
		if (item == NULL)							\
			goto exit_no_mem;						\
		item->orig = &array[i];							\
		tmp = Py_BuildValue("O", item);						\
		if (tmp == NULL)							\
		{									\
			Py_DECREF(item);						\
			goto exit_no_mem;						\
		}									\
		PyList_Append(list, tmp);						\
		Py_DECREF(tmp);								\
	}										\
	return list;									\
exit_no_mem:										\
	Py_DECREF(list);								\
	return PyErr_NoMemory();							\
}

#define define_func_getlist(s, f, t)							\
	define_func_getlist_detailed(s, f, 0, f(&count), t, skin_##t##_id, skin_##t##_size)

#define define_func_getlist_notfunc(s, f, c, t, ct)					\
	define_func_getlist_detailed(s, f, obj->orig->c, f, t, skin_##ct##_id, skin_##ct##_size)

#define define_func_getlist_nums_detailed(s, f, c, fp, t, idt, sizet)			\
static PyObject *_skin_##s##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	skin_##t *array;								\
	PyObject *item;									\
	PyObject *list;									\
	idt i;										\
	sizet count = c;								\
	list = PyList_New(0);								\
	if (list == NULL)								\
		return PyErr_NoMemory();						\
	array = obj->orig->fp;								\
	for (i = 0; i < count; ++i)							\
	{										\
		if (sizeof(array[i]) <= 4)						\
			item = Py_BuildValue("i", array[i]);				\
		else									\
			item = Py_BuildValue("l", (unsigned long long)array[i]);	\
		if (item == NULL)							\
			goto exit_no_mem;						\
		PyList_Append(list, item);						\
		Py_DECREF(item);							\
	}										\
	return list;									\
exit_no_mem:										\
	Py_DECREF(list);								\
	return PyErr_NoMemory();							\
}

#define define_func_getlist_nums(s, f, t, ct)						\
	define_func_getlist_nums_detailed(s, f, 0, f(&count), t, skin_##ct##_id, skin_##ct##_size)

#define define_func_getlist_nums_notfunc(s, f, c, t, ct)				\
	define_func_getlist_nums_detailed(s, f, obj->orig->c, f, t, skin_##ct##_id, skin_##ct##_size)

#define define_servicefunc(f, F)							\
static PyObject *_skin_service_##f(_skin_service *obj, PyObject *args)			\
{											\
	return Py_BuildValue("l", SKIN_SERVICE_##F(obj->orig->memory));			\
}

#define define_smfunc_fivearg(f, t1, t2, t3, t4, t5, format)				\
static PyObject *_skin_service_manager_##f(_skin_service_manager *obj, PyObject *args)	\
{											\
	t1 a;										\
	t2 b;										\
	t3 c;										\
	t4 d;										\
	t5 e;										\
	if (!PyArg_ParseTuple(args, format, &a, &b, &c, &d, &e))			\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t1", "#t2", "#t3", "#t4", "#t5")");	\
		return NULL;								\
	}										\
	int ret = obj->orig->f(a, b, c, d, e);						\
	if (ret < 0)									\
	{										\
		_set_exception(ret);							\
		return NULL;								\
	}										\
	return Py_BuildValue("i", ret);							\
}

#define define_sensorfunc_3d(f)								\
static PyObject *_skin_sensor_##f(_skin_sensor *obj, PyObject *args)			\
{											\
	return Py_BuildValue("fff", obj->orig->f[0], obj->orig->f[1], obj->orig->f[2]);	\
}											\

#define define_sensorfunc_set_3d(f)							\
static PyObject *_skin_sensor_set_##f(_skin_sensor *obj, PyObject *args)		\
{											\
	float v[3];									\
	if (!PyArg_ParseTuple(args, "fff", &v[0], &v[1], &v[2]))			\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: set_"#f"(float, float, float)");	\
		return NULL;								\
	}										\
	obj->orig->f[0] = v[0];								\
	obj->orig->f[1] = v[1];								\
	obj->orig->f[2] = v[2];								\
	Py_RETURN_NONE;									\
}

#define define_sensorfunc_2d(f)								\
static PyObject *_skin_sensor_##f(_skin_sensor *obj, PyObject *args)			\
{											\
	return Py_BuildValue("ff", obj->orig->f[0], obj->orig->f[1]);			\
}											\

#define define_sensorfunc_num(f, format)						\
static PyObject *_skin_sensor_##f(_skin_sensor *obj, PyObject *args)			\
{											\
	return Py_BuildValue(format, obj->orig->f);					\
}

#define define_sensorfunc_setnum(f, t, format)						\
static PyObject *_skin_sensor_set_##f(_skin_sensor *obj, PyObject *args)		\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: set_"#f"("#t")");		\
		return NULL;								\
	}										\
	obj->orig->f = a;								\
	Py_RETURN_NONE;									\
}

#define define_gfunc_void(f)								\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	skin_##f();									\
	Py_RETURN_NONE;									\
}

#define define_gfunc_noarg(f)								\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	int ret = skin_##f();								\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_gfunc_getstr(f, max_size)						\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	char tmp[max_size + 1];								\
	int ret = skin_##f(tmp);							\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	tmp[max_size] = '\0';								\
	return Py_BuildValue("s", tmp);							\
}

#define define_gfunc_getnum(f)								\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	if (sizeof(skin_##f()) <= 4)							\
		return Py_BuildValue("i", skin_##f());					\
	else										\
		return Py_BuildValue("l", (unsigned long long)skin_##f());		\
}

#define define_gfunc_onearg_getbool(f, t, format)					\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t")");			\
		return NULL;								\
	}										\
	return Py_BuildValue("i", skin_##f(a));						\
}

#define define_gfunc_onearg_void(f, t, format)						\
static PyObject *_skin_##f(PyObject *unused, PyObject *args)				\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t")");			\
		return NULL;								\
	}										\
	skin_##f(a);									\
	Py_RETURN_NONE;									\
}

#define define_taskfunc_void(f)								\
static PyObject *_skin_rt_task_##f(_skin_rt_task *obj, PyObject *args)			\
{											\
	skin_rt_user_task_##f();							\
	Py_RETURN_NONE;									\
}

#define define_taskfunc_noarg_byfield(f, field)						\
static PyObject *_skin_rt_task_##f(_skin_rt_task *obj, PyObject *args)			\
{											\
	int ret = skin_rt_user_task_##f(obj->field);					\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_taskfunc_twoarg(f, field, t1, t2, format)				\
static PyObject *_skin_rt_task_##f(_skin_rt_task *obj, PyObject *args)			\
{											\
	t1 a;										\
	t2 b;										\
	if (!PyArg_ParseTuple(args, format, &a, &b))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t1", "#t2")");		\
		return NULL;								\
	}										\
	int ret = skin_rt_user_task_##f(obj->field, a, b);				\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_taskfunc_noarg(f)							\
static PyObject *_skin_rt_task_##f(_skin_rt_task *obj, PyObject *args)			\
{											\
	int ret = skin_rt_user_task_##f();						\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_lockfunc_void(s, sh, f)							\
static PyObject *_skin_##sh##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	skin_##sh##_##f(obj->orig);							\
	Py_RETURN_NONE;									\
}

#define define_lockfunc_noarg(s, sh, f)							\
static PyObject *_skin_##sh##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	int ret = skin_##sh##_##f(obj->orig);						\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

#define define_lockfunc_onearg(s, sh, f, t, format)					\
static PyObject *_skin_##sh##_##f(_skin_##s *obj, PyObject *args)			\
{											\
	t a;										\
	if (!PyArg_ParseTuple(args, format, &a))					\
	{										\
		PyErr_SetString(PyExc_ValueError, "Usage: "#f"("#t")");			\
		return NULL;								\
	}										\
	int ret = skin_##sh##_##f(obj->orig, a);					\
	if (_set_rt_exception(ret))							\
		return NULL;								\
	Py_RETURN_NONE;									\
}

/* skin_object functions */
define_func_noarg(object, load)
define_func_noarg(object, calibration_begin)
define_func_onearg(object, calibration_end, const char *, "s")
define_func_onearg(object, calibration_reload, const char *, "s")
define_func_noarg(object, regionalization_begin)
/* skin_object_regionalization_end hand-made */
define_func_onearg(object, regionalization_reload, const char *, "s")
define_func_getone(object, reader, reader)
define_func_getone(object, service_manager, service_manager)
define_func_getlist(object, sensor_types, sensor_type)
define_func_getlist(object, sensors, sensor)
define_func_getlist(object, modules, module)
define_func_getlist(object, patches, patch)
define_func_getlist(object, sub_regions, sub_region)
define_func_getlist(object, regions, region)
define_func_getlist_nums(object, sub_region_indices, sub_region_id, sub_region_index)
define_func_getlist_nums(object, region_indices, region_id, region_index)
define_func_getnum(object, sensor_types_count)
define_func_getnum(object, sensors_count)
define_func_getnum(object, modules_count)
define_func_getnum(object, patches_count)
define_func_getnum(object, sub_regions_count)
define_func_getnum(object, regions_count)
define_func_getnum(object, sub_region_indices_count)
define_func_getnum(object, region_indices_count)
define_func_getstr(object, last_error)
define_func_getnum(object, time_diff_with_kernel)

/* skin_reader */
define_func_threearg_detailed(reader, start, unsigned int, skin_sensor_type_id,
		unsigned int, skin_acquisition_type, unsigned long long, skin_rt_time, "IIK")
define_func_onearg(reader, request, unsigned int, "I")
define_func_onearg(reader, request_nonblocking, unsigned int, "I")
define_func_onearg(reader, await_response, unsigned int, "I")
define_func_onearg(reader, stop, unsigned int, "I")
define_func_onearg(reader, pause, unsigned int, "I")
define_func_onearg(reader, resume, unsigned int, "I")
define_func_onearg_getbool(reader, is_paused, unsigned int, "I")
define_func_twoarg_obj_not(reader, register_user, _skin_rt_mutex *, unsigned int, "OI")
define_func_twoarg_obj_not(reader, wait_read, _skin_rt_mutex *, unsigned int, "OI")
define_func_twoarg_obj_not(reader, unregister_user, _skin_rt_mutex *, unsigned int, "OI")
define_func_void(reader, enable_swap_skip_prediction)
define_func_void(reader, disable_swap_skip_prediction)
define_func_getnum_notfunc(reader, tasks_count)
define_func_getlist_notfunc(reader, tasks_statistics, tasks_count, task_statistics, sensor_type)

/* skin_service */
define_func_noarg(service, lock)
define_func_noarg(service, unlock)
define_func_noarg(service, request)
define_func_noarg(service, request_nonblocking)
define_func_noarg(service, await_response)
define_func_void(service, disconnect)
define_func_getpointer_notfunc(service, memory)
define_servicefunc(period, PERIOD)
define_servicefunc(timestamp, TIMESTAMP)
define_servicefunc(mode, MODE)
define_servicefunc(is_periodic, IS_PERIODIC)
define_servicefunc(is_sporadic, IS_SPORADIC)
define_servicefunc(result_count, RESULT_COUNT)
define_servicefunc(mem_size, MEM_SIZE)
define_servicefunc(status, STATUS)
define_servicefunc(is_alive, IS_ALIVE)
define_servicefunc(is_dead, IS_DEAD)

/* skin_service_manager */
define_smfunc_fivearg(initialize_periodic_service, const char *, unsigned int, unsigned int, const char *, unsigned long long, "sIIsK")
define_smfunc_fivearg(initialize_sporadic_service, const char *, unsigned int, unsigned int, const char *, const char *, "sIIss")
/* skin_service_manager_start_service hand-made */
define_func_onearg(service_manager, pause_service, unsigned int, "I")
define_func_onearg(service_manager, resume_service, unsigned int, "I")
define_func_onearg(service_manager, stop_service, unsigned int, "I")
/* skin_service_manager_connect_to_periodic_service hand-made */
/* skin_service_manager_connect_to_sporadic_service hand-made */

/* skin_sensor_type */
define_func_getlist(sensor_type, sensors, sensor)
define_func_getlist(sensor_type, modules, module)
define_func_getlist(sensor_type, patches, patch)
define_func_getnum(sensor_type, sensors_count)
define_func_getnum(sensor_type, modules_count)
define_func_getnum(sensor_type, patches_count)
define_func_getnum_notfunc(sensor_type, id)
define_func_getstr_notfunc(sensor_type, name)
define_func_getnum_notfunc(sensor_type, is_active)

/* skin_sensor */
define_func_getnum(sensor, get_response)
define_func_getone(sensor, sensor_type, sensor_type)
define_func_getone(sensor, module, module)
define_func_getone(sensor, patch, patch)
define_func_getone(sensor, sub_region, sub_region)
define_func_getnum_notfunc(sensor, id)
define_sensorfunc_3d(relative_position)
define_sensorfunc_3d(relative_orientation)
define_sensorfunc_set_3d(relative_position)
define_sensorfunc_set_3d(relative_orientation)
define_sensorfunc_3d(global_position)
define_sensorfunc_3d(global_orientation)
define_sensorfunc_2d(flattened_position)
define_sensorfunc_num(radius, "f")
define_func_getlist_nums_notfunc(sensor, neighbors, neighbors_count, sensor_id, sensor)
define_func_getnum_notfunc(sensor, robot_link)
define_sensorfunc_setnum(robot_link, unsigned int, "I")

/* skin_module */
define_func_getlist(module, sensors, sensor)
define_func_getnum(module, sensors_count)
define_func_getone(module, patch, patch)
define_func_getone(module, sensor_type, sensor_type)
define_func_getnum_notfunc(module, id)

/* skin_patch */
define_func_getlist(patch, modules, module)
define_func_getnum(patch, sensors_count)
define_func_getnum(patch, modules_count)
define_func_getone(patch, sensor_type, sensor_type)
define_func_getnum_notfunc(patch, id)

/* skin_sub_region */
/* skin_sub_region_sensors hand-made */
define_func_getlist_nums(sub_region, region_indices, region_id, region_index)
define_func_getnum(sub_region, sensor_types_count)
define_func_getnum(sub_region, region_indices_count)
define_func_getnum_notfunc(sub_region, id)

/* skin_region */
define_func_getlist_nums(region, sub_region_indices, sub_region_id, sub_region_index)
define_func_getnum(region, sub_region_indices_count)
define_func_getnum_notfunc(region, id)


/* skin_task_statistics */
define_func_getnum_notfunc(task_statistics, worst_read_time)
define_func_getnum_notfunc(task_statistics, number_of_reads)
define_func_getnum_notfunc(task_statistics, best_read_time)
define_func_getnum_notfunc(task_statistics, accumulated_read_time)

/* skin_rt functions */
define_gfunc_getstr(rt_get_free_name, SKIN_RT_MAX_NAME_LENGTH)
define_gfunc_onearg_getbool(rt_priority_is_valid, int, "i")
define_gfunc_noarg(rt_init)
define_gfunc_void(rt_stop)
/* skin_rt_get_task hand-made */
define_gfunc_getnum(rt_is_rt_context)
define_gfunc_getnum(rt_get_time)
define_gfunc_getnum(rt_get_exectime)
define_gfunc_onearg_void(rt_sleep, unsigned long long, "K")
define_gfunc_onearg_void(rt_sleep_until, unsigned long long, "K")
/* TODO: skin_rt_shared_memory* */
define_gfunc_onearg_getbool(rt_name_available, const char *, "s")

/* skin_rt_task functions */
/* skin_rt_user_task_init hand-made */
define_taskfunc_noarg_byfield(join, id)
define_taskfunc_noarg_byfield(resume, orig)
define_taskfunc_noarg_byfield(suspend, orig)
define_taskfunc_void(wait_period)
define_taskfunc_twoarg(make_periodic, orig, skin_rt_time, skin_rt_time, "KK")
define_taskfunc_twoarg(make_periodic_relative, orig, skin_rt_time, skin_rt_time, "KK")
define_taskfunc_noarg_byfield(delete, orig)
define_taskfunc_void(make_hard_real_time)
define_taskfunc_void(make_soft_real_time)
/* skin_rt_user_task_on_start hand-made */
define_taskfunc_noarg(on_stop)

/* skin_rt_sem functions */
define_lockfunc_noarg(rt_semaphore, rt_sem, wait)
define_lockfunc_noarg(rt_semaphore, rt_sem, try_wait)
define_lockfunc_onearg(rt_semaphore, rt_sem, timed_wait, unsigned long long, "K")
define_lockfunc_noarg(rt_semaphore, rt_sem, post)
define_lockfunc_noarg(rt_semaphore, rt_sem, broadcast)

/* skin_rt_mutex functions */
define_lockfunc_noarg(rt_mutex, rt_mutex, lock)
define_lockfunc_noarg(rt_mutex, rt_mutex, try_lock)
define_lockfunc_onearg(rt_mutex, rt_mutex, timed_lock, unsigned long long, "K")
define_lockfunc_noarg(rt_mutex, rt_mutex, unlock)
define_lockfunc_noarg(rt_mutex, rt_mutex, broadcast)

/* skin_rt_rwlock functions */
define_lockfunc_noarg(rt_rwlock, rt_rwlock, read_lock)
define_lockfunc_noarg(rt_rwlock, rt_rwlock, try_read_lock)
define_lockfunc_onearg(rt_rwlock, rt_rwlock, timed_read_lock, unsigned long long, "K")
define_lockfunc_void(rt_rwlock, rt_rwlock, read_unlock)
define_lockfunc_noarg(rt_rwlock, rt_rwlock, write_lock)
define_lockfunc_noarg(rt_rwlock, rt_rwlock, try_write_lock)
define_lockfunc_onearg(rt_rwlock, rt_rwlock, timed_write_lock, unsigned long long, "K")
define_lockfunc_void(rt_rwlock, rt_rwlock, write_unlock)

/* hand-made functions */
static PyObject *_skin_object_regionalization_end(_skin_object *obj, PyObject *args)
{
	PyObject *regions_list;
	const char *cache_file;
	skin_regionalization_data *regions = NULL;
	skin_region_size regions_count = 0;
	int ret;

	if (!PyArg_ParseTuple(args, "Os", &regions_list, &cache_file))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: regionalization_end(list<set<module_id>>, const char *)");
		return NULL;
	}

	regions_count = PyList_Size(regions_list);
	if (regions_count > 0)
	{
		regions = new skin_regionalization_data[regions_count];
		if (regions == NULL)
			goto exit_no_mem;
		for (skin_region_id i = 0; i < regions_count; ++i)
		{
			PyObject *region;
			PyObject *iter;
			PyObject *item;
			skin_module_size count;

			regions[i].modules = NULL;
			regions[i].size = 0;
			region = PyList_GetItem(regions_list, i);
			if (region == NULL)
				goto exit_no_mem;
			count = PySet_Size(region);
			if (count == 0)
				continue;

			regions[i].modules = new skin_module_id[count];
			if (regions[i].modules == NULL)
				goto exit_no_mem;

			iter = PyObject_GetIter(region);
			if (iter == NULL)
				continue;
			while ((item = PyIter_Next(iter)))
			{
				regions[i].modules[regions[i].size++] = PyInt_AsLong(item);
				Py_DECREF(item);
			}
			Py_DECREF(iter);
		}
	}

	ret = obj->orig->regionalization_end(regions, regions_count, cache_file);
	if (_set_exception(ret))
		return NULL;
	for (skin_region_id i = 0; i < regions_count; ++i)
		delete[] regions[i].modules;
	Py_RETURN_NONE;
exit_no_mem:
	if (regions)
	{
		for (skin_region_id i = 0; i < regions_count; ++i)
			if (regions[i].modules)
				delete[] regions[i].modules;
		delete[] regions;
	}
	return PyErr_NoMemory();
}

static void _service_callback_calling_python(void *mem, void *data)
{
	PyGILState_STATE gstate;

	gstate = PyGILState_Ensure();

	PyObject *arg = PyCObject_FromVoidPtr(mem, NULL);
	PyObject *args = Py_BuildValue("(O)", arg);
	PyObject *callback = (PyObject *)data;

	PyObject *res = PyObject_CallObject(callback, args);

	Py_DECREF(arg);
	Py_DECREF(args);
	Py_XDECREF(res);
	Py_DECREF(callback);

	PyGILState_Release(gstate);
}

static PyObject *_skin_service_manager_start_service(_skin_service_manager *obj, PyObject *args)
{
	PyObject *callback;
	unsigned int id;
	if (!PyArg_ParseTuple(args, "IO", &id, &callback))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: start_service(unsigned int, void (*f)(void *))");
		return NULL;
	}
	if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: start_service(unsigned int, void (*f)(void *))");
		return NULL;
	}
	Py_XINCREF(callback);
	int ret = obj->orig->start_service(id, _service_callback_calling_python, callback);
	if (_set_exception(ret))
		return NULL;
	Py_RETURN_NONE;
}

static PyObject *_skin_service_manager_connect_to_periodic_service(_skin_service_manager *obj, PyObject *args)
{
	const char *service_name;
	const char *rwl_name;
	_skin_service *service;
	if (!PyArg_ParseTuple(args, "ss", &service_name, &rwl_name))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: connect_to_periodic_service(const char *, const char *)");
		return NULL;
	}
	service = (_skin_service *)_skin_service_pynew(&_skin_service_pytype, NULL, NULL);
	int ret = obj->orig->connect_to_periodic_service(service_name, rwl_name, service->orig);
	if (_set_exception(ret))
	{
		Py_DECREF(service);
		return NULL;
	}
	return Py_BuildValue("O", service);
}

static PyObject *_skin_service_manager_connect_to_sporadic_service(_skin_service_manager *obj, PyObject *args)
{
	const char *service_name;
	const char *request_name;
	const char *response_name;
	_skin_service *service;
	if (!PyArg_ParseTuple(args, "sss", &service_name, &request_name, &response_name))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: connect_to_sporadic_service(const char *, const char *, const char *)");
		return NULL;
	}
	service = (_skin_service *)_skin_service_pynew(&_skin_service_pytype, NULL, NULL);
	int ret = obj->orig->connect_to_sporadic_service(service_name, request_name, response_name, service->orig);
	if (_set_exception(ret))
	{
		Py_DECREF(service);
		return NULL;
	}
	return Py_BuildValue("O", service);
}

static PyObject *_skin_sub_region_sensors(_skin_sub_region *obj, PyObject *args)
{
	skin_sensor **array;
	skin_sensor_size *counts;
	skin_sensor_type_size st_count;
	PyObject *list;
	list = PyList_New(0);
	if (list == NULL)
		return PyErr_NoMemory();

	st_count = obj->orig->sensor_types_count();
	array = new skin_sensor *[st_count];
	counts = new skin_sensor_size[st_count];
	if (array == NULL || counts == NULL)
		goto exit_no_mem;

	obj->orig->sensors(array, counts);
	for (skin_sensor_type_id i = 0; i < st_count; ++i)
	{
		PyObject *inner_list;

		inner_list = PyList_New(0);
		if (inner_list == NULL)
			goto exit_no_mem;
		PyList_Append(list, inner_list);
		Py_DECREF(inner_list);

		for (skin_sensor_id j = 0; j < counts[i]; ++j)
		{
			PyObject *tmp;
			_skin_sensor *item = (_skin_sensor *)_skin_sensor_pytype.tp_alloc(&_skin_sensor_pytype, 0);
			if (item == NULL)
				goto exit_no_mem;
			item->orig = &array[i][j];
			tmp = Py_BuildValue("O", item);
			if (tmp == NULL)
			{
				Py_DECREF(item);
				goto exit_no_mem;
			}
			PyList_Append(inner_list, tmp);
			Py_DECREF(tmp);
		}
	}

	delete[] array;
	delete[] counts;
	return list;
exit_no_mem:
	Py_DECREF(list);
	if (array)
		delete[] array;
	if (counts)
		delete[] counts;
	return PyErr_NoMemory();
}

static PyObject *_skin_rt_get_task(PyObject *unused, PyObject *args)
{
	_skin_rt_task *obj;

	obj = (_skin_rt_task *)_skin_rt_task_pytype.tp_alloc(&_skin_rt_task_pytype, 0);
	if (obj == NULL)
		return PyErr_NoMemory();
	obj->orig = skin_rt_get_task();
	obj->id = pthread_self();
	obj->callback = NULL;
	return Py_BuildValue("O", obj);
}

static void *_rt_callback_calling_python(void *p)
{
	PyGILState_STATE gstate;

	gstate = PyGILState_Ensure();

	_skin_rt_task *task = (_skin_rt_task *)p;
	PyObject *args;
	PyObject *res;

	args = Py_BuildValue("(l)", task->callback_data);
	res = PyObject_CallObject(task->callback, args);
	Py_DECREF(args);
	if (res == NULL)
		return NULL;
	PyArg_ParseTuple(res, "l", &task->callback_result);
	Py_DECREF(res);

	PyGILState_Release(gstate);
	return (void *)task->callback_result;
}

static PyObject *_skin_rt_task_init(_skin_rt_task *obj, PyObject *args)
{
	PyObject *callback;
	long int data;
	int stack_size;
	if (!PyArg_ParseTuple(args, "Oli", &callback, &data, &stack_size))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: init(long (*f)(long), long, int)");
		return NULL;
	}
	if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: init(long (*f)(long), long, int)");
		return NULL;
	}
	Py_XINCREF(callback);
	Py_XDECREF(obj->callback);
	obj->callback = callback;
	obj->callback_data = data;
	obj->id = skin_rt_user_task_init(_rt_callback_calling_python, obj, stack_size);
	if (obj->id == 0)
	{
		_set_rt_exception(obj->id);
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *_skin_rt_task_on_start(_skin_rt_task *obj, PyObject *args)
{
	int priority;
	if (!PyArg_ParseTuple(args, "i", &priority))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: on_start(int)");
		return NULL;
	}
	obj->orig = skin_rt_user_task_on_start(priority);
	if (obj->orig == NULL)
	{
		_set_rt_exception(SKIN_RT_FAIL);
		return NULL;
	}
	Py_RETURN_NONE;
}

#define define_method(s, f) {#f, (PyCFunction)_skin_##s##_##f, METH_VARARGS, "Wrapper for skin_"#s"_"#f}
#define define_gmethod(f) {#f, (PyCFunction)_skin_##f, METH_VARARGS, "Wrapper for skin_"#f}
#define define_taskmethod(f) {#f, (PyCFunction)_skin_rt_task_##f, METH_VARARGS, "Wrapper for skin_rt_user_task_"#f}

static PyMethodDef _skin_object_methods[] = {
	define_method(object, load),
	define_method(object, calibration_begin),
	define_method(object, calibration_end),
	define_method(object, calibration_reload),
	define_method(object, regionalization_begin),
	define_method(object, regionalization_end),
	define_method(object, regionalization_reload),
	define_method(object, reader),
	define_method(object, service_manager),
	define_method(object, sensor_types),
	define_method(object, sensors),
	define_method(object, modules),
	define_method(object, patches),
	define_method(object, sub_regions),
	define_method(object, regions),
	define_method(object, sub_region_indices),
	define_method(object, region_indices),
	define_method(object, sensor_types_count),
	define_method(object, sensors_count),
	define_method(object, modules_count),
	define_method(object, patches_count),
	define_method(object, sub_regions_count),
	define_method(object, regions_count),
	define_method(object, sub_region_indices_count),
	define_method(object, region_indices_count),
	define_method(object, last_error),
	define_method(object, time_diff_with_kernel),
	{0}
};

static PyMethodDef _skin_reader_methods[] = {
	define_method(reader, start),
	define_method(reader, request),
	define_method(reader, request_nonblocking),
	define_method(reader, await_response),
	define_method(reader, stop),
	define_method(reader, pause),
	define_method(reader, resume),
	define_method(reader, is_paused),
	define_method(reader, register_user),
	define_method(reader, wait_read),
	define_method(reader, unregister_user),
	define_method(reader, enable_swap_skip_prediction),
	define_method(reader, disable_swap_skip_prediction),
	define_method(reader, tasks_count),
	define_method(reader, tasks_statistics),
	{0}
};

static PyMethodDef _skin_service_methods[] = {
	define_method(service, lock),
	define_method(service, unlock),
	define_method(service, request),
	define_method(service, request_nonblocking),
	define_method(service, await_response),
	define_method(service, disconnect),
	define_method(service, memory),
	define_method(service, period),
	define_method(service, timestamp),
	define_method(service, mode),
	define_method(service, is_periodic),
	define_method(service, is_sporadic),
	define_method(service, result_count),
	define_method(service, mem_size),
	define_method(service, status),
	define_method(service, is_alive),
	define_method(service, is_dead),
	{0}
};

static PyMethodDef _skin_service_manager_methods[] = {
	define_method(service_manager, initialize_periodic_service),
	define_method(service_manager, initialize_sporadic_service),
	define_method(service_manager, start_service),
	define_method(service_manager, pause_service),
	define_method(service_manager, resume_service),
	define_method(service_manager, stop_service),
	define_method(service_manager, connect_to_periodic_service),
	define_method(service_manager, connect_to_sporadic_service),
	{0}
};

static PyMethodDef _skin_sensor_type_methods[] = {
	define_method(sensor_type, sensors),
	define_method(sensor_type, modules),
	define_method(sensor_type, patches),
	define_method(sensor_type, sensors_count),
	define_method(sensor_type, modules_count),
	define_method(sensor_type, patches_count),
	define_method(sensor_type, id),
	define_method(sensor_type, name),
	define_method(sensor_type, is_active),
	{0}
};

static PyMethodDef _skin_sensor_methods[] = {
	define_method(sensor, get_response),
	define_method(sensor, sensor_type),
	define_method(sensor, module),
	define_method(sensor, patch),
	define_method(sensor, sub_region),
	define_method(sensor, id),
	define_method(sensor, relative_position),
	define_method(sensor, relative_orientation),
	define_method(sensor, set_relative_position),
	define_method(sensor, set_relative_orientation),
	define_method(sensor, global_position),
	define_method(sensor, global_orientation),
	define_method(sensor, flattened_position),
	define_method(sensor, radius),
	define_method(sensor, neighbors),
	define_method(sensor, robot_link),
	define_method(sensor, set_robot_link),
	{0}
};

static PyMethodDef _skin_module_methods[] = {
	define_method(module, sensors),
	define_method(module, sensors_count),
	define_method(module, patch),
	define_method(module, sensor_type),
	define_method(module, id),
	{0}
};

static PyMethodDef _skin_patch_methods[] = {
	define_method(patch, modules),
	define_method(patch, sensors_count),
	define_method(patch, modules_count),
	define_method(patch, sensor_type),
	define_method(patch, id),
	{0}
};

static PyMethodDef _skin_sub_region_methods[] = {
	define_method(sub_region, sensors),
	define_method(sub_region, region_indices),
	define_method(sub_region, sensor_types_count),
	define_method(sub_region, region_indices_count),
	define_method(sub_region, id),
	{0}
};

static PyMethodDef _skin_region_methods[] = {
	define_method(region, sub_region_indices),
	define_method(region, sub_region_indices_count),
	define_method(region, id),
	{0}
};

static PyMethodDef _skin_task_statistics_methods[] = {
	define_method(task_statistics, worst_read_time),
	define_method(task_statistics, number_of_reads),
	define_method(task_statistics, best_read_time),
	define_method(task_statistics, accumulated_read_time),
	{0}
};

static PyMethodDef _skin_rt_task_methods[] = {
	define_taskmethod(init),
	define_taskmethod(join),
	define_taskmethod(resume),
	define_taskmethod(suspend),
	define_taskmethod(wait_period),
	define_taskmethod(make_periodic),
	define_taskmethod(make_periodic_relative),
	define_taskmethod(delete),
	define_taskmethod(make_hard_real_time),
	define_taskmethod(make_soft_real_time),
	define_taskmethod(on_start),
	define_taskmethod(on_stop),
	{0}
};

static PyMethodDef _skin_rt_semaphore_methods[] = {
	define_method(rt_sem, wait),
	define_method(rt_sem, try_wait),
	define_method(rt_sem, timed_wait),
	define_method(rt_sem, post),
	define_method(rt_sem, broadcast),
	{0}
};

static PyMethodDef _skin_rt_mutex_methods[] = {
	define_method(rt_mutex, lock),
	define_method(rt_mutex, try_lock),
	define_method(rt_mutex, timed_lock),
	define_method(rt_mutex, unlock),
	define_method(rt_mutex, broadcast),
	{0}
};

static PyMethodDef _skin_rt_rwlock_methods[] = {
	define_method(rt_rwlock, read_lock),
	define_method(rt_rwlock, try_read_lock),
	define_method(rt_rwlock, timed_read_lock),
	define_method(rt_rwlock, read_unlock),
	define_method(rt_rwlock, write_lock),
	define_method(rt_rwlock, try_write_lock),
	define_method(rt_rwlock, timed_write_lock),
	define_method(rt_rwlock, write_unlock),
	{0}
};

static PyMethodDef _skin_globals[] = {
	define_gmethod(rt_get_free_name),
	define_gmethod(rt_priority_is_valid),
	define_gmethod(rt_init),
	define_gmethod(rt_stop),
	define_gmethod(rt_get_task),
	define_gmethod(rt_is_rt_context),
	define_gmethod(rt_get_time),
	define_gmethod(rt_get_exectime),
	define_gmethod(rt_sleep),
	define_gmethod(rt_sleep_until),
	define_gmethod(rt_name_available),
	{0}
};

#define ready_pytype(s)							\
do {									\
	_skin_##s##_pytype.tp_name = "pyskin."#s;			\
	_skin_##s##_pytype.tp_basicsize = sizeof(_skin_##s);		\
	_skin_##s##_pytype.tp_dealloc = (destructor)_skin_##s##_pyfree;	\
	_skin_##s##_pytype.tp_flags = Py_TPFLAGS_DEFAULT;		\
	_skin_##s##_pytype.tp_doc = "wrapper for skin_"#s;		\
	_skin_##s##_pytype.tp_methods = _skin_##s##_methods;		\
	_skin_##s##_pytype.tp_init = (initproc)_skin_##s##_pyinit;	\
	_skin_##s##_pytype.tp_new = _skin_##s##_pynew;			\
	if (PyType_Ready(&_skin_##s##_pytype) < 0)			\
		return;							\
} while (0)

#define add_pytype(s)							\
do {									\
	Py_INCREF(&_skin_##s##_pytype);					\
	PyModule_AddObject(m, #s, (PyObject *)&_skin_##s##_pytype);	\
} while (0)

#define add_pyconst(c)							\
do {									\
	PyObject *num = PyInt_FromLong(SKIN_##c);			\
	PyDict_SetItemString(d, #c, num);				\
	Py_DECREF(num);							\
} while (0)

#define add_exception(e)							\
do {										\
	_exc_##e = PyErr_NewException((char *)"pyskin.exc_"#e, NULL, NULL);	\
	PyDict_SetItemString(d, "exc_"#e, _exc_##e);				\
} while (0)

PyMODINIT_FUNC initpyskin(void)
{
	PyObject *m;
	PyObject *d;

	/* make threading possible */
	PyEval_InitThreads();

	ready_pytype(object);
	ready_pytype(reader);
	ready_pytype(service);
	ready_pytype(service_manager);
	ready_pytype(sensor_type);
	ready_pytype(sensor);
	ready_pytype(module);
	ready_pytype(patch);
	ready_pytype(sub_region);
	ready_pytype(region);
	ready_pytype(task_statistics);
	ready_pytype(rt_task);
	ready_pytype(rt_semaphore);
	ready_pytype(rt_mutex);
	ready_pytype(rt_rwlock);

	m = Py_InitModule3("pyskin", _skin_globals, "Python wrapper for SkinWare");
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);

	add_pytype(object);
	add_pytype(reader);
	add_pytype(service);
	add_pytype(service_manager);
	add_pytype(sensor_type);
	add_pytype(sensor);
	add_pytype(module);
	add_pytype(patch);
	add_pytype(sub_region);
	add_pytype(region);
	add_pytype(task_statistics);
	add_pytype(rt_task);
	add_pytype(rt_semaphore);
	add_pytype(rt_mutex);
	add_pytype(rt_rwlock);

	add_pyconst(INVALID_ID);
	add_pyconst(INVALID_SIZE);
	add_pyconst(SENSOR_TYPE_MAX);
	add_pyconst(SENSOR_MAX);
	add_pyconst(SUB_REGION_MAX);
	add_pyconst(REGION_MAX);
	add_pyconst(MODULE_MAX);
	add_pyconst(PATCH_MAX);
	add_pyconst(SENSOR_RESPONSE_MAX);
	add_pyconst(ALL_SENSOR_TYPES);
	add_pyconst(ACQUISITION_ASAP);
	add_pyconst(ACQUISITION_PERIODIC);
	add_pyconst(ACQUISITION_SPORADIC);
	add_pyconst(SERVICE_HEADER_SIZE);
	add_pyconst(SERVICE_PERIODIC);
	add_pyconst(SERVICE_SPORADIC);
	add_pyconst(SERVICE_ALIVE);
	add_pyconst(SERVICE_DEAD);
	add_pyconst(RT_SUCCESS);
	add_pyconst(RT_FAIL);
	add_pyconst(RT_INVALID);
	add_pyconst(RT_NO_MEM);
	add_pyconst(RT_TIMEOUT);
	add_pyconst(RT_SYNC_MECHANISM_ERROR);
	add_pyconst(RT_LOCK_NOT_ACQUIRED);
	add_pyconst(RT_INVALID_SEM);
	add_pyconst(RT_INVALID_MUTEX);
	add_pyconst(RT_INVALID_RWLOCK);
	add_pyconst(RT_MIN_PRIORITY);
	add_pyconst(RT_MAX_PRIORITY);
	add_pyconst(RT_LINUX_PRIORITY);
	add_pyconst(RT_MORE_PRIORITY);
	add_pyconst(RT_MAX_NAME_LENGTH);

	add_exception(partial);
	add_exception(fail);
	add_exception(no_file);
	add_exception(no_mem);
	add_exception(file_incomplete);
	add_exception(file_parse_error);
	add_exception(file_invalid);
	add_exception(lock_timeout);
	add_exception(lock_not_acquired);
	add_exception(bad_data);
	add_exception(bad_name);
	add_exception(bad_id);
	add_exception(rt_fail);
	add_exception(rt_invalid);
	add_exception(rt_no_mem);
	add_exception(rt_lock_timeout);
	add_exception(rt_sync_mech_error);
	add_exception(rt_lock_not_acquired);
}
