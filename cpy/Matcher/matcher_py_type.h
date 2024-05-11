#ifndef MATCHER_PY_TYPE_H
#define MATCHER_PY_TYPE_H

#include <Python.h>

#include "dfa_table.h"
#include "pattern_py_type.h"

typedef struct {
    PyObject_HEAD Py_buffer str_view;
    PatternObject *pattern_object;
    Py_ssize_t forward_offset;
    unsigned int current_state;
} MatcherIterator;

#define MatcherIteratorCAST(PyObject_ptr) ((MatcherIterator *)(PyObject_ptr))
#define MatcherIterator_buf(PyObject_ptr) \
    ((char *)(PyObject_ptr)->str_view.buf)
#define MatcherIterator_len(PyObject_ptr) ((PyObject_ptr)->str_view.len)

PyObject *
MatcherIterator_new(
        PyTypeObject *subtype, 
        PyObject *args, 
        PyObject *kwds);

void
MatcherIterator_dealloc(MatcherIterator *self);

PyObject *
MatcherIterator_iter(PyObject *self);

PyObject *
MatcherIterator_iternext(PyObject *self);

static PyType_Slot MatcherIterator_slots[] = {
    {Py_tp_dealloc,  (void *) MatcherIterator_dealloc},
    {Py_tp_iter,     (void *) MatcherIterator_iter},
    {Py_tp_iternext, (void *) MatcherIterator_iternext},
    {0, 0}};

static PyType_Spec spec_matcher_iterator = {
    "MatcherIterator", sizeof(MatcherIterator), 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, MatcherIterator_slots};

PyObject *PyMatcherIterator_Type;

// module methods

PyObject *
finditer(PyObject *, PyObject *);

#define finditer_METHODDEF                                                   \
    {                                                                        \
        "finditer", finditer, METH_VARARGS,                                  \
            "finditer(pattern, string) - return an iterator yielding match " \
            "tuples"                                                         \
    }

#endif
