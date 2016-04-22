/*
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

#ifdef PARTIO_WIN32
#    define NOMINMAX
#endif

#define __STDC_LIMIT_MACROS
#include "ParticleSimple.h"
#include "ParticleCaching.h"
#include <PartioVec3.h>
#include <map>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "KdTree.h"


using namespace Partio;

ParticlesSimple::
ParticlesSimple()
    :particleCount(0),allocatedCount(0),kdtree(0)
{
}

ParticlesSimple::
~ParticlesSimple()
{
    for(unsigned int i=0;i<attributeData.size();i++) free(attributeData[i]);
    for(unsigned int i=0;i<fixedAttributeData.size();i++) free(fixedAttributeData[i]);
    delete kdtree;
}

void ParticlesSimple::
release() const
{
    freeCached(const_cast<ParticlesSimple*>(this));
}

int ParticlesSimple::
numParticles() const
{
    return particleCount;
}

int ParticlesSimple::
numAttributes() const
{
    return attributes.size();
}

int ParticlesSimple::
numFixedAttributes() const
{
    return fixedAttributes.size();
}

bool ParticlesSimple::
attributeInfo(const int attributeIndex,ParticleAttribute& attribute) const
{
    if(attributeIndex<0 || attributeIndex>=(int)attributes.size()) return false;
    attribute=attributes[attributeIndex];
    return true;
}

bool ParticlesSimple::
fixedAttributeInfo(const int attributeIndex,FixedAttribute& attribute) const
{
    if(attributeIndex<0 || attributeIndex>=(int)fixedAttributes.size()) return false;
    attribute=fixedAttributes[attributeIndex];
    return true;
}

bool ParticlesSimple::
attributeInfo(const char* attributeName,ParticleAttribute& attribute) const
{
    std::map<std::string,int>::const_iterator it=nameToAttribute.find(attributeName);
    if(it!=nameToAttribute.end()){
        attribute=attributes[it->second];
        return true;
    }
    return false;
}

bool ParticlesSimple::
fixedAttributeInfo(const char* attributeName,FixedAttribute& attribute) const
{
    std::map<std::string,int>::const_iterator it=nameToFixedAttribute.find(attributeName);
    if(it!=nameToFixedAttribute.end()){
        attribute=fixedAttributes[it->second];
        return true;
    }
    return false;
}

void ParticlesSimple::
sort()
{
    ParticleAttribute attr;
    bool foundPosition=attributeInfo("position",attr);
    if(!foundPosition){
        std::cerr<<"Partio: sort, Failed to find position in particle"<<std::endl;
        return;
    }else if(attr.type!=VECTOR || attr.count!=3){
        std::cerr<<"Partio: sort, position attribute is not a vector of size 3"<<std::endl;
        return;
    }

    const ParticleIndex baseParticleIndex=0;
    const float* data=this->data<float>(attr,baseParticleIndex); // contiguous assumption used here
    KdTree<3>* kdtree_temp=new KdTree<3>();
    kdtree_temp->setPoints(data,numParticles());
    kdtree_temp->sort();

    kdtree_mutex.lock();
    // TODO: this is not threadsafe!
    if(kdtree) delete kdtree;
    kdtree=kdtree_temp;
    kdtree_mutex.unlock();
}

void ParticlesSimple::
findPoints(const float bboxMin[3],const float bboxMax[3],std::vector<ParticleIndex>& points) const
{
    if(!kdtree){
        std::cerr<<"Partio: findPoints without first calling sort()"<<std::endl;
        return;
    }

    BBox<3> box(bboxMin);box.grow(bboxMax);

    int startIndex=points.size();
    kdtree->findPoints(points,box);
    // remap points found in findPoints to original index space
    for(unsigned int i=startIndex;i<points.size();i++){
        points[i]=kdtree->id(points[i]);
    }
}

float ParticlesSimple::
findNPoints(const float center[3],const int nPoints,const float maxRadius,std::vector<ParticleIndex>& points,
    std::vector<float>& pointDistancesSquared) const
{
    if(!kdtree){
        std::cerr<<"Partio: findNPoints without first calling sort()"<<std::endl;
        return 0;
    }

    //assert(sizeof(ParticleIndex)==sizeof(uint64_t));
    //std::vector<uint64_t>& rawPoints=points;
    float maxDistance=kdtree->findNPoints(points,pointDistancesSquared,center,nPoints,maxRadius);
    // remap all points since findNPoints clears array
    for(unsigned int i=0;i<points.size();i++){
        ParticleIndex index=kdtree->id(points[i]);
        points[i]=index;
    }
    return maxDistance;
}

int ParticlesSimple::
findNPoints(const float center[3],int nPoints,const float maxRadius, ParticleIndex *points,
    float *pointDistancesSquared, float *finalRadius2) const
{
    if(!kdtree){
        std::cerr<<"Partio: findNPoints without first calling sort()"<<std::endl;
        return 0;
    }

    int count = kdtree->findNPoints (points, pointDistancesSquared, finalRadius2, center, nPoints, maxRadius);
    // remap all points since findNPoints clears array
    for(int i=0; i < count; i++){
        ParticleIndex index = kdtree->id(points[i]);
        points[i]=index;
    }
    return count;
}

double hash(int n, double* args)
{
    // combine args into a single seed
    uint32_t seed = 0;
    for (int i = 0; i < n; i++) {
        // make irrational to generate fraction and combine xor into 32 bits
        int exp=0;
        double frac = std::frexp(args[i] * double(M_E*M_PI), &exp);
        uint32_t s = (uint32_t) (frac * UINT32_MAX) ^ (uint32_t) exp;

        // blend with seed (constants from Numerical Recipes, attrib. from Knuth)
        static const uint32_t M = 1664525, C = 1013904223;
        seed = seed * M + s + C;
    }

    // tempering (from Matsumoto)
    seed ^= (seed >> 11);
    seed ^= (seed << 7) & 0x9d2c5680UL;
    seed ^= (seed << 15) & 0xefc60000UL;
    seed ^= (seed >> 18);

    // permute
    static unsigned char p[256] = {
        148,201,203,34,85,225,163,200,174,137,51,24,19,252,107,173,
        110,251,149,69,180,152,141,132,22,20,147,219,37,46,154,114,
        59,49,155,161,239,77,47,10,70,227,53,235,30,188,143,73,
        88,193,214,194,18,120,176,36,212,84,211,142,167,57,153,71,
        159,151,126,115,229,124,172,101,79,183,32,38,68,11,67,109,
        221,3,4,61,122,94,72,117,12,240,199,76,118,5,48,197,
        128,62,119,89,14,45,226,195,80,50,40,192,60,65,166,106,
        90,215,213,232,250,207,104,52,182,29,157,103,242,97,111,17,
        8,175,254,108,208,224,191,112,105,187,43,56,185,243,196,156,
        246,249,184,7,135,6,158,82,130,234,206,255,160,236,171,230,
        42,98,54,74,209,205,33,177,15,138,178,44,116,96,140,253,
        233,125,21,133,136,86,245,58,23,1,75,165,92,217,39,0,
        218,91,179,55,238,170,134,83,25,189,216,100,129,150,241,210,
        123,99,2,164,16,220,121,139,168,64,190,9,31,228,95,247,
        244,81,102,145,204,146,26,87,113,198,181,127,237,169,28,93,
        27,41,231,248,78,162,13,186,63,66,131,202,35,144,222,223};
    union {
        uint32_t i;
        unsigned char c[4];
    } u1, u2;
    u1.i = seed;
    u2.c[3] = p[u1.c[0]];
    u2.c[2] = p[(u1.c[1]+u2.c[3])&0xff];
    u2.c[1] = p[(u1.c[2]+u2.c[2])&0xff];
    u2.c[0] = p[(u1.c[3]+u2.c[1])&0xff];

    // scale to [0.0 .. 1.0]
    return u2.i * (1.0/UINT32_MAX);
}

struct IdAndIndex {
    IdAndIndex(int id, int index) : _id(id), _index(index) {}
    int _id, _index;
    bool operator<(const IdAndIndex &other) const {
        return _id < other._id;
    }
};

template<class T> T smoothstep(T t){return (3.-2.*t)*t*t;}

ParticlesDataMutable* ParticlesSimple::
computeClustering(const int numNeighbors,const double radiusSearch,const double radiusInside,const int connections,const double density)
{
    ParticleAttribute posAttr;
    if (!attributeInfo("position", posAttr)) return 0;
    if (posAttr.type != VECTOR && posAttr.type != FLOAT) return 0;
    if (posAttr.count != 3) return 0;
    ParticleAttribute idAttr;
    if (!attributeInfo("id", idAttr)) return 0;
    if (idAttr.type != INT) return 0;
    if (idAttr.count != 1) return 0;
    sort();
    ParticlesDataMutable* cluster = create();
    ParticleAttribute clusterPosAttr = cluster->addAttribute("position",VECTOR,3);
    for (int index=0; index<numParticles(); index++) {
        const float* center=data<float>(posAttr,index);
        Vec3 position(center[0], center[1], center[2]);
        int id = data<int>(idAttr,index)[0];
        double radius = std::max(0., radiusInside);
        radius = std::min(100., radiusInside);
        double innerRadius = .01 * radius * radiusSearch;
        double invRadius = 1 / (radiusSearch - innerRadius);

        std::vector<Partio::ParticleIndex> points;
        std::vector<float> distSq;
        findNPoints(center, numNeighbors, radiusSearch, points, distSq);

        std::vector<IdAndIndex> idAndIndex;
        idAndIndex.reserve(points.size());
        idAndIndex.push_back(IdAndIndex(id, 0));
        for (unsigned int i = 0; i < points.size(); i++) {
            const int pointid = data<int>(idAttr,points[i])[0];
            if (pointid != id) idAndIndex.push_back(IdAndIndex(pointid, points[i]));
        }
        std::sort(++idAndIndex.begin(), idAndIndex.end());

        double hashArgs[3];
        hashArgs[0] = id;

        int foundConnections = std::min((int) idAndIndex.size(), connections+1);
        for (int i = 1; i < foundConnections; i++) {
            const float* neighbor=data<float>(posAttr,idAndIndex[i]._index);
            Vec3 neighborPosition(neighbor[0], neighbor[1], neighbor[2]);
            Vec3 dir = neighborPosition - position;

            // calculate number of instances based on density
            int numInstances = 0;
            double len = dir.length();
            if (len < innerRadius) {
                numInstances = density * len;
            } else {
                numInstances = density * len * smoothstep(1.-(len-innerRadius)*invRadius);
            }
            for (int j = 0; j < numInstances; j++) {
                hashArgs[1] = idAndIndex[i]._id;
                hashArgs[2] = j;
                Vec3 randpos = position + hash(3, hashArgs) * dir;
                ParticleIndex clusterIndex = cluster->addParticle();
                float* pos = cluster->dataWrite<float>(clusterPosAttr,clusterIndex);
                pos[0]=randpos.x;
                pos[1]=randpos.y;
                pos[2]=randpos.z;
            }
        }
    }
    return cluster;;
}

ParticleAttribute ParticlesSimple::
addAttribute(const char* attribute,ParticleAttributeType type,const int count)
{
    if(nameToAttribute.find(attribute) != nameToAttribute.end()){
        std::cerr<<"Partio: addAttribute failed because attr '"<<attribute<<"'"<<" already exists"<<std::endl;
        return ParticleAttribute();
    }
    // TODO: check if attribute already exists and if so what data type
    ParticleAttribute attr;
    attr.name=attribute;
    attr.type=type;
    attr.attributeIndex=attributes.size(); //  all arrays separate so we don't use this here!
    attr.count=count;
    attributes.push_back(attr);
    nameToAttribute[attribute]=attributes.size()-1;

    int stride=TypeSize(type)*count;
    attributeStrides.push_back(stride);
    char* dataPointer=(char*)malloc((size_t)allocatedCount*(size_t)stride);
    attributeData.push_back(dataPointer);
    attributeOffsets.push_back(dataPointer-(char*)0);
    attributeIndexedStrs.push_back(IndexedStrTable());

    return attr;
}

FixedAttribute ParticlesSimple::
addFixedAttribute(const char* attribute,ParticleAttributeType type,const int count)
{
    if(nameToFixedAttribute.find(attribute) != nameToFixedAttribute.end()){
        std::cerr<<"Partio: addFixedAttribute failed because attr '"<<attribute<<"'"<<" already exists"<<std::endl;
        return FixedAttribute();
    }
    // TODO: check if attribute already exists and if so what data type
    FixedAttribute attr;
    attr.name=attribute;
    attr.type=type;
    attr.attributeIndex=fixedAttributes.size(); //  all arrays separate so we don't use this here!
    attr.count=count;
    fixedAttributes.push_back(attr);
    nameToFixedAttribute[attribute]=fixedAttributes.size()-1;

    int stride=TypeSize(type)*count;
    char* dataPointer=(char*)malloc(stride);
    fixedAttributeData.push_back(dataPointer);
    fixedAttributeIndexedStrs.push_back(IndexedStrTable());

    return attr;
}

ParticleIndex ParticlesSimple::
addParticle()
{
    if(allocatedCount==particleCount){
        allocatedCount=std::max(10,std::max(allocatedCount*3/2,particleCount));
        for(unsigned int i=0;i<attributes.size();i++)
            attributeData[i]=(char*)realloc(attributeData[i],(size_t)attributeStrides[i]*(size_t)allocatedCount);
    }
    ParticleIndex index=particleCount;
    particleCount++;
    return index;
}

ParticlesDataMutable::iterator ParticlesSimple::
addParticles(const int countToAdd)
{
    if(particleCount+countToAdd>allocatedCount){
        // TODO: this should follow 2/3 rule
        allocatedCount=allocatedCount+countToAdd;
        for(unsigned int i=0;i<attributes.size();i++){
            attributeData[i]=(char*)realloc(attributeData[i],(size_t)attributeStrides[i]*(size_t)allocatedCount);
            attributeOffsets[i]=attributeData[i]-(char*)0;
        }
    }
    int offset=particleCount;
    particleCount+=countToAdd;
    return setupIterator(offset);
}

ParticlesDataMutable::iterator ParticlesSimple::
setupIterator(const int index)
{
    if(numParticles()==0) return ParticlesDataMutable::iterator();
    return ParticlesDataMutable::iterator(this,index,numParticles()-1);
}

ParticlesData::const_iterator ParticlesSimple::
setupConstIterator(const int index) const
{
    if(numParticles()==0) return ParticlesDataMutable::const_iterator();
    return ParticlesData::const_iterator(this,index,numParticles()-1);
}

void ParticlesSimple::
setupIteratorNextBlock(Partio::ParticleIterator<false>& iterator)
{
    iterator=end();
}

void ParticlesSimple::
setupIteratorNextBlock(Partio::ParticleIterator<true>& iterator) const
{
    iterator=ParticlesData::end();
}


void ParticlesSimple::
setupAccessor(Partio::ParticleIterator<false>& iterator,ParticleAccessor& accessor)
{
    accessor.stride=accessor.count*sizeof(float);
    accessor.basePointer=attributeData[accessor.attributeIndex];
}

void ParticlesSimple::
setupAccessor(Partio::ParticleIterator<true>& iterator,ParticleAccessor& accessor) const
{
    accessor.stride=accessor.count*sizeof(float);
    accessor.basePointer=attributeData[accessor.attributeIndex];
}

void* ParticlesSimple::
dataInternal(const ParticleAttribute& attribute,const ParticleIndex particleIndex) const
{
    assert(attribute.attributeIndex>=0 && attribute.attributeIndex<(int)attributes.size());
    return attributeData[attribute.attributeIndex]+attributeStrides[attribute.attributeIndex]*particleIndex;
}

void* ParticlesSimple::
fixedDataInternal(const FixedAttribute& attribute) const
{
    assert(attribute.attributeIndex>=0 && attribute.attributeIndex<(int)fixedAttributes.size());
    return fixedAttributeData[attribute.attributeIndex];
}

void ParticlesSimple::
dataInternalMultiple(const ParticleAttribute& attribute,const int indexCount,
    const ParticleIndex* particleIndices,const bool sorted,char* values) const
{
    assert(attribute.attributeIndex>=0 && attribute.attributeIndex<(int)attributes.size());

    char* base=attributeData[attribute.attributeIndex];
    int bytes=attributeStrides[attribute.attributeIndex];
    for(int i=0;i<indexCount;i++)
        memcpy(values+bytes*i,base+particleIndices[i]*bytes,bytes);
}

void ParticlesSimple::
dataAsFloat(const ParticleAttribute& attribute,const int indexCount,
    const ParticleIndex* particleIndices,const bool sorted,float* values) const
{
    assert(attribute.attributeIndex>=0 && attribute.attributeIndex<(int)attributes.size());

    if(attribute.type==FLOAT || attribute.type==VECTOR) dataInternalMultiple(attribute,indexCount,particleIndices,sorted,(char*)values);
    else if(attribute.type==INT || attribute.type==INDEXEDSTR){
        char* attrrawbase=attributeData[attribute.attributeIndex];
        int* attrbase=(int*)attrrawbase;
        int count=attribute.count;
        for(int i=0;i<indexCount;i++) for(int k=0;k<count;k++) values[i*count+k]=(int)attrbase[particleIndices[i]*count+k];
    }
}

int ParticlesSimple::
registerIndexedStr(const ParticleAttribute& attribute,const char* str)
{
    IndexedStrTable& table=attributeIndexedStrs[attribute.attributeIndex];
    std::map<std::string,int>::const_iterator it=table.stringToIndex.find(str);
    if(it!=table.stringToIndex.end()) return it->second;
    int newIndex=table.strings.size();
    table.strings.push_back(str);
    table.stringToIndex[str]=newIndex;
    return newIndex;
}

int ParticlesSimple::
registerFixedIndexedStr(const FixedAttribute& attribute,const char* str)
{
    IndexedStrTable& table=fixedAttributeIndexedStrs[attribute.attributeIndex];
    std::map<std::string,int>::const_iterator it=table.stringToIndex.find(str);
    if(it!=table.stringToIndex.end()) return it->second;
    int newIndex=table.strings.size();
    table.strings.push_back(str);
    table.stringToIndex[str]=newIndex;
    return newIndex;
}

int ParticlesSimple::
lookupIndexedStr(Partio::ParticleAttribute const &attribute, char const *str) const
{
    const IndexedStrTable& table=attributeIndexedStrs[attribute.attributeIndex];
    std::map<std::string,int>::const_iterator it=table.stringToIndex.find(str);
    if(it!=table.stringToIndex.end()) return it->second;
    return -1;
}

int ParticlesSimple::
lookupFixedIndexedStr(Partio::FixedAttribute const &attribute, char const *str) const
{
    const IndexedStrTable& table=fixedAttributeIndexedStrs[attribute.attributeIndex];
    std::map<std::string,int>::const_iterator it=table.stringToIndex.find(str);
    if(it!=table.stringToIndex.end()) return it->second;
    return -1;
}

const std::vector<std::string>& ParticlesSimple::
indexedStrs(const ParticleAttribute& attr) const
{
    const IndexedStrTable& table=attributeIndexedStrs[attr.attributeIndex];
    return table.strings;
}

const std::vector<std::string>& ParticlesSimple::
fixedIndexedStrs(const FixedAttribute& attr) const
{
    const IndexedStrTable& table=fixedAttributeIndexedStrs[attr.attributeIndex];
    return table.strings;
}

void ParticlesSimple::setIndexedStr(const ParticleAttribute& attribute,int indexedStringToken,const char* str){
    IndexedStrTable& table=attributeIndexedStrs[attribute.attributeIndex];
    if(indexedStringToken >= int(table.strings.size()) || indexedStringToken < 0) return;
    table.stringToIndex.erase(table.stringToIndex.find(table.strings[indexedStringToken]));
    table.strings[indexedStringToken] = str;
    table.stringToIndex[str]=indexedStringToken;
}

void ParticlesSimple::setFixedIndexedStr(const FixedAttribute& attribute,int indexedStringToken,const char* str){
    IndexedStrTable& table=fixedAttributeIndexedStrs[attribute.attributeIndex];
    if(indexedStringToken >= int(table.strings.size()) || indexedStringToken < 0) return;
    table.stringToIndex.erase(table.stringToIndex.find(table.strings[indexedStringToken]));
    table.strings[indexedStringToken] = str;
    table.stringToIndex[str]=indexedStringToken;
}
