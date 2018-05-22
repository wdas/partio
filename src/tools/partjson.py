#!/usr/bin/env python

"""
Converts partio files to and from json.

Usage:  partjson [FLAGS] <foo.(json|bgeo)> <bar.(bgeo|json)>

Supported FLAGS:
    -c/--compress: When converting to partio, compress the output
    -v/--verbose : Turn on verbosity for partio
    -h/--help    : Print this help message
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

import os, sys, json
import partio

def toJson(particleSet):
    """ Converts a particle set to json """

    data = {}

    # Put types in json just for readability
    data['__types__'] = { 0 : 'NONE',
                          1 : 'VECTOR',
                          2 : 'FLOAT',
                          3 : 'INT',
                          4 : 'INDEXED_STR',
                        }

    # Convert fixed attributes
    fixedAttributes = {}
    fixedIndexedStrings = {}
    for i in range(particleSet.numFixedAttributes()):
        attr = particleSet.fixedAttributeInfo(i)
        fixedAttributes[attr.name] = {'type': attr.type,
                                      'count': attr.count,
                                      'value': particleSet.getFixed(attr),
                                     }

        # Convert indexed string attributse
        if attr.type == 4:
            fixedIndexedStrings[attr.name] = particleSet.fixedIndexedStrs(attr)

    if fixedAttributes:
        data['fixedAttributes'] = fixedAttributes
    if fixedIndexedStrings:
        data['fixedIndexedStrings'] = fixedIndexedStrings

    # Convert particle attributes
    attributes = {}
    attrs = []
    indexedStrings = {}
    for i in range(particleSet.numAttributes()):
        attr = particleSet.attributeInfo(i)
        attrs.append(attr)
        attributes[attr.name] = {'type': attr.type, 'count': attr.count }

        # Convert indexed string attributse
        if attr.type == 4:
            indexedStrings[attr.name] = particleSet.indexedStrs(attr)

    if attributes:
        data['attributes'] = attributes
    if indexedStrings:
        data['indexedStrings'] = indexedStrings

    # Convert particles to an indexed dictionary
    particles = {}
    for i in range(particleSet.numParticles()):
        particle = {}
        for attr in attrs:
            particle[attr.name] = particleSet.get(attr, i)
        # Add an index purely for readability & debugging (not consumed converting back)
        particles[i] = particle

    if particles:
        data['particles'] = particles

    return data


def fromJson(data):
    """ Converts a json dictionary to a particle set """

    particleSet = partio.create()

    # Convert fixed attributes
    fixedAttributes = {}
    if 'fixedAttributes' in data:
        for attrName, attrInfo in data['fixedAttributes'].iteritems():
            attrName = str(attrName)
            attr = particleSet.addFixedAttribute(attrName, attrInfo['type'], attrInfo['count'])
            fixedAttributes[attrName] = attr
            if len(attrInfo['value']) == attrInfo['count']:
                particleSet.setFixed(attr, attrInfo['value'])
            else:
                sys.stderr.write('Mismatched count for fixed attribute {}. Skipping.\n'.format(attrName))

    # Convert attributes
    attributes = {}
    if 'attributes' in data:
        for attrName, attrInfo in data['attributes'].iteritems():
            attrName = str(attrName)
            attr = particleSet.addAttribute(attrName, attrInfo['type'], attrInfo['count'])
            attributes[attrName] = attr

    # Convert fixed indexed strings
    if 'fixedIndexedStrings' in data:
        for attrName, strings in data['fixedIndexedStrings'].iteritems():
            if attrName not in fixedAttributes:
                sys.stderr.write('Could not match fixed indexed string {} with any defined fixed attribute. Skipping.\n'.format(attrName))
                continue
            for string in strings:
                particleSet.registerFixedIndexedStr(fixedAttributes[attrName], string)

    # Convert indexed strings
    if 'indexedStrings' in data:
        for attrName, strings in data['indexedStrings'].iteritems():
            if attrName not in attributes:
                sys.stderr.write('Could not match indexed string {} with any defined attribute. Skipping.\n'.format(attrName))
                continue
            for string in strings:
                particleSet.registerIndexedStr(attributes[attrName], str(string))

    # Convert particles
    if 'particles' in data:
        particleSet.addParticles(len(data['particles']))
        for pnum, particle in data['particles'].iteritems():
            pnum = int(pnum)
            for attrName, value in particle.iteritems():
                try:
                    attr = attributes[attrName]
                except IndexError:
                    sys.stderr.write('Could not match attribute "{}" for particle {} with any defined attributes. Skipping.\n'.format(attrName, pnum))
                    continue
                if len(value) != attr.count:
                    sys.stderr.write('Mismatched count for attribute "{}" ({}) and particle {} ({}). Skipping.\n'.format(attrName, attr.count, pnum, len(value)))
                    continue

                particleSet.set(attr, pnum, value)

    return particleSet

def main():
    """ Main """

    # Process command-line arguments
    filenames = []
    verbose = False
    compress = False
    for arg in sys.argv[1:]:
        if arg in ('-h', '--help'):
            print __doc__
            return

        if arg in ('-v', '--verbose'):
            verbose = True
            continue

        if arg in ('-c', '--compress'):
            compress = True
            continue

        filenames.append(arg)

    if len(filenames) != 2:
        print __doc__
        sys.stderr.write('Incorrect number of arguments.\n')
        sys.exit(1)

    file1, file2 = filenames[0:2]
    ext1 = os.path.splitext(file1)[1]
    ext2 = os.path.splitext(file2)[1]

    partio_extensions = ('.bgeo', '.geo', '.bhclassic', '.ptc', '.pdb')

    # Validate files
    if not os.path.exists(file1):
        sys.stderr.write('Invalid input file: {}\n'.format(file1))
        sys.exit(1)

    # Convert from json to partio
    if ext1 == '.json':
        if ext2 not in partio_extensions:
            sys.stderr.write('Unknown partio extension for: {}\n'.format(file2))
            sys.exit(1)

        with open(file1, 'r') as fp:
            data = json.load(fp)
        particleSet = fromJson(data)
        partio.write(file2, particleSet, compress)
        sys.exit(0)

    if ext1 not in partio_extensions:
        sys.stderr.write('Unknown partio extension for: {}\n'.format(file1))
        sys.exit(1)

    # Convert from partio to json
    if ext1 in partio_extensions:
        particleSet = partio.read(file1, verbose)
        data = toJson(particleSet)
        with open(file2, 'w') as fp:
            json.dump(data, fp, indent=2, sort_keys=True)
        sys.exit(0)

    print __doc__
    sys.stderr.write('Unknown file extension(s)')
    sys.exit(1)


if __name__ == '__main__':
    main()
