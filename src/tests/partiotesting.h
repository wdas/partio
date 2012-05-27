#ifndef PARTIOTESTING_H
#define PARTIOTESTING_H

#include <iostream>
#include <cmath>

#define TESTASSERT(x)\
 if(!(x)) throw std::runtime_error(__FILE__ ": Test failed on " #x );

#define TESTEXPECT(x)\
 if(!(x)) std::cerr << __FILE__ << ": Test failed in " << #x << " on line " << __LINE__ << std::endl;

namespace PartioTests {
    const size_t GRIDN = 9;

    template <size_t k>
    bool floatArraysEq(const float *array1,
                       const float *array2, float tol = 0.0001)
    {
        assert(tol >= 0.0f);
        size_t i = 0;
        bool tolNotExceeded = true;
        while(i < k && tolNotExceeded)
        {
            tolNotExceeded = std::abs(array1[i] - array2[i]) < tol;
            ++i;
        }
        return tolNotExceeded;
    }
}

#endif
