#ifndef PATTERN_PY_TYPE_H
#define PATTERN_PY_TYPE_H

#include <Python.h>

#include "dfa_table.h"

typedef struct {
    PyObject_HEAD dfa_table table;
} PatternObject;

#define PATTERN_TABLE_ADDR(PatternObject_ptr) \
    (&(((PatternObject *)(PatternObject_ptr))->table))

int
Pattern_init(PyObject *self, PyObject *args, PyObject *kwds);
PyObject *
Pattern_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds);
void
Pattern_dealloc(PatternObject *self);

static PyType_Slot Pattern_slots[] = {
    {Py_tp_new, (void *)Pattern_new},
    {Py_tp_init, (void *)Pattern_init},
    {Py_tp_dealloc, (void *)Pattern_dealloc},
    {0, 0}};

static PyType_Spec spec_pattern = {
    "Pattern",                                 // name
    sizeof(PatternObject),                     // basicsize
    0,                                         // itemsize
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // flags
    Pattern_slots                              // slots
};

PyObject *PyPattern_Type;

#endif
