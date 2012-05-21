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

#include <iostream>
#include <cmath>
#include <stdexcept>


#include <Partio.h>
#include "partiotesting.h"
#include "testkdtree.h"
#include "core/KdTree.h"

namespace PartioTests {

namespace Internal {

    void setUp4ptData(Partio::KdTree<3>& dat)
    {
        float ptData[] = { 1.0f, 1.0f, 1.0f,
                       0.5f, 0.5f, 0.5f,
                    -0.5f, -0.5f, -0.5f,
                    -1.0f, -1.0f, -1.0f };
        dat.setPoints(ptData, 4);
    }

    // just instantiate empty tree and check that size is 0

    void test_KdTree_size()
    {
        Partio::KdTree<2> emptyTree;
        TESTEXPECT(emptyTree.size() == 0);
        Partio::KdTree<3> fourPtTree;
        setUp4ptData(fourPtTree);
        TESTEXPECT(fourPtTree.size() == 4);
        std::cout << "[Finished test_KdTree_size]" << std::endl;

    }

    void test_KdTree_point()
    {
        Partio::KdTree<3> fourPtTree;
        setUp4ptData(fourPtTree);

        const float* pt0 = fourPtTree.point(0);
        const float* pt3 = fourPtTree.point(3);
        for(size_t i = 0; i < 3; i++)
        {
            TESTEXPECT(pt0[i] == 1.0f);
            TESTEXPECT(pt3[i] == -1.0f);
        }
        std::cout << "[Finished test_KdTree_point]" << std::endl;

    }

    void test_KdTree_bbox()
    {
        Partio::KdTree<3> fourPtTree;
        setUp4ptData(fourPtTree);

        const Partio::BBox<3>& fourPtBox = fourPtTree.bbox();
        for(size_t i = 0; i < 3; i++)
        {
            TESTEXPECT(fourPtBox.max[i] == 1.0f);
            TESTEXPECT(fourPtBox.min[i] == -1.0f);
        }
        std::cout << "[Finished test_KdTree_bbox]" << std::endl;
    }

    // currently not working as expected, need to understanad findPoints better
    void test_KdTree_findPoints()
    {
        Partio::KdTree<3> fourPtTree;
        setUp4ptData(fourPtTree);
        const float origin[3] = {0.0f, 0.0f, 0.0f};
        Partio::BBox<3> testBox(origin);
        testBox.grow(0.75);

        std::vector<uint64_t> result;
        result.resize(2);

        fourPtTree.findPoints(result, testBox);
        TESTASSERT(result.size() == 2);
        TESTEXPECT(result[0] == 2);
        TESTEXPECT(result[1] == 1);

        std::cout << "[Finished test_KdTree_findPoints]" << std::endl;

    }

}

void test_KDTree()
{
	std::cout << "------- Executing test_KDTree() -------" << std::endl;
    try
    {
        Internal::test_KdTree_size();
        Internal::test_KdTree_point();
        Internal::test_KdTree_bbox();
      //  Internal::test_KdTree_findPoints();
    }
    catch(std::exception& excep)
    {
        std::cerr << "Encountered exception " << excep.what() << " during testing" << std::endl;
    }
    std::cout << "------- finished -------" << std::endl;

}

}
