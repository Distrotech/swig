/* ----------------------------------------------------------------------------- 
 * type.c
 *
 *     This file defines a new python type that contains information from
 *     the WAD stack trace.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

#include "wad.h"
#include "Python.h"

typedef struct {
  PyObject_HEAD
  WadFrame   *frame;       /* Wad Stack frame object   */
  int         count;       /* Number of frames         */
} wadobject;

staticforward PyTypeObject WadObjectType;


PyObject *
new_wadobject(WadFrame *f, int count) {
  wadobject   *self;
  self = PyObject_NEW(wadobject, &WadObjectType);
  if (self == NULL) return NULL;
  
  self->frame = f;
  if (count > 0) {
    self->count = count;
  } else {
    self->count = 0;
    while (f) {
      self->count++;
      f = f->next;
    }
  }
  return (PyObject *) self;
}

/* release a wad object */
static void
wadobject_dealloc(wadobject *self) {
  PyMem_DEL(self);
}

static PyObject *
wadobject_repr(wadobject *self) {
  char message[65536];
  char *srcstr = 0;
  WadFrame *fp = 0;
  int    n;
  WadFrame *f = self->frame;

  strcpy(message,"[ C stack trace ]\n\n");
  /* Find the last exception frame */
  n = self->count;
  while (f && n) {
    fp = f;
    f= f->next;
    n--;
  }

  if (fp) {
  /* Now work backwards */
    f = fp;
    while (f) {
      strcat(message, f->debug_str);
      if (f->debug_srcstr) srcstr = f->debug_srcstr;
      if (f == self->frame) break;
      f = f->prev;
    }
    if (srcstr) {
      strcat(message,"\n");
      strcat(message, srcstr);
      strcat(message,"\n");
    }
  }
  return PyString_FromString(message);
}

static PyObject *
wadobject_str(wadobject *self) {
  char message[65536];
  char *srcstr = 0;
  int   n;

  WadFrame *f = self->frame;
  n = self->count;
  strcpy(message,"[ C stack trace ]\n\n");
  /* Find the last exception frame */
  while (!f->last && n) {
    f= f->next;
    n--;
  }
  /* Now work backwards */
  if (n <= 0) {
    f = f->prev;
  }
  while (f) {
    strcat(message, f->debug_str);
    if (f->debug_srcstr) srcstr = f->debug_srcstr;
    if (self->frame == f) break;
    f = f->prev;
  }
  if (srcstr) {
    strcat(message,"\n");
    strcat(message, srcstr);
    strcat(message,"\n");
  }
  return PyString_FromString(message);
}

static int
wadobject_len(wadobject *self) {
  int n = 0;
  WadFrame *f = self->frame;
  while (f) {
    n++;
    f = f->next;
  }
  return n;
}

static PyObject *
wadobject_getitem(wadobject *self, int n) {
  int i;
  WadFrame *f;
  if (n < 0) {
    n = self->count + n;
  }
  if ((n < 0) || (n >= self->count)) {
    PyErr_SetString(PyExc_IndexError,"Stack frame out of range");
    return NULL;
  }
  f = self->frame;
  for (i = 0; i <n; i++) {
    f = f->next;
  }
  return new_wadobject(f,1);
}

static PyObject *
wadobject_getslice(wadobject *self, int start, int end) {
  int i;
  WadFrame *f;

  f = self->frame;
  for (i = 0; i < start; i++) {
    f = f->next;
  }
  return new_wadobject(f,(end-start));
}

static PyObject *
wadobject_getattr(wadobject *self, char *name) {
  if (strcmp(name,"name") == 0) {
    return Py_BuildValue("z", self->frame->sym_name);
  } else if (strcmp(name,"exe") == 0) {
    return Py_BuildValue("z", self->frame->object->path);
  } else if (strcmp(name,"source") == 0) {
    return Py_BuildValue("z", self->frame->loc_srcfile);
  } else if (strcmp(name,"object") == 0) {
    return Py_BuildValue("z", self->frame->loc_objfile);
  } else if (strcmp(name,"line") == 0) {
    return Py_BuildValue("i", self->frame->loc_line);
  } else if (strcmp(name,"pc") == 0) {
    return PyLong_FromUnsignedLong(self->frame->pc);
  } else if (strcmp(name,"sp") == 0) {
    return PyLong_FromUnsignedLong(self->frame->sp);
  } else if (strcmp(name,"fp") == 0) {
    return PyLong_FromUnsignedLong(self->frame->fp);
  } else if (strcmp(name,"stack_size") == 0) {
    return PyInt_FromLong(self->frame->stack_size);
  } else if (strcmp(name,"stack") == 0) {
    return PyString_FromStringAndSize(self->frame->stack, self->frame->stack_size);
  } else if (strcmp(name,"nargs") == 0) {
    return PyInt_FromLong(self->frame->debug_nargs);
  } else if (strcmp(name,"seg_base") == 0) {
    return PyLong_FromUnsignedLong((long )self->frame->segment->base);
  } else if (strcmp(name,"seg_size") == 0) {
    return PyLong_FromUnsignedLong((long) self->frame->segment->size);
  }

  PyErr_SetString(PyExc_NameError,"Unknown attribute.");
  return NULL;
}
static PySequenceMethods wadobject_as_sequence = {
  (inquiry)   wadobject_len,
  0,
  0,
  (intargfunc)  wadobject_getitem,  /* get item */
  (intintargfunc)  wadobject_getslice,  /* get slice */
  0,
  0
};

static PyTypeObject WadObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "WadObject",
  sizeof(wadobject),
  0,
  (destructor) wadobject_dealloc,
  0,             /* printfunc */
  (getattrfunc) wadobject_getattr,
  (setattrfunc) 0,
  (cmpfunc) 0,
  (reprfunc) wadobject_repr,
  
  0,     /* number */
  &wadobject_as_sequence,     /* sequence */
  0,     /* mapping */
  0,     /* hash */
  0,     /* call */
  (reprfunc) wadobject_str,   /* str */
};

