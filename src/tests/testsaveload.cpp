/*
PARTIO SOFTWARE
Copyright 2013 Disney Enterprises, Inc. All rights reserved

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

#include "testsaveload.h"

// using namespace Partio;
namespace PartioTests {

Partio::ParticlesDataMutable* makeSaveLoadTestData()
{
    Partio::ParticlesDataMutable& foo=*Partio::create();
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

void testEmptySaveLoadFile(const char* filename)
{
    Partio::ParticlesDataMutable* p=Partio::create();
    p->addAttribute("position",Partio::VECTOR,3);
    std::cerr<<"Testing empty save with file '"<<filename<<"'"<<std::endl;
    Partio::write(filename,*p);
    p->release();
    Partio::ParticlesDataMutable* pread=Partio::read(filename);
    pread->release();
}

void testSaveLoadFile(Partio::ParticlesData* p,const char* filename)
{
    std::cerr<<"Testing with file '"<<filename<<"'"<<std::endl;
    Partio::write(filename,*p);
    Partio::ParticlesData* pnew=Partio::read(filename);
    pnew->release();
}

void test_SaveLoad()
{
 {
        Partio::ParticlesDataMutable* foo=makeSaveLoadTestData();
        testSaveLoadFile(foo,"test.bgeo");
        testSaveLoadFile(foo,"test.bgeo.gz");
        testSaveLoadFile(foo,"test.geo");
        testSaveLoadFile(foo,"test.geo.gz");
        testSaveLoadFile(foo,"test.ptc");
        testSaveLoadFile(foo,"test.ptc.gz");
        testSaveLoadFile(foo,"test.pdb");
        testSaveLoadFile(foo,"test.pdb.gz");
        testEmptySaveLoadFile("testEmpty.geo");
        testEmptySaveLoadFile("testEmpty.bgeo");
        testEmptySaveLoadFile("testEmpty.pdb");
        testEmptySaveLoadFile("testEmpty.ptc");
        foo->release();
    }

}
}
