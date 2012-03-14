/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include <pdal/pdal_internal.hpp>
#ifdef PDAL_HAVE_PYTHON

#include <pdal/plang/Invocation.hpp>

#ifdef PDAL_COMPILER_MSVC
#  pragma warning(disable: 4127) // conditional expression is constant
#endif

#include <Python.h>
#include <numpy/arrayobject.h>


namespace pdal { namespace plang {

void Invocation::numpy_init()
{
    // this macro is defined be NumPy and must be included
    if (_import_array() < 0)
    {
        throw python_error("unable to initialize NumPy");
    }
}


Invocation::Invocation(const std::string& source)
    : m_env(*Environment::get())
    , m_source(source)
    , m_compile(NULL)
    , m_module(NULL)
    , m_dict(NULL)
    , m_func(NULL)
    , m_scriptSource(NULL)
    , m_varsIn(NULL)
    , m_varsOut(NULL)
    , m_scriptArgs(NULL)
    , m_scriptResult(NULL)
{
    resetArguments();

    return;
}


void Invocation::compile()
{
    m_compile = Py_CompileString(m_source.c_str(), "YowModule", Py_file_input);
    if (!m_compile) m_env.handleError();

    assert(m_compile);
    m_module = PyImport_ExecCodeModule("YowModule", m_compile);
    if (!m_module) m_env.handleError();

    m_dict = PyModule_GetDict(m_module);

    m_func = PyDict_GetItemString(m_dict, "yow");
    if (!m_func) m_env.handleError();
    if (!PyCallable_Check(m_func)) m_env.handleError();
  
    return;
}


void Invocation::resetArguments()
{
    Py_XDECREF(m_varsIn);
    Py_XDECREF(m_varsOut);

    Py_XDECREF(m_scriptResult);

    Py_XDECREF(m_scriptArgs); // also decrements script and vars

    for (unsigned int i=0; i<m_pyInputArrays.size(); i++)
    {
        PyObject* obj = m_pyInputArrays[i];
        Py_XDECREF(obj);
    }
    m_pyInputArrays.clear();

    m_varsIn = PyDict_New();
    m_varsOut = PyDict_New();
    
    return;
}



void Invocation::insertArgument(const std::string& name, 
                                   boost::uint8_t* data, 
                                   boost::uint32_t data_len, 
                                   boost::uint32_t data_stride,                                  
                                   dimension::Interpretation dataType, 
                                   boost::uint32_t numBytes)
{
    npy_intp mydims = data_len;
    int nd = 1;
    npy_intp* dims = &mydims;
    npy_intp stride = data_stride;
    npy_intp* strides = &stride;
    int flags = NPY_CARRAY; // NPY_BEHAVED

    const int pyDataType = getPythonDataType(dataType, numBytes);
        
    PyObject* pyArray = PyArray_New(&PyArray_Type, nd, dims, pyDataType, strides, data, 0, flags, NULL);
    
    m_pyInputArrays.push_back(pyArray);

    PyDict_SetItemString(m_varsIn, name.c_str(), pyArray);
    
    return;
}




void Invocation::extractResult(const std::string& name, 
                                  boost::uint8_t* dst, 
                                  boost::uint32_t data_len, 
                                  boost::uint32_t data_stride,                                  
                                  dimension::Interpretation dataType, 
                                  boost::uint32_t numBytes)
{      
    PyObject* xarr = PyDict_GetItemString(m_varsOut, name.c_str());
    assert(xarr);
    assert(PyArray_Check(xarr));
    PyArrayObject* arr = (PyArrayObject*)xarr;

    npy_intp one=0;
    const int pyDataType = getPythonDataType(dataType, numBytes);
    
    boost::uint8_t* p = dst;

    if (pyDataType == PyArray_DOUBLE)
    {
        double* src = (double*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(double*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_FLOAT)
    {
        float* src = (float*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(float*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_BYTE)
    {
        boost::int8_t* src = (boost::int8_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::int8_t*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_UBYTE)
    {
        boost::uint8_t* src = (boost::uint8_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::uint8_t*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_INT)
    {
        boost::int32_t* src = (boost::int32_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::int32_t*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_UINT)
    {
        boost::uint32_t* src = (boost::uint32_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::uint32_t*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_LONGLONG)
    {
        boost::int64_t* src = (boost::int64_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::int64_t*)p = src[i];
            p += data_stride;
        }
    }
    else if (pyDataType == PyArray_ULONGLONG)
    {
        boost::uint64_t* src = (boost::uint64_t*)PyArray_GetPtr(arr, &one);
        for (unsigned int i=0; i<data_len; i++)
        {
            *(boost::uint64_t*)p = src[i];
            p += data_stride;
        }
    }
    else
    {
        assert(0);
    }

    return;
}


int Invocation::getPythonDataType(dimension::Interpretation datatype, boost::uint32_t siz)
{
    switch (datatype)
    {
    case dimension::SignedByte:
        switch (siz)
        {
        case 1:
            return PyArray_BYTE;
        }
        break;
    case dimension::UnsignedByte:
        switch (siz)
        {
        case 1:
            return PyArray_UBYTE;
        }
        break;
    case dimension::Float:
        switch (siz)
        {
        case 4:
            return PyArray_FLOAT;
        case 8:
            return PyArray_DOUBLE;
        }
        break;
    case dimension::SignedInteger:
        switch (siz)
        {
        case 4:
            return PyArray_INT;
        case 8:
            return PyArray_LONGLONG;
        }
        break;
    case dimension::UnsignedInteger:
        switch (siz)
        {
        case 4:
            return PyArray_UINT;
        case 8:
            return PyArray_ULONGLONG;
        }
        break;
    }

    assert(0);

    return -1;
}


bool Invocation::hasOutputVariable(const std::string& name) const
{
   PyObject* obj = PyDict_GetItemString(m_varsOut, name.c_str());

   return (obj!=NULL);
}


bool Invocation::execute()
{
    if (!m_compile)
    {
        throw python_error("no code has been compiled");
    }

    Py_INCREF(m_varsIn);
    Py_INCREF(m_varsOut);
    m_scriptArgs = PyTuple_New(2);
    PyTuple_SetItem(m_scriptArgs, 0, m_varsIn);
    PyTuple_SetItem(m_scriptArgs, 1, m_varsOut);

    m_scriptResult = PyObject_CallObject(m_func, m_scriptArgs);
    if (!m_scriptResult) m_env.handleError();

    if (!PyBool_Check(m_scriptResult))
    {
        throw python_error("user function return value not a boolean type");
    }
    bool sts = false;
    if (m_scriptResult == Py_True)
    {
        sts = true;
    }

    return sts;
}



} } //namespaces

#endif