#!/usr/bin/env python

"""
Test for partjson - back and forth
"""

__copyright__ = """
CONFIDENTIAL INFORMATION: This software is the confidential and
proprietary information of Walt Disney Animation Studios ("WDAS").
This software may not be used, disclosed, reproduced or distributed
for any purpose without prior written authorization and license
from WDAS.  Reproduction of any section of this software must
include this legend and all copyright notices.
Copyright Disney Enterprises, Inc.  All rights reserved.
"""

import os, unittest
import partjson, partio


class test(unittest.TestCase):
    """ Test json conversions """

    def testPartJson(self):
        """ Test round-tripping """

        filename = os.path.join(os.getenv('PARTIO'), 'src/data/json.bgeo')
        particleSet = partio.read(filename)
        json1 = partjson.toJson(particleSet)
        particleSet2 = partjson.fromJson(json1)
        json2 = partjson.toJson(particleSet2)
        self.assertEquals(json1, json2)

if __name__ == '__main__':
    unittest.main()
