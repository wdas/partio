/**
PARTIO SOFTWARE
Copyright 2010 Disney Enterprises, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
Studios" or the names of its contributors may NOT be used to
endorse or promote products derived from this software without
specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/


%module partio
%include "std_string.i"

%{
#ifdef PARTIO_USE_NUMPY
#include <numpy/arrayobject.h> 
#endif

#include <Partio.h>
#include <PartioIterator.h>
namespace Partio{
typedef uint64_t ParticleIndex;
}

using namespace Partio;


// This is for passing fixed sized arrays
struct fixedFloatArray
{
    int count;
    Partio::ParticleAttributeType type; // should be float or VECTOR
    union{
        float f[16];
        int i[16];
    };
};
%}

%init %{
#ifdef PARTIO_USE_NUMPY
import_array();
#endif
%}

// Particle Types
enum ParticleAttributeType {NONE=0,VECTOR=1,FLOAT=2,INT=3,INDEXEDSTR=4};


%feature("docstring","A handle for operating on attribbutes of a particle set");
class ParticleAttribute
{
public:
    %feature("docstring","Type of the particle data (VECTOR,INT,FLOAT)");
    ParticleAttributeType type;

    %feature("docstring","Number of primitives (int's or float's)");
    int count;

    %feature("docstring","Attribute name");
    std::string name;

    // internal use
    //int attributeIndex; 
};


%typemap(in) uint64_t {
	$1 = (uint64_t) PyLong_AsLong($input);
}
%typemap(out) uint64_t {
	$result = PyLong_FromLong(
            (long)$1);
}
%apply uint64_t { ParticleIndex };

%typemap(in) fixedFloatArray
{
    int i;
    if(!PySequence_Check($input)){
        PyErr_SetString(PyExc_TypeError,"Expecting a sequence");
        return NULL;
    }
    int size=PyObject_Length($input);
    $1.count=size;
    for(i=0;i<size;i++){
        PyObject* o=PySequence_GetItem($input,i);
        if(!PyFloat_Check(o)){
            Py_XDECREF(o);
            PyErr_SetString(PyExc_ValueError,"Expecting a sequence of floats");
            return NULL;
        }
        $1.f[i]=PyFloat_AsDouble(o);
        Py_DECREF(o);
    } 
}

%typemap(out) fixedFloatArray
{
    PyObject* tuple=PyTuple_New($1.count);
    for(int k=0;k<$1.count;k++)
        PyTuple_SetItem(tuple,k,PyFloat_FromDouble($1.f[k]));
    $result=tuple;
}

%typemap(freearg) float* floatTupleIn
{
    delete [] $1;
}

%unrefobject ParticlesInfo "$this->release();"
%feature("autodoc");
%feature("docstring","A set of particles with associated data attributes.");
class ParticlesInfo
{
public:
    %feature("autodoc");
    %feature("docstring","Returns the number of particles in the set");
    virtual int numParticles() const=0;

    %feature("autodoc");
    %feature("docstring","Returns the number of particles in the set");
    virtual int numAttributes() const=0;
};



%unrefobject ParticlesData "$this->release();"
%feature("autodoc");
%feature("docstring","A reader for a set of particles.");
class ParticlesData:public ParticlesInfo
{
  public:
    %feature("autodoc");
    %feature("docstring","Looks up a given indexed string given the index, returns -1 if not found");
    int lookupIndexedStr(const ParticleAttribute& attribute,const char* str) const=0;

};

%rename(ParticleIteratorFalse) ParticleIterator<false>;
%rename(ParticleIteratorTrue) ParticleIterator<true>;
class ParticleIterator<true>
{};
class ParticleIterator<false>
{};

%unrefobject ParticlesDataMutable "$this->release();"
%feature("autodoc");
%feature("docstring","A writer for a set of particles.");
class ParticlesDataMutable:public ParticlesData
{
public:

    %feature("autodoc");
    %feature("docstring","Registers a string in the particular attribute");
    virtual int registerIndexedStr(const ParticleAttribute& attribute,const char* str)=0;


    %feature("autodoc");
    %feature("docstring","Prepares data for N nearest neighbor searches using the\n"
       "attribute in the file with name 'position'");
    virtual void sort()=0;

    %feature("autodoc");
    %feature("docstring","Adds a new attribute of given name, type and count. If type is\n"
        "partio.VECTOR, then count must be 3");
    virtual ParticleAttribute addAttribute(const char* attribute,ParticleAttributeType type,
        const int count)=0;

    %feature("autodoc");
    %feature("docstring","Adds a new particle and returns the index");
    virtual ParticleIndex addParticle()=0;

    %feature("autodoc");
    %feature("docstring","Adds count particles and returns the offset to the first one");
    virtual ParticleIterator<false> addParticles(const int count)=0;
};



%extend ParticlesData {
    %feature("autodoc");
    %feature("docstring","Searches for the N nearest points to the center location\n"
        "or as many as can be found within maxRadius distance.");
    PyObject* findNPoints(fixedFloatArray center,int nPoints,float maxRadius)
    {
        // make sure we are close
        if(center.count!=3){
            fprintf(stderr,"Need center to be a 3 tuple of floats\n");
            return NULL;
        }
        // find nearest neighbors
        std::vector<ParticleIndex> points;
        std::vector<float> pointDistancesSquared;
        $self->findNPoints(center.f,nPoints,maxRadius,points,pointDistancesSquared);
        
        // build the python return type
        PyObject* list=PyList_New(points.size());
        for(unsigned int i=0;i<points.size();i++){
            PyObject* tuple=Py_BuildValue("(if)",points[i],pointDistancesSquared[i]);
            PyList_SetItem(list,i,tuple); // tuple reference is stolen, so no decref needed
        }
        return list;
    }

    %feature("autodoc");
    %feature("docstring","Returns the indices of all points within the bounding\n"
        "box defined by the two cube corners bboxMin and bboxMax");
    PyObject* findPoints(fixedFloatArray bboxMin,fixedFloatArray bboxMax)
    {    
        // make sure we are close
        if(bboxMin.count!=3 || bboxMax.count!=3){
            fprintf(stderr,"Need bboxMin and bboxMax to be a 3 tuple of floats\n");
            return NULL;
        }
        // find nearest neighbors
        std::vector<ParticleIndex> points;
        $self->findPoints(bboxMin.f,bboxMax.f,points);
        
        // build the python return type
        PyObject* list=PyList_New(points.size());
        for(unsigned int i=0;i<points.size();i++){
            PyList_SetItem(list,i,PyInt_FromLong(points[i])); // tuple reference is stolen, so no decref needed
        }
        return list;
    }

#ifdef PARTIO_USE_NUMPY
    %feature("autodoc");
    %feature("docstring","Get particle data as a NumPy array");
    PyObject* getArray(const ParticleAttribute& attr)
    {    
        npy_intp dims[2] = { $self->numParticles(), attr.count };
        
        int npy_type;
        switch (attr.type) {
            case Partio::NONE:
            case Partio::INDEXEDSTR:
                Py_INCREF(Py_None);
                return Py_None;
                break;
            case Partio::FLOAT:
            case Partio::VECTOR:
                npy_type = NPY_FLOAT32;
                break;
            case Partio::INT:
                npy_type = NPY_INT32;
                break;
        }

        PyObject *array = PyArray_SimpleNew(2, dims, npy_type);
        
        if (!array) {
            PyErr_SetString(PyExc_TypeError,"Unable to create array");
            return NULL;
        }

        Partio::ParticlesDataMutable::const_iterator it = $self->begin();
        Partio::ParticleAccessor acc(attr);
        it.addAccessor(acc);

        switch (attr.type) {
            case Partio::NONE:
            case Partio::INDEXEDSTR:
                break;
            case Partio::FLOAT:
            case Partio::VECTOR:
            {
                float *dptr = (float *)PyArray_DATA(array);
                for(;it!=$self->end();++it){
                    for(int c=0;c<attr.count;c++) dptr[c] = acc.raw<float>(it)[c];
                    dptr += attr.count;
                }
                break;
            }
            case Partio::INT:
            {
                int *dptr = (int *)PyArray_DATA(array);
                for(;it!=$self->end();++it){
                    for(int c=0;c<attr.count;c++) dptr[c] = acc.raw<int>(it)[c];
                    dptr += attr.count;
                }
                break;
            }
        }

        return PyArray_Return((PyArrayObject *)array);
    }

    %feature("autodoc");
    %feature("docstring","Get");
    PyObject* getNDArray(const ParticleAttribute& attr)
    {    
        unsigned int numparticles = $self->numParticles();
        
        // 1 dimensional for now
        npy_intp dims[1] = { numparticles*attr.count };
        PyObject *array = PyArray_SimpleNew(1, dims, NPY_FLOAT);

        if (!array) {
            PyErr_SetString(PyExc_TypeError,"Unable to create array");
            return NULL;
        }

        npy_intp size;
        unsigned int i=0;
        float *dptr;
        size = PyArray_SIZE(array);
        dptr = (float *)PyArray_DATA(array);

        for (int j=0;j<size;j+=3) {
            const float* p=$self->data<float>(attr,i);
            for(int k=0;k<attr.count;k++) {
                dptr[0] = p[k];
                dptr++;
            }
            i++;
        }
 
        return PyArray_Return((PyArrayObject *)array);
    }
#endif

    %feature("autodoc");
    %feature("docstring","Gets a single flattened tuple, containing attribute data for all particles");
    PyObject* getArray(const ParticleAttribute& attr)
    {    
        unsigned int numparticles = $self->numParticles();
        PyObject* tuple=PyTuple_New(numparticles * attr.count);
        
        if(attr.type==Partio::INT){
            for(unsigned int i=0;i<numparticles;i++) {
                const int* p=$self->data<int>(attr,i);
                for(int k=0;k<attr.count;k++) PyTuple_SetItem(tuple, i*attr.count+k, PyInt_FromLong(p[k]));
            }
        }else{
            for(unsigned int i=0;i<numparticles;i++) {
                const float* p=$self->data<float>(attr,i);
                for(int k=0;k<attr.count;k++) PyTuple_SetItem(tuple, i*attr.count+k, PyFloat_FromDouble(p[k]));
            }
        }
        return tuple;
    }

    %feature("autodoc");
    %feature("docstring","Gets attribute data for particleIndex'th particle");
    PyObject* get(const ParticleAttribute& attr,const ParticleIndex particleIndex)
    {
        PyObject* tuple=PyTuple_New(attr.count);
        if(attr.type==Partio::INT || attr.type==Partio::INDEXEDSTR){
            const int* p=$self->data<int>(attr,particleIndex);
            for(int k=0;k<attr.count;k++) PyTuple_SetItem(tuple,k,PyInt_FromLong(p[k]));
        }else if(attr.type==Partio::FLOAT || attr.type==Partio::VECTOR){
            const float* p=$self->data<float>(attr,particleIndex);
            for(int k=0;k<attr.count;k++) PyTuple_SetItem(tuple,k,PyFloat_FromDouble(p[k]));
        }else{
            Py_XDECREF(tuple);
            PyErr_SetString(PyExc_ValueError,"Internal error unexpected data type");
            return NULL;
        }
        return tuple;
    }


    %feature("autodoc");
    %feature("docstring","Gets a list of all indexed strings for the given attribute handle");
    PyObject* indexedStrs(const ParticleAttribute& attr) const
    {
        const std::vector<std::string>& indexes=self->indexedStrs(attr);
        PyObject* list=PyList_New(indexes.size());
        for(size_t k=0;k<indexes.size();k++) PyList_SetItem(list,k,PyString_FromString(indexes[k].c_str()));
        return list;
    }
}

%extend ParticlesDataMutable {

    %feature("autodoc");
    %feature("docstring","Sets data on a given attribute for a single particle.\n"
        "Data must be specified as tuple.");
    PyObject* set(const ParticleAttribute& attr,const uint64_t particleIndex,PyObject* tuple)
    {
        if(!PySequence_Check(tuple)){
            PyErr_SetString(PyExc_TypeError,"Expecting a sequence");
            return NULL;
        }
        int size=PyObject_Length(tuple);
        if(size!=attr.count){
            PyErr_SetString(PyExc_ValueError,"Invalid number of parameters");
            return NULL;
        }
        
        if(attr.type==Partio::INT || attr.type==Partio::INDEXEDSTR){
            int* p=$self->dataWrite<int>(attr,particleIndex);
            for(int i=0;i<size;i++){
                PyObject* o=PySequence_GetItem(tuple,i);
                if(PyInt_Check(o)){
                    p[i]=PyInt_AsLong(o);
                }else{
                    Py_XDECREF(o);
                    PyErr_SetString(PyExc_ValueError,"Expecting a sequence of ints");
                    return NULL;
                }
                Py_XDECREF(o);
            }
        }else if(attr.type==Partio::FLOAT || attr.type==Partio::VECTOR){
            float* p=$self->dataWrite<float>(attr,particleIndex);
            for(int i=0;i<size;i++){
                PyObject* o=PySequence_GetItem(tuple,i);
                //fprintf(stderr,"checking %d\n",i);
                if(PyFloat_Check(o)){
                    p[i]=PyFloat_AsDouble(o);
                }else if(PyInt_Check(o)){
                    p[i]=(float)PyInt_AsLong(o);
                }else{
                    Py_XDECREF(o);
                    PyErr_SetString(PyExc_ValueError,"Expecting a sequence of floats or ints");
                    return NULL;
                }
                Py_XDECREF(o);
            }
        }else{
            PyErr_SetString(PyExc_ValueError,"Internal error: invalid attribute type");
            return NULL;
        }

        Py_INCREF(Py_None);
        return Py_None;
    }

#ifdef PARTIO_USE_NUMPY
    %feature("autodoc");
    %feature("docstring","Set particle data from a NumPy array. \n"
        "Input array will be converted to data type numpy.float32 or numpy.int32 \n"
        "depending on attribute type. Input array length will also need to be divisible \n"
        "by the attribute size, so it can be reshaped to fit. \n");
    PyObject* setArray(const ParticleAttribute& attr, PyObject *input_array)
    {   
        if (!PyArray_Check(input_array)) {
            PyErr_SetString(PyExc_TypeError,"Invalid input array");
            return NULL;
        }

        switch (attr.type) {
            case Partio::NONE:
            case Partio::INDEXEDSTR:
                break;
            case Partio::FLOAT:
            case Partio::VECTOR:
                if (PyArray_TYPE(input_array) != NPY_FLOAT32) {     // cast to float32 data type
                    PyObject *numpy = PyImport_ImportModule("numpy");
                    PyObject *f32 = PyObject_GetAttrString(numpy, "float32");
                    input_array = PyObject_CallMethod(input_array, "astype", "O", f32);
                    Py_DECREF(numpy);
                    Py_DECREF(f32);
                }
                break;
            case Partio::INT:
                if (PyArray_TYPE(input_array) != NPY_INT32) {   // cast to int32 data type
                    PyObject *numpy = PyImport_ImportModule("numpy");
                    PyObject *i32 = PyObject_GetAttrString(numpy, "int32");
                    input_array = PyObject_CallMethod(input_array, "astype", "O", i32);
                    Py_DECREF(numpy);
                    Py_DECREF(i32);
                }
                break;
        }

        // reshape to 2d array, rows of attribute size
        PyObject *array = PyObject_CallMethod(input_array, "reshape", "(ii)", -1, attr.count);
        if (!array || !PyArray_Check(array)) {
            PyErr_Format(PyExc_ValueError, "Unable to reshape array to row size: %d. Total size of new array must be unchanged", attr.count);
            return NULL;
        }

        // iterate over the minimum of available particles and the number of array items / attr count
        unsigned int array_rows = (unsigned int)PyArray_DIM(array, 2);
        unsigned int i=0, numparticles=$self->numParticles();
        unsigned int numcopies = array_rows<numparticles? array_rows:numparticles;
        
        // copy data from the array to the particle attribute
        switch (attr.type) {
            case Partio::NONE:
            case Partio::INDEXEDSTR:
                break;
            case Partio::FLOAT:
            case Partio::VECTOR:
                for(i=0; i<numcopies; i++){
                    float* v=$self->dataWrite<float>(attr,i);
                    for (int j=0;j<attr.count;j++) {
                        v[j] = *(float *)PyArray_GETPTR2(array, (npy_intp)i, j);
                    }
                }
                break;
            case Partio::INT:
                for(i=0; i<numcopies; i++){
                    int* v=$self->dataWrite<int>(attr,i);
                    for (int j=0;j<attr.count;j++) {
                        v[j] = *(int *)PyArray_GETPTR2(array, (npy_intp)i, j);
                    }
                }
                break;
        }
        Py_DECREF(array);

        return Py_None;
    }
#endif

}

%extend ParticlesInfo {

    %feature("autodoc");
    %feature("docstring","Searches for and returns the attribute handle for a named attribute");
    %newobject attributeInfo;
    ParticleAttribute* attributeInfo(const char* name)
    {
        ParticleAttribute a;
        bool valid=$self->attributeInfo(name,a);
        if(valid) return new ParticleAttribute(a);
        else return 0;
    }

    %feature("autodoc");
    %feature("docstring","Returns the attribute handle by index");
    %newobject attributeInfo;
    ParticleAttribute* attributeInfo(const int index)
    {
        if(index<0 || index>=$self->numAttributes()){
            PyErr_SetString(PyExc_IndexError,"Invalid attribute index");
            return NULL;
        }
        ParticleAttribute a;
        bool valid=$self->attributeInfo(index,a);
        if(valid) return new ParticleAttribute(a);
        else return 0;
    }
}

%feature("autodoc");
%feature("docstring","Create an empty particle array");
%newobject create;
ParticlesDataMutable* create();

%feature("autodoc");
%feature("docstring","Reads a particle set from disk");
%newobject read;
ParticlesDataMutable* read(const char* filename);

%feature("autodoc");
%feature("docstring","Reads a particle set headers from disk");
%newobject readHeaders;
ParticlesInfo* readHeaders(const char* filename);

%feature("autodoc");
%feature("docstring","Writes a particle set to disk");
void write(const char* filename,const ParticlesData&,const bool=false);

%feature("autodoc");
%feature("docstring","Print a summary of particle file");
void print(const ParticlesData* particles);
