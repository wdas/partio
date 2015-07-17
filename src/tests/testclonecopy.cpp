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
    if (numFixedAttributes == 3) {
        std::cout << "ok " << test++
                  << " - Partio::cloneSchema() numFixedAttributes()" << std::endl;
    } else {
        std::cerr << "not ok " << test++
                  << " - Partio::cloneSchema() fixed attribute count is "
                  << numFixedAttributes << ", expected 3" << std::endl;
        exit(EXIT_FAILURE);
    }

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
        p->fixedAttributeInfo(i,attr);

        if (attr.name == expectedName[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() fixed attribute name for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() fixed attribute name is "
                      << attr.name << ", expected " << expectedName[i] << std::endl;
            exit(EXIT_FAILURE);
        }

        if (attr.type == expectedType[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() fixed attribute type for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() fixed attribute type for "
                      << attr.name << ", expected "
                      << expectedType[i] << ", got " << attr.type << std::endl;
        }

        if (attr.count == expectedCount[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() fixed attribute count for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() fixed attribute count for "
                      << attr.name << ", expected "
                      << expectedCount[i] << ", got " << attr.count << std::endl;
        }
    }

    p->release();
}


void testCloneParticleDataAttributes(Partio::ParticlesData* other, int& test)
{
    Partio::ParticlesData* p=Partio::cloneSchema(*other);

    int numAttributes = p->numAttributes();
    if (numAttributes == 3) {
        std::cout << "ok " << test++
                  << " - Partio::cloneSchema() numAttributes()" << std::endl;
    } else {
        std::cerr << "not ok " << test++
                  << " - Partio::cloneSchema() attribute count is "
                  << numAttributes << ", expected 3" << std::endl;
        exit(EXIT_FAILURE);
    }

    int numParticles = p->numParticles();
    if (numParticles == 0) {
        std::cout << "ok " << test++
                  << " - Partio::cloneSchema() contains no particles" << std::endl;
    } else {
        std::cerr << "not ok " << test++
                  << " - Partio::cloneSchema() contains particles, "
                  << "expected 0, got " << numParticles
                  << std::endl;
        exit(EXIT_FAILURE);
    }

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

    // Test attributes
    for (int i=0;i<numAttributes;++i) {
        Partio::ParticleAttribute attr;
        p->attributeInfo(i,attr);

        if (attr.name == expectedName[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() attribute name for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() attribute name is " << attr.name
                      << ", expected " << expectedName[i] << std::endl;
            exit(EXIT_FAILURE);
        }

        if (attr.type == expectedType[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() attribute type for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() attribute type for " << attr.name
                      << ", expected " << expectedType[i]
                      << ", got " << attr.type << std::endl;
        }

        if (attr.count == expectedCount[i]) {
            std::cout << "ok " << test++
                      << " - Partio::cloneSchema() attribute count for "
                      << attr.name << std::endl;
        } else {
            std::cerr << "not ok " << test++
                      << " - Partio::cloneSchema() attribute count for " << attr.name
                      << ", expected " << expectedCount[i]
                      << ", got " << attr.count << std::endl;
        }
    }

    p->release();
}


int main(int argc,char *argv[])
{
    int test = 1;

    Partio::ParticlesDataMutable* foo=makeData();
    {
        testCloneParticleFixedAttributes(foo, test);
        testCloneParticleDataAttributes(foo, test);
    }
    foo->release();

    return 0;
}
