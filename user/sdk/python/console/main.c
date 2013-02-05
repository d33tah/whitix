#include <Python.h>
#include <structmember.h>

/* Console includes */
#include <console.h>

/* Globals */
static PyObject* completer=NULL;

/* Library functions */
static PyObject* wSetHook(const char* funcName, PyObject** hookVar, PyObject* args)
{
	PyObject* function = NULL;
	char buf[80];

	PyOS_snprintf(buf, sizeof(buf), "|O:set%.50s", funcName);
	if (!PyArg_ParseTuple(args, buf, &function))
		return NULL;

	if (function == Py_None)
	{
		Py_XDECREF(*hookVar);
		*hookVar = NULL;
	}else if (PyCallable_Check(function))
	{
		PyObject* tmp = *hookVar;
		Py_INCREF(function);
		*hookVar = function;
		Py_XDECREF(tmp);
	}else{
		PyOS_snprintf(buf, sizeof(buf), "set%.50s(function): argument not callable", funcName);
		PyErr_SetString(PyExc_TypeError, buf);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Python functions. */

static PyObject* wConsoleClearScreen(PyObject *self, PyObject *noarg)
{
	ConsClearScreen();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleClearScreenDoc,
"clearScreen() -> none\n\
Clear the console and move the cursor to the home position.");

static PyObject* wConsoleSetForeColor(PyObject* self, PyObject* args)
{
	int color;

	if (!PyArg_ParseTuple(args, "i:setForeColor", &color))
		return NULL;

	ConsSetForeColor(color);

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleSetForeColorDoc,
"setForeColor(color) -> none\n\
Set the foreground color of the console (change the text color).");

static PyObject* wConsoleMoveCursor(PyObject* self, PyObject* args)
{
	int x, y;

	if (!PyArg_ParseTuple(args, "ii:moveCursor", &x, &y))
		return NULL;

	ConsCursorMove(x, y);

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleMoveCursorDoc,
"moveCursor(x, y) -> none\n\
Move the text cursor to position (x, y) from the top-left of the console.");

static PyObject* wConsoleEraseLine(PyObject *self, PyObject *noarg)
{
	ConsEraseLine();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleEraseLineDoc,
"eraseLine() -> none\n\
Clear the current line of the console.");

static PyObject* wConsoleInvert(PyObject *self, PyObject *noarg)
{
	ConsInvert();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleInvertDoc,
"invert() -> none\n\
Invert both the foreground and background console colors");

static PyObject* wConsoleColorReset(PyObject *self, PyObject *noarg)
{
	ConsColorReset();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleColorResetDoc,
"reset() -> none\n\
Reset the console to the default foreground and background colors.");

static PyObject* wConsoleGetDimensions(PyObject *self, PyObject *noarg)
{
	struct ConsoleInfo consInfo;
	PyObject* convert;
	PyObject* pos;

	ConsGetDimensions(&consInfo);

	pos = PyTuple_New(2);

	if (!pos)
		return NULL;

	convert=PyInt_FromLong(consInfo.cols);
	PyTuple_SetItem(pos, 0, convert);

	convert=PyInt_FromLong(consInfo.rows);
	PyTuple_SetItem(pos, 1, convert);

	return pos;
}

PyDoc_STRVAR(wConsoleGetDimensionsDoc,
"getDimensions() -> (width, height)\n\
Get the width and height of the current console.");

static PyObject* wConsoleSetEcho(PyObject* self, PyObject* args)
{
	int echoOn;

	if (!PyArg_ParseTuple(args, "i:setEcho", &echoOn))
		return NULL;

	ConsSetEcho(echoOn);

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(wConsoleSetEchoDoc,
"setEcho(on) -> none\n\
Turn the console echo on or off.");

static PyObject* wConsoleReadLine(PyObject* self, PyObject* args)
{
	char* prompt;

	if (!PyArg_ParseTuple(args, "s:readLine", &prompt))
		return NULL;

	char* p=ConsReadLine(prompt);

	return PyString_FromString(p);
}

PyDoc_STRVAR(wConsoleReadLineDoc,
"readLine(prompt) -> string\n\
Read a line of input from the console.");

static PyObject* wConsoleSetCompleter(PyObject* self, PyObject* args)
{
	return wSetHook("Completer", &completer, args);
}

PyDoc_STRVAR(wConsoleSetCompleterDoc,
"setCompleter(function) -> None\n\
Set the function used for tab completion.");

/* Call the python tab completer function. */
char* wConsoleCallCompleter(char* text, int state)
{
	char* result = NULL;

	if (completer != NULL)
	{
		PyObject* r;
#ifdef WITH_THREAD	      
		PyGILState_STATE gilstate = PyGILState_Ensure();
#endif
		r = PyObject_CallFunction(completer, "si", text, state);
		if (r == NULL)
			goto error;
		if (r == Py_None)
		{
			result = NULL;
		}else{
			char *s = PyString_AsString(r);
			if (s == NULL)
				goto error;
			result = strdup(s);
		}

		Py_DECREF(r);
		goto done;
	  error:
		PyErr_Clear();
		Py_XDECREF(r);
	  done:
#ifdef WITH_THREAD	      
		PyGILState_Release(gilstate);
#endif
	}

	return result;
}

static struct PyMethodDef wConsoleMethods[] =
{
	/* Console display. */
	{"clearScreen", wConsoleClearScreen, METH_NOARGS, wConsoleClearScreenDoc},
	{"setForeColor", wConsoleSetForeColor, METH_VARARGS, wConsoleSetForeColorDoc},
	{"moveCursor", wConsoleMoveCursor, METH_VARARGS, wConsoleMoveCursorDoc},
	{"eraseLine", wConsoleEraseLine, METH_NOARGS, wConsoleEraseLineDoc},
	{"invert", wConsoleInvert, METH_NOARGS, wConsoleInvertDoc},
	{"colorReset", wConsoleColorReset, METH_NOARGS, wConsoleColorResetDoc},
	{"getDimensions", wConsoleGetDimensions, METH_NOARGS, wConsoleGetDimensionsDoc},
	{"setEcho", wConsoleSetEcho, METH_VARARGS, wConsoleSetEchoDoc},

	/* Line editing. */
	{"readLine", wConsoleReadLine, METH_VARARGS, wConsoleReadLineDoc},
	{"setCompleter", wConsoleSetCompleter, METH_VARARGS, wConsoleSetCompleterDoc},
	{0, 0},
};

void ReadLineInit()
{
	ConsSetTabCompleter(wConsoleCallCompleter);
}

PyDoc_STRVAR(docModule,
"Importing this module enables usage of the Whitix console API.");

PyMODINIT_FUNC initwconsole(void)
{
	Py_Initialize();

	PyObject* m;

	m = Py_InitModule4("wconsole", wConsoleMethods, docModule,
			   (PyObject *)NULL, PYTHON_API_VERSION);

	if (m == NULL)
		return;

	PyModule_AddIntConstant(m, "ColorBlue", CONS_COLOR_BLUE);

	ReadLineInit();
}
