#!/usr/bin/env python
"""Partio Python API tests"""
from __future__ import absolute_import, division, print_function, unicode_literals
import os
import sys

import pytest

import partio


num_particles = 3
num_attributes = 3


@pytest.fixture(scope='module')
def particle_file():
    testdir = os.path.dirname(os.path.abspath(__file__))
    srcdir = os.path.dirname(testdir)
    filename = os.path.join(srcdir, 'data', 'test.bgeo')

    result = partio.read(filename)
    _check_particles(result, num_attributes)
    _check_attribute(result, 'position', num_particles)
    return result


def test_clone_without_data(particle_file):
    """Clone particles without data"""
    clone = partio.clone(particle_file, False)
    _check_particles(clone, 0)
    assert clone.attributeInfo('position') is None


def test_clone_with_data(particle_file):
    """Clone with data and no attrNameMap specified"""
    clone = partio.clone(particle_file, True)
    _check_particles(clone, num_attributes)
    _check_attribute(clone, 'position', 3)


def test_clone_with_data_and_map_is_none(particle_file):
    """Clone with data and attrNameMap specified as None"""
    clone = partio.clone(particle_file, True, None)
    _check_particles(clone, num_attributes)
    _check_attribute(clone, 'position', 3)


def test_clone_with_data_and_attribute_map(particle_file):
    """Clone with data and a remapped position -> pos attribute"""
    attr_name_map = {'position': 'pos'}
    clone = partio.clone(particle_file, True, attr_name_map)
    _check_particles(clone, num_attributes)
    _check_attribute(clone, 'pos', 3)  # New attribute should be present
    assert clone.attributeInfo('position') is None  # Old attribute is gone.


def _check_particles(partfile, count):
    """Ensure that the particles file contains attributes"""
    assert partfile is not None
    assert partfile.numAttributes() == count


def _check_attribute(partfile, name, count):
    """Ensure that the specified attribute is valid"""
    info = partfile.attributeInfo(name)
    assert info is not None
    assert info.count == count


if __name__ == '__main__':
    sys.exit(pytest.main([__file__]))
