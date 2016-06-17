#include "pyapi.h"
#include "connection.h"
#include "emit.h"

#include "unicode.h"
#include "pytypes.h"
#include "interprocess.h"
#include "type_conversion.h"
#include "formatinput.h"


static void _loader_import_array(void) {
    import_array();
}

str _loader_init(void)
{
    str msg = MAL_SUCCEED;
    _loader_import_array();
    msg = _emit_init();
    if (msg != MAL_SUCCEED) {
        return msg;
    }

    if (PyType_Ready(&Py_ConnectionType) < 0)
        return createException(MAL, "pyapi.eval", "Failed to initialize loader functions.");
    return msg;
}


str PyAPIevalLoader(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci) {
    sql_func * sqlfun;
	sql_subfunc * sqlmorefun;
    str exprStr;

    const int additional_columns = 2;
    int i = 1, ai = 0;
    char* pycall = NULL;
    str *args;
    char *msg = MAL_SUCCEED;
    node * argnode;
    PyObject *pArgs = NULL, *pEmit = NULL, *pConnection; // this is going to be the parameter tuple
    PyObject *code_object = NULL;
    EmitCol *cols = NULL;
    bool gstate = 0;
    int unnamedArgs = 0;
    int argcount = pci->argc;

    char * loader_additional_args[] = {"_emit", "_conn"};

    if (!PyAPIEnabled()) {
        throw(MAL, "pyapi.eval",
              "Embedded Python has not been enabled. Start server with --set %s=true",
              pyapi_enableflag);
    }

    if (!PyAPIInitialized()) {
        throw(MAL, "pyapi.eval",
              "Embedded Python is enabled but an error was thrown during initialization.");
    }
    sqlmorefun = *(sql_subfunc**) getArgReference(stk, pci, pci->retc);
    sqlfun = sqlmorefun->func;
    exprStr = *getArgReference_str(stk, pci, pci->retc + 1);


    args = (str*) GDKzalloc(pci->argc * sizeof(str));
    if (args == NULL) {
        throw(MAL, "pyapi.eval", MAL_MALLOC_FAIL" arguments.");
    }

    // Analyse the SQL_Func structure to get the parameter names
    if (sqlfun != NULL && sqlfun->ops->cnt > 0) {
        unnamedArgs = pci->retc + 2;
        argnode = sqlfun->ops->h;
        while (argnode) {
            char* argname = ((sql_arg*) argnode->data)->name;
            args[unnamedArgs++] = GDKstrdup(argname);
            argnode = argnode->next;
        }

    }

    // We name all the unknown arguments
    for (i = pci->retc + 2; i < argcount; i++) {
        if (args[i] == NULL) {
			char argbuf[64];
			snprintf(argbuf, sizeof(argbuf), "arg%i", i - pci->retc - 1);
			args[i] = GDKstrdup(argbuf);
        }
    }
    gstate = Python_ObtainGIL();

    pArgs = PyTuple_New(argcount - pci->retc - 2 + additional_columns);
    // TODO: check pArgs
    ai = 0;

    argnode = sqlfun && sqlfun->ops->cnt > 0 ? sqlfun->ops->h : NULL;
    for (i = pci->retc + 2; i < argcount; i++) {
        PyInput inp;

        PyObject *val = NULL;
        if (isaBatType(getArgType(mb,pci,i))) {
        	// complain
        }
		inp.scalar = true;
		inp.bat_type = getArgType(mb, pci, i);
		inp.count = 1;
		if (inp.bat_type == TYPE_str) {
			inp.dataptr = getArgReference_str(stk, pci, i);
		}
		else {
			inp.dataptr = getArgReference(stk, pci, i);
		}
		val = PyArrayObject_FromScalar(&inp, &msg);
		if (msg != MAL_SUCCEED) {
			// complain
		}
		if (PyTuple_SetItem(pArgs, ai++, val) != 0) {
			// complain
		}
		// TODO deal with sql types
    }

    pConnection = Py_Connection_Create(cntxt, 0, 0, 0);
    {
    	node *n = sqlmorefun->colnames->h;
    	cols = GDKmalloc(sizeof(EmitCol) * pci->retc);
    	if (!cols) {
    		// complain
    	}
    	i = 0;
    	while (n) {
    		cols[i].name = *((char**) n->data);
    		n = n->next;
    		cols[i].b = BATnew(TYPE_void, getColumnType(getArgType(mb, pci, i)), 0, TRANSIENT);
    		i++;
    	}
    	// TODO: assert list length == number of return BATs
    	pEmit = Py_Emit_Create(cols, pci->retc);
    	// TODO: check pConnection

    }
    PyTuple_SetItem(pArgs, ai++, pEmit);
    PyTuple_SetItem(pArgs, ai++, pConnection);


    pycall = FormatCode(exprStr, args, argcount, 4, &code_object, &msg, loader_additional_args, additional_columns);
    if (pycall == NULL && code_object == NULL) {
        if (msg == NULL) { msg = createException(MAL, "pyapi.eval", "Error while parsing Python code."); }
        goto wrapup;
    }


    {
		PyObject *pFunc, *pModule, *v, *d;

		// First we will load the main module, this is required
		pModule = PyImport_AddModule("__main__");
		if (!pModule) {
			msg = PyError_CreateException("Failed to load module", NULL);
			goto wrapup;
		}

		// Now we will add the UDF to the main module
		d = PyModule_GetDict(pModule);
		if (code_object == NULL) {
			v = PyRun_StringFlags(pycall, Py_file_input, d, d, NULL);
			if (v == NULL) {
				msg = PyError_CreateException("Could not parse Python code", pycall);
				goto wrapup;
			}
			Py_DECREF(v);

			// Now we need to obtain a pointer to the function, the function is called "pyfun"
			pFunc = PyObject_GetAttrString(pModule, "pyfun");
			if (!pFunc || !PyCallable_Check(pFunc)) {
				msg = PyError_CreateException("Failed to load function", NULL);
				goto wrapup;
			}
		} else {
			pFunc = PyFunction_New(code_object, d);
			if (!pFunc || !PyCallable_Check(pFunc)) {
				msg = PyError_CreateException("Failed to load function", NULL);
				goto wrapup;
			}
		}
        PyObject_CallObject(pFunc, pArgs);

        Py_DECREF(pFunc);
        Py_DECREF(pArgs);

        if (PyErr_Occurred()) {
            msg = PyError_CreateException("Python exception", pycall);
            if (code_object == NULL) { PyRun_SimpleString("del pyfun"); }
            goto wrapup;
        }


    }


    {
    	size_t nval = ((Py_EmitObject *) pEmit)->nvals;
    	for (i = 0; i < pci->retc; i++) {
    		BAT *b = cols[i].b;
    		BATsetcount(b, nval);
    		// TODO set various other BAT properties here

            *getArgReference_bat(stk, pci, i) = b->batCacheid;
            BBPkeepref(b->batCacheid);
    	}
    }

    wrapup:
	// TODO: fix leaks of which there are many
		Python_ReleaseGIL(gstate);
		return(msg);

}
