#include "pattern_py_type.h"

#include <stdio.h>
#include <string.h>

void
Pattern_dealloc(PatternObject *self)
{
    dfa_table_deinit(&self->table);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *
Pattern_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PatternObject *self;

    self = (PatternObject *)type->tp_alloc(type, 0);

    if (self != NULL) 
    {
        memset(&self->table, 0, sizeof(dfa_table));
    }

    return (PyObject *)self;
}

int
Pattern_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *patterns_list;
    Py_ssize_t list_len;
    PyObject **ascii_strings;
    char **patterns;
    int result;
    dfa_table_syntax_error err_msg;

    patterns_list = NULL;
    list_len = 0;
    ascii_strings = NULL;
    patterns = 0;
    result = -1;

    dfa_table_deinit(&(((PatternObject *)self)->table));

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &patterns_list)) 
    {
        return -1;
    }

    list_len = PyList_Size(patterns_list);

    patterns = (char **)PyMem_RawCalloc(list_len, sizeof(char *));

    if (patterns == NULL) 
    {
        PyErr_NoMemory();
        goto CLEANUP_EXIT;
    }

    ascii_strings = (PyObject **)PyMem_RawCalloc(list_len, sizeof(PyObject *));

    if (ascii_strings == NULL) 
    {
        PyErr_NoMemory();
        goto CLEANUP_EXIT;
    }

    for (Py_ssize_t i = 0; i < list_len; i++) 
    {
        ascii_strings[i] = PyUnicode_AsASCIIString(
                 PyList_GetItem(patterns_list, i));

        if (ascii_strings[i] == NULL)
        {
            goto CLEANUP_EXIT;
        }

        patterns[i] = PyBytes_AsString(ascii_strings[i]);
    }

    switch (dfa_table_init(
                &((PatternObject *)self)->table, 
                patterns, 
                list_len,
                &err_msg))
    {
        case DFA_BUILD_ERR_NOMEM:
            PyErr_NoMemory();
            goto CLEANUP_EXIT;
            break;

        case DFA_BUILD_ERR_SYNTAX:
            PyErr_SetString(PyExc_ValueError, err_msg.err_str);
            goto CLEANUP_EXIT;
            break;

        case DFA_BUILD_ERR_OVERFLOW:
            PyErr_SetString(PyExc_ValueError, "Too many statements");
            goto CLEANUP_EXIT;
            break;
    }

    result = 0;

CLEANUP_EXIT:
    PyMem_RawFree(patterns);

    for (Py_ssize_t i = 0; i < list_len; i++) 
    {
        Py_XDECREF(ascii_strings[i]);
    }

    PyMem_RawFree(ascii_strings);

    return result;
}
