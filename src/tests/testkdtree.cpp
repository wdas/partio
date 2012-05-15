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

#include <Partio.h>
#include <iostream>
#include <cmath>
#include <stdexcept>

#include "partiotesting.h"
#include "testkdtree.h"
#include "core/KdTree.h"

// using namespace Partio;
namespace PartioTests {

KdTreeTestData makeKDTreeData()
{
    // kd tree with 4 data points located on the diagonal of a cube
    float ptData[] = { 1.0f, 1.0f, 1.0f,
                       0.5f, 0.5f, 0.5f,
                    -0.5f, -0.5f, -0.5f,
                    -1.0f, -1.0f, -1.0f };
    KdTreeTestData testData;
    testData.m_fourPtTree = new Partio::KdTree<3>();
    testData.m_fourPtTree->setPoints(ptData, 4);
    return testData;

}

namespace Internal {
    // just instantiate empty tree and check that size is 0
    void test_KdTreeSize(const KdTreeTestData& dat)
    {
        Partio::KdTree<2> emptyTree;
        TESTEXPECT(emptyTree.size() == 0);
        TESTEXPECT(dat.m_fourPtTree->size() == 4);

    }

    void freeKDTreeData( KdTreeTestData& data )
    {
        delete data.m_fourPtTree;
    }

}

void test_KDTree()
{
	std::cout << "------- Executing test_KDTree() -------" << std::endl;
    KdTreeTestData testData = makeKDTreeData();
    try
    {
        Internal::test_KdTreeSize(testData);
    }
    catch(std::exception& excep)
    {
        std::cerr << "Encountered exception " << excep.what() << " during testing" << std::endl;
    }
    Internal::freeKDTreeData(testData);

}

}
