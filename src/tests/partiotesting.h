#ifndef PARTIOTESTING_H
#define PARTIOTESTING_H

#include <iostream>

#define TESTASSERT(x)\
 if(!(x)) throw std::runtime_error(__FILE__ ": Test failed on " #x );

#define TESTEXPECT(x)\
 if(!(x)) std::cerr << __FILE__ << ": Test failed in " << #x << " on line " << __LINE__ << std::endl;

namespace Partio {
    const size_t GRIDN = 9;
}

#endif
