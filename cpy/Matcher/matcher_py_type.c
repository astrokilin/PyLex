#include "matcher_py_type.h"

#include "pattern_py_type.h"

PyObject *
finditer(PyObject *self, PyObject *args)
{
    return MatcherIterator_new(
            (PyTypeObject *)PyMatcherIterator_Type, 
            args,
            NULL);
}

PyObject *
MatcherIterator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *string;
    PyObject *pattern;
    MatcherIterator *self;

    string = NULL;
    self = (MatcherIterator *)type->tp_alloc(type, 0);

    if (self == NULL){
        return NULL;
    }

    if (!PyArg_ParseTuple(
                args, 
                "O!O!", 
                PyPattern_Type, 
                &pattern,
                &PyUnicode_Type, 
                &string))
    {
        goto CLEANUP_EXIT;
    }

    string = PyUnicode_AsEncodedString(string, "ascii", "strict");

    if (string == NULL)
    {
        goto CLEANUP_EXIT;
    }

    if (PyObject_GetBuffer(string, &self->str_view, PyBUF_CONTIG_RO) == -1) 
    {
        printf("no buffer\n");
        goto CLEANUP_EXIT;
    }

    Py_INCREF(pattern);
    Py_DECREF(string);
    self->pattern_object = (PatternObject *)pattern;
    self->forward_offset = 0;
    self->current_state = 0;
    return (PyObject *)self;

CLEANUP_EXIT:
    Py_XDECREF(string);
    Py_TYPE(self)->tp_free((PyObject *)self);
    return NULL;
}

void
MatcherIterator_dealloc(MatcherIterator *iter)
{
    PyBuffer_Release(&iter->str_view);
    Py_DECREF(iter->pattern_object);
    Py_TYPE(iter)->tp_free((PyObject *)iter);
}

PyObject *
MatcherIterator_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

PyObject *
MatcherIterator_iternext(PyObject *self)
{
    MatcherIterator *self_iter;
    PyObject *result_tuple;
    int check_code;
    unsigned int last_match_target;
    unsigned int tmp;
    Py_ssize_t last_match_offset;
    char match_occured;
    Py_ssize_t current_offset;
    dfa_table *p;

    self_iter = MatcherIteratorCAST(self);
    match_occured = 0;
    tmp = 0;
    current_offset = self_iter->forward_offset;
    p = &(self_iter->pattern_object->table);
    int re;

    while (current_offset < MatcherIterator_len(self_iter))
    {
        switch (dfa_table_next_state(
                    p, 
                    self_iter->current_state,
                    MatcherIterator_buf(self_iter)[current_offset],
                    &self_iter->current_state)) 
        {
            case DFA_TRANSITION_IMPOSIBLE:
                self_iter->current_state = 0;

                if (match_occured)
                {
                    goto MATCH_FOUND;
                }

                self_iter->forward_offset = current_offset + 1;
                break;

            case DFA_TRANSITION_SUCCES:
                check_code = dfa_table_is_state_accepting(
                    p, self_iter->current_state, &tmp);

                if (check_code == DFA_STATE_ACCEPTING &&
                    (!match_occured || tmp <= last_match_target)) 
                {
                    last_match_offset =
                        current_offset - self_iter->forward_offset;
                    last_match_target = tmp;
                    match_occured = 1;
                }
                break;
        }
        current_offset++;
    }

    // end of string and still no match
    if (!match_occured)
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

MATCH_FOUND:
    result_tuple = Py_BuildValue(
            "Inn", 
            last_match_target, 
            self_iter->forward_offset,
            last_match_offset + 1);

    self_iter->forward_offset = current_offset + 1;
    return result_tuple;
}
