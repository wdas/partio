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


#include <gtest/gtest.h>
#include <Partio.h>
#include <iostream>

using namespace Partio;

// TODD - Create test base class to read in base and delta files
// and validate them, as well as release() on test completion.
// Add test to merge two particle sets without an identifier
// (should be fully additive)

class PartioTest : public ::testing::Test {
public:
    PartioTest() {}
    virtual ~PartioTest() {}

    void SetUp() {
        // Read in base and delta files
        std::string datadir(PARTIO_DATA_DIR "/src/data/");
        std::string base_geo = datadir + "base.bgeo";
        std::string delta_geo = datadir + "delta.bgeo";
        base  = Partio::read(base_geo.c_str());
        delta = Partio::read(delta_geo.c_str());

        // Validate each before merging (don't allow test data to change)
        ASSERT_EQ(5, base->numParticles());
        ASSERT_EQ(3, delta->numParticles());
        ASSERT_TRUE(base->attributeInfo("life", base_life_attr));
        ASSERT_TRUE(delta->attributeInfo("life", delta_life_attr));
        ASSERT_TRUE(base->attributeInfo("position", base_pos_attr));
        ASSERT_TRUE(delta->attributeInfo("position", delta_pos_attr));
        for (int i=0; i<5; i++) {
            ASSERT_EQ(base_values_life[i], base->data<float>(base_life_attr, i)[0]);
            ASSERT_EQ(base_values_posx[i], base->data<float>(base_pos_attr, i)[0]);
        }
        for (int i=0; i<3; i++) {
            ASSERT_EQ(delta_values_life[i], delta->data<float>(delta_life_attr, i)[0]);
            ASSERT_EQ(delta_values_posx[i], delta->data<float>(delta_pos_attr, i)[0]);
        }
    }

    void TearDown() {
        base->release();
        delta->release();
    }

    Partio::ParticlesDataMutable* base;
    Partio::ParticlesDataMutable* delta;
    ParticleAttribute base_life_attr, base_pos_attr;
    ParticleAttribute delta_life_attr, delta_pos_attr;
    std::vector<float> base_values_life{ -1.2, -0.2, 0.8, 1.8, 2.8 };
    std::vector<float> base_values_posx{ 0.0, 0.1, 0.2, 0.3, 0.4 };
    std::vector<float> delta_values_life{ 1.0, 3.0, 5.0 };
    std::vector<float> delta_values_posx{ 0.1, 0.3, 0.5 };
};

TEST_F(PartioTest, merge)
{
    std::cout << "\nBase particle set:\n";
    Partio::print(base);
    std::cout << "\nDelta particle set:\n";
    Partio::print(delta);

    // Do the merge
    Partio::merge(*base, *delta, "id");
    std::cout << "\nMerged particle set:\n";
    Partio::print(base);
    ASSERT_EQ(6, base->numParticles());
    std::vector<float> expected_life({-1.2, 1.0, 0.8, 3.0, 2.8, 5.0});
    std::vector<float> expected_posx({0.0, 0.1, 0.2, 0.3, 0.4, 0.5});
    for (int i=0; i<6; i++) {
        ASSERT_EQ(expected_life[i], base->data<float>(base_life_attr, i)[0]);
        ASSERT_EQ(expected_posx[i], base->data<float>(base_pos_attr, i)[0]);
    }

    int numFixed = base->numFixedAttributes();
    ASSERT_EQ(3, numFixed);
}

TEST_F(PartioTest, mergenoid)
{
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
