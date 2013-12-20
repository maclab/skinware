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
#include <skin.h>
#include "example_pyservice.h"

typedef struct _example_pyservice
{
	PyObject_HEAD
	struct example_pyservice_data *d;
} _example_pyservice;

/*
 * note: these functions will be called from real-time context.
 * Therefore, memory allocation is not allowed.  The current solution
 * (static variable) means that only one service can use this function,
 * which is not too restrictive, since the file is designed for that
 * specific service.
 *
 * P.S. The whole thing should be tested, since the binding seems to be
 * full of tuple creation both for args and return values.  Memory
 * allocation may in the end turn out to be completely unavoidable.
 * In that case, the service won't be real-time, so perhaps a service
 * in python wouldn't then make much sense.
 *
 * There perhaps could be a workaround for the return value, but I don't
 * know about the 
 */
static _example_pyservice *_cur;

static PyTypeObject _pytype = {PyObject_HEAD_INIT(NULL)};

static PyObject *_at_index(PyObject *unused, PyObject *args)
{
	PyObject *ref;
	int index;
	struct example_pyservice_data *mem;

	if (!PyArg_ParseTuple(args, "Oi", &ref, &index))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: at_index(void *, int)");
		return NULL;
	}
	mem = PyCObject_AsVoidPtr(ref);
	_cur->d = &mem[index];

	return Py_BuildValue("O", _cur);
}

static PyObject *_get_x(_example_pyservice *obj, PyObject *args)
{
	return Py_BuildValue("i", obj->d->x);
}

static PyObject *_get_y(_example_pyservice *obj, PyObject *args)
{
	return Py_BuildValue("i", obj->d->y);
}

static PyObject *_set_x(_example_pyservice *obj, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i", &obj->d->x))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: set_x(int)");
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *_set_y(_example_pyservice *obj, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "i", &obj->d->y))
	{
		PyErr_SetString(PyExc_ValueError, "Usage: set_y(int)");
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject *_elem_size(PyObject *unused, PyObject *args)
{
	return Py_BuildValue("i", sizeof(struct example_pyservice_data));
}

static PyMethodDef _methods[] = {
	{"get_x", (PyCFunction)_get_x, METH_VARARGS, "Get x of data"},
	{"get_y", (PyCFunction)_get_y, METH_VARARGS, "Get y of data"},
	{"set_x", (PyCFunction)_set_x, METH_VARARGS, "Set x of data"},
	{"set_y", (PyCFunction)_set_y, METH_VARARGS, "Set y of data"},
	{0}
};

static PyMethodDef _globals[] = {
	{"at_index", _at_index, METH_VARARGS, "Get data at given index"},
	{"elem_size", _elem_size, METH_VARARGS, "Get size of each element"},
	{0}
};

PyMODINIT_FUNC initexample_pyservice(void)
{
	PyObject *m;
	PyObject *d;
	PyObject *str;

	_pytype.tp_name = "example_pyservice.data";
	_pytype.tp_basicsize = sizeof(_example_pyservice);
	_pytype.tp_flags = Py_TPFLAGS_DEFAULT;
	_pytype.tp_doc = "Data for example Python Skinware service";
	_pytype.tp_methods = _methods;
	if (PyType_Ready(&_pytype) < 0)
		return;

	/* allocate one element now, to prevent allocating memory in real-time context */
	_cur = (_example_pyservice *)_pytype.tp_alloc(&_pytype, 0);
	if (_cur == NULL)
		return;

	m = Py_InitModule3("example_pyservice", _globals, "Example of a Python Skinware service");
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);

	Py_INCREF(&_pytype);
	PyModule_AddObject(m, "data", (PyObject *)&_pytype);

	str = PyString_FromString(EXAMPLE_PYSERVICE_MEMNAME);
	PyDict_SetItemString(d, "MEMNAME", str);
	Py_DECREF(str);
	str = PyString_FromString(EXAMPLE_PYSERVICE_REQUEST);
	PyDict_SetItemString(d, "REQUEST", str);
	Py_DECREF(str);
	str = PyString_FromString(EXAMPLE_PYSERVICE_RESPONSE);
	PyDict_SetItemString(d, "RESPONSE", str);
	Py_DECREF(str);
}
