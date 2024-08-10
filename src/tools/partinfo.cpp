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
#include <iomanip>
#include <stdlib.h>
#include <set>

void help()
{
    std::cerr << "Usage: partinfo <filename> [FLAGS] { particle indices to print full info }\n";
    std::cerr << "Supported FLAGS:\n";
    std::cerr << "  -a/--all    : Print all particles\n";
    std::cerr << "  -s/--strings: Print all indexed string values (default=5)\n";
    std::cerr << "  -h/--help   : Print this help message\n";
}

void printParticle(int particleIndex, Partio::ParticlesDataMutable* p, size_t widest)
{
    std::cout << "---------------------------" << std::endl;
    std::cout << "Particle " << particleIndex << ":" << std::endl;
    if (particleIndex >= p->numParticles() || particleIndex < 0) {
        std::cout << "OUT OF RANGE" << std::endl;
    } else {
        for (int i = 0; i < p->numAttributes(); i++) {
            Partio::ParticleAttribute attr;
            p->attributeInfo(i, attr);
            std::cout << std::setw(10) << Partio::TypeName(attr.type) << " " << std::setw(widest) << attr.name << " ";
            if (attr.count > 1)
                std::cout << "[ ";
            for (int ii = 0; ii < attr.count; ii++) {
                if (ii)
                    std::cout << ", ";
                if (attr.type == Partio::INDEXEDSTR) {
                    int val = p->data<int>(attr, particleIndex)[ii];
                    std::cout << val << "=";
                    if (val < p->indexedStrs(attr).size()) {
                        std::cout << "'" << p->indexedStrs(attr)[val] << "'";
                    } else {
                        std::cout << "<out-of-bounds>";
                    }
                } else if (attr.type == Partio::INT)
                    std::cout << p->data<int>(attr, particleIndex)[ii];
                else
                    std::cout << p->data<float>(attr, particleIndex)[ii];
            }
            if (attr.count > 1)
                std::cout << " ]";
            std::cout << std::endl;
        }
    }
}

#define FLAG(shortFlag, longFlag) strcmp(argv[i], shortFlag) == 0 || strcmp(argv[i], longFlag) == 0

int main(int argc, char* argv[])
{
    const char* filename(nullptr);
    std::set<int> particleIndices;
    bool printAllStrings(false);
    bool printAllParticles(false);
    if (argc < 2) {
        help();
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        if (FLAG("-h", "--help")) {
            help();
            return 0;
        } else if (FLAG("-a", "--all")) {
            printAllParticles = true;
        } else if (FLAG("-s", "--strings")) {
            printAllStrings = true;
        } else if (argv[i][0] == '-') {
            std::cerr << "Unknown flag: " << argv[i] << std::endl;
        } else if (!filename) {
            filename = argv[i];
        } else {
            particleIndices.insert(atoi(argv[i]));
        }
    }
    Partio::ParticlesDataMutable* p = Partio::read(filename);
    if (!p) {
        std::cerr << "Unable to read \"" << filename << "\"\n";
        return 1;
    }

    std::cout << std::setiosflags(std::ios::left) << "Number of particles:  " << p->numParticles() << std::endl;
    int numAttr = p->numAttributes();
    std::cout << std::setw(12) << "Type" << std::setw(10) << "Count" << std::setw(30) << "Name" << std::endl;
    std::cout << std::setw(12) << "----" << std::setw(10) << "-----" << std::setw(30) << "----" << std::endl;
    for (int i = 0; i < numAttr; i++) {
        Partio::ParticleAttribute attr;
        p->attributeInfo(i, attr);
        std::cout << std::setw(12) << Partio::TypeName(attr.type) << std::setw(10) << attr.count << std::setw(30)
                  << attr.name << std::endl;
        ;
    }

    // Get widest attribute name for better formatting
    size_t widest(0);
    for (int i = 0; i < p->numAttributes(); ++i) {
        Partio::ParticleAttribute attr;
        p->attributeInfo(i, attr);
        widest = std::max(widest, attr.name.size());
    }
    widest++;

    if (printAllParticles) {
        for (int i = 0; i < p->numParticles(); ++i)
            printParticle(i, p, widest);
    } else if (!particleIndices.empty()) {
        for (int i : particleIndices)
            printParticle(i, p, widest);
    } else {
        int numFixedAttr = p->numFixedAttributes();
        if (numFixedAttr) {
            std::cout << "---------------------------" << std::endl;
            std::cout << "Fixed Attributes" << std::endl;
            std::cout << std::setw(12) << "Type" << std::setw(10) << "Count" << std::setw(widest) << "Name"
                      << std::endl;
            std::cout << std::setw(12) << "----" << std::setw(10) << "-----" << std::setw(widest) << "----"
                      << std::endl;
            for (int i = 0; i < numFixedAttr; i++) {
                Partio::FixedAttribute attr;
                p->fixedAttributeInfo(i, attr);
                std::cout << std::setw(12) << Partio::TypeName(attr.type) << std::setw(10) << attr.count
                          << std::setw(widest) << attr.name << std::endl;
                ;
            }

            for (int i = 0; i < numFixedAttr; i++) {
                Partio::FixedAttribute attr;
                p->fixedAttributeInfo(i, attr);
                std::cout << std::setw(10) << Partio::TypeName(attr.type) << " " << std::setw(widest) << attr.name;
                for (int ii = 0; ii < attr.count; ii++) {
                    if (attr.type == Partio::INDEXEDSTR) {
                        int val = p->fixedData<int>(attr)[ii];
                        std::cout << " " << val;
                        for (size_t index = 0; index < p->fixedIndexedStrs(attr).size(); index++) {
                            std::cout << std::endl << " " << index << " '" << p->fixedIndexedStrs(attr)[index] << "'";
                        }
                    } else if (attr.type == Partio::INT)
                        std::cout << " " << p->fixedData<int>(attr)[ii];
                    else
                        std::cout << " " << p->fixedData<float>(attr)[ii];
                }
                std::cout << std::endl;
            }
        }

        // Display indexed strings
        bool printedTitle(false);
        for (int i = 0; i < p->numAttributes(); ++i) {
            Partio::ParticleAttribute attr;
            p->attributeInfo(i, attr);
            if (attr.type == Partio::INDEXEDSTR) {
                if (!printedTitle) {
                    std::cout << "\nINDEXED STRINGS:\n";
                    printedTitle = true;
                }
                const std::vector<std::string> indexedStrs = p->indexedStrs(attr);
                std::cout << attr.name << " (" << indexedStrs.size() << ") = [";
                if (!indexedStrs.empty())
                    std::cout << std::endl;
                size_t numStrings = printAllStrings ? indexedStrs.size() : std::min((size_t)5, indexedStrs.size());
                for (size_t i = 0; i < numStrings; ++i) {
                    std::cout << "  " << indexedStrs[i] << std::endl;
                }
                if (indexedStrs.size() > 5)
                    std::cout << "  ...\n";
                std::cout << "]\n";
            }
        }
    }

    p->release();
    return 0;
}
