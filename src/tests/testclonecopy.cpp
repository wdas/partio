/*
PARTIO SOFTWARE
Copyright 2015 Disney Enterprises, Inc. All rights reserved

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


#include <Partio.h>
#include <iostream>
#include <cstdlib>
#include <sstream>

// TODO grab a copy of some header-only MIT/BSD tap library
void success(int& test, const std::string& msg)
{
    std::cout << "ok " << test++ << " - " << msg << std::endl;
}

void fail(int& test, const std::string& msg)
{
    std::cerr << "not ok " << test++ << " - " << msg << std::endl;
    exit(EXIT_FAILURE);
}

void check(int& test, bool ok, const std::string& msg, const std::string& err)
{
    if (ok) {
        success(test, msg);
    } else {
        fail(test, msg + ": " + err);
    }
}

template<typename T>
std::string to_string(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

Partio::ParticlesDataMutable* makeData()
{
    Partio::ParticlesDataMutable& foo=*Partio::create();

    Partio::FixedAttribute originAttr=foo.addFixedAttribute("origin",Partio::VECTOR,3);
    Partio::FixedAttribute uvAttr=foo.addFixedAttribute("uv",Partio::FLOAT,2);
    Partio::FixedAttribute sid=foo.addFixedAttribute("sid",Partio::INT,1);

    Partio::ParticleAttribute positionAttr=foo.addAttribute("position",Partio::VECTOR,3);
    Partio::ParticleAttribute lifeAttr=foo.addAttribute("life",Partio::FLOAT,2);
    Partio::ParticleAttribute idAttr=foo.addAttribute("id",Partio::INT,1);

    for(int i=0;i<5;i++){
        Partio::ParticleIndex index=foo.addParticle();
        float* pos=foo.dataWrite<float>(positionAttr,index);
        float* life=foo.dataWrite<float>(lifeAttr,index);
        int* id=foo.dataWrite<int>(idAttr,index);

        pos[0]=.1*i;
        pos[1]=.1*(i+1);
        pos[2]=.1*(i+2);
        life[0]=-1.2+i;
        life[1]=10.;
        id[0]=index;
    }

    return &foo;
}


void testCloneParticleFixedAttributes(Partio::ParticlesData* other, int& test)
{
    Partio::ParticlesData* p=Partio::cloneSchema(*other);

    int numFixedAttributes = p->numFixedAttributes();
    check(test, numFixedAttributes == 3,
          "cloneSchema()->numFixedAttributes()",
          "expected 3, got "+to_string(numFixedAttributes));

    std::vector<std::string> expectedName(numFixedAttributes);
    std::vector<Partio::ParticleAttributeType> expectedType(numFixedAttributes);
    std::vector<int> expectedCount(numFixedAttributes);

    expectedName[0] = "origin";
    expectedType[0] = Partio::VECTOR;
    expectedCount[0] = 3;

    expectedName[1] = "uv";
    expectedType[1] = Partio::FLOAT;
    expectedCount[1] = 2;

    expectedName[2] = "sid";
    expectedType[2] = Partio::INT;
    expectedCount[2] = 1;

    // Test attributes
    for (int i=0;i<numFixedAttributes;++i) {
        Partio::FixedAttribute attr;
        p->fixedAttributeInfo(i, attr);

        check(test, attr.name == expectedName[i],
              "attribute name for " + attr.name,
              "expected " + expectedName[i] +
              ", got " + attr.name);

        check(test, attr.type == expectedType[i],
              "attribute type for " + attr.name,
              "expected " + Partio::TypeName(expectedType[i]) +
              ", got " + Partio::TypeName(attr.type));

        check(test, attr.count == expectedCount[i],
              "fixed attribute count for " + attr.name,
              "expected " + to_string(expectedCount[i]) +
              ", got " + to_string(attr.count));
    }

    p->release();
}


void testCloneParticleDataAttributes(Partio::ParticlesData* other, int& test)
{
    Partio::ParticlesData* p=Partio::cloneSchema(*other);

    int numAttributes = p->numAttributes();
    check(test, numAttributes == 3, "cloneSchema()->numAttributes()",
          "expected 3, got " + to_string(numAttributes));

    int numParticles = p->numParticles();
    check(test, numParticles == 0, "clone contains no particles",
          "expected 0, got " + to_string(numParticles));

    std::vector<std::string> expectedName(numAttributes);
    std::vector<Partio::ParticleAttributeType> expectedType(numAttributes);
    std::vector<int> expectedCount(numAttributes);

    expectedName[0] = "position";
    expectedType[0] = Partio::VECTOR;
    expectedCount[0] = 3;

    expectedName[1] = "life";
    expectedType[1] = Partio::FLOAT;
    expectedCount[1] = 2;

    expectedName[2] = "id";
    expectedType[2] = Partio::INT;
    expectedCount[2] = 1;

    for (int i=0;i<numAttributes;++i) {
        Partio::ParticleAttribute attr;
        p->attributeInfo(i,attr);

        check(test, attr.name == expectedName[i],
              "attribute name for " + attr.name,
              "expected " + expectedName[i] + ", got " + attr.name);

        check(test, attr.type == expectedType[i],
              "attribute type for " + attr.name,
              "expected " + to_string(expectedType[i]) +
              ", got " + to_string(attr.type));

        check(test, attr.count == expectedCount[i],
              "attribute count for " + attr.name,
              "expected " + to_string(expectedCount[i]) +
              ", got " + to_string(attr.count));
    }

    p->release();
}

void testCloneParticleDataFullCopy(int& test)
{
    Partio::ParticlesDataMutable* other = makeData();

    // Add fixed string attributes
    Partio::FixedAttribute str = other->addFixedAttribute("str", Partio::INDEXEDSTR, 2);
    int value0 = other->registerFixedIndexedStr(str, "value 0");
    int value1 = other->registerFixedIndexedStr(str, "value 1");
    int* strdata = other->fixedDataWrite<int>(str);
    strdata[0] = value0;
    strdata[1] = value1;

    Partio::FixedAttribute fattr = other->addFixedAttribute("float", Partio::FLOAT, 2);
    float* fdata = other->fixedDataWrite<float>(fattr);
    fdata[0] = 1.0;
    fdata[1] = 2.0;

    Partio::ParticlesData* p = Partio::clone(*other);

    check(test, p->numFixedAttributes() == other->numFixedAttributes(),
          "clone() preserves fixed attribute count",
          "expected " + to_string(other->numFixedAttributes()) +
          ", got " + to_string(p->numFixedAttributes()));

    Partio::FixedAttribute cloneStr;
    check(test, p->fixedAttributeInfo("str", cloneStr),
          "str attribute lookup returns true", "expected true, got false");

    check(test, cloneStr.count == 2, "fixed strings are cloned",
          "expected 2, got " + to_string(cloneStr.count));

    const std::vector<std::string>& strings = p->fixedIndexedStrs(cloneStr);
    check(test, strings[0] == "value 0",
          "indexed string value 1", "expected value 1, got " + strings[0]);
    check(test, strings[1] == "value 1",
          "indexed string value 2", "expected value 2, got " + strings[1]);

    const int* stringvals = p->fixedData<int>(cloneStr);
    check(test, stringvals[0] == value0 && stringvals[1] == value1,
          "fixed string data", "expected 0, 1, got " +
          to_string(stringvals[0]) + ", " + to_string(stringvals[1]));

    Partio::FixedAttribute cfloat;
    check(test, p->fixedAttributeInfo("float", cfloat),
          "float attribute lookup returns true", "expected true, got false");

    const float* cfdata = p->fixedData<float>(cfloat);
    check(test, cfdata[0] == 1.0 && cfdata[1] == 2.0,
          "fixed float data", "expected (1.0, 2.0), got " +
          to_string(cfdata[0]) + ", " + to_string(cfdata[1]));

    check(test, p->numAttributes() == 3,
          "attributes are cloned", "expected 3, got " +
          to_string(p->numAttributes()));

    check(test, p->numParticles() == 5,
          "particles are cloned", "expected 5, got " +
          to_string(p->numParticles()));

    Partio::ParticleAttribute id;
    check(test, p->attributeInfo("id", id),
          "id attribute exists", "expected true, got false");

    Partio::ParticleAttribute life;
    check(test, p->attributeInfo("life", life),
          "life attribute exists", "expected true, got false");

    for (int i = 0; i < 5; ++i) {
        const int* idvals = p->data<int>(id, i);
        check(test, idvals[0] == i,
              "particle id " + to_string(i) + " is copied",
              "expected " + to_string(i) +
              ", got " + to_string(idvals[0]));

        const float* lifevals = p->data<float>(life, i);
        check(test, lifevals[1] == 10.,
              "particle life " + to_string(i) + " is copied",
              "expected 10.0, got " + to_string(lifevals[1]));
    }

    p->release();
}


int main(int argc,char *argv[])
{
    int test = 1;

    // Expected number of tests
    std::cout << "1..43" << std::endl;

    Partio::ParticlesDataMutable* foo=makeData();
    {
        testCloneParticleFixedAttributes(foo, test);
        testCloneParticleDataAttributes(foo, test);
        testCloneParticleDataFullCopy(test);
    }
    foo->release();

    return 0;
}
