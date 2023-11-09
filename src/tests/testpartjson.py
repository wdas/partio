#!/usr/bin/env python
"""Test for partjson - back and forth"""

import os, unittest
import partjson, partio


class test(unittest.TestCase):
    """ Test json conversions """

    def testPartJson(self):
        """ Test round-tripping """

        testdir = os.path.dirname(os.path.abspath(__file__))
        srcdir = os.path.dirname(testdir)
        filename = os.path.join(srcdir, 'data', 'json.bgeo')
        particleSet = partio.read(filename)
        json1 = partjson.toJson(particleSet)
        particleSet2 = partjson.fromJson(json1)
        json2 = partjson.toJson(particleSet2)
        self.assertEquals(json1, json2)

if __name__ == '__main__':
    unittest.main()
