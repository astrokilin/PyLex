#include <Python.h>

#include "matcher_py_type.h"
#include "pattern_py_type.h"

static PyMethodDef pylex_methods[] = {finditer_METHODDEF,
                                      {NULL, NULL, 0, NULL}};


PyModuleDef pylex_module = {
    PyModuleDef_HEAD_INIT,
    "pylex",  // Module name
    "Simple python lexer",
    -1,             // Optional size of the module state memory
    pylex_methods,  // Optional module methods
    NULL,           // Optional slot definitions
    NULL,           // Optional traversal function
    NULL,           // Optional clear function
    NULL            // Optional module deallocation function
};

PyMODINIT_FUNC
PyInit_pylex(void)
{
    PyObject *module;
    PyPattern_Type = NULL;
    PyMatcherIterator_Type = NULL;

    module = PyModule_Create(&pylex_module);

    PyPattern_Type = PyType_FromSpec(&spec_pattern);

    if (PyPattern_Type == NULL)
        goto CLEANUP_EXIT;

    Py_INCREF(PyPattern_Type);

    PyMatcherIterator_Type = PyType_FromSpec(&spec_matcher_iterator);

    if (PyMatcherIterator_Type == NULL)
        goto CLEANUP_EXIT;

    Py_INCREF(PyMatcherIterator_Type);

    if (PyModule_AddObject(module, spec_pattern.name, PyPattern_Type) < 0)
        goto CLEANUP_EXIT;

    if (PyModule_AddObject(module, "_MatcherIterator",
                           PyMatcherIterator_Type) < 0)
        goto CLEANUP_EXIT;

    return module;

CLEANUP_EXIT:
    Py_XDECREF(module);
    Py_XDECREF(PyPattern_Type);
    Py_XDECREF(PyMatcherIterator_Type);
    return NULL;
}
