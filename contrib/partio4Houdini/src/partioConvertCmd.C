/* partio4Houdini  5/01/2013, Miles Green

 PARTIO Import/Export
 Copyright 2013 (c)  All rights reserved

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.

 Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
 IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */


// Notes: This code produces the command line program "partioConverCmd" that will be used by houdini's GEOio protocol so that the regualr file + geometry ROPS's can read and write bgeo formats


#include <CMD/CMD_Args.h>
#include <GU/GU_Detail.h>

#include "partio_houdini.h"

static void usage(const char *program)
{
    cerr << "ERROR program arguments called incorrectly\n";
    cerr << "Usage: " << program << " sourcefile dstfile\n";
    cerr << "The fil eextension of the source/dest will be used to determine how the conversion is procesed." << endl;
    cerr << "Supported extensions are .pdb and .bin .pdc .pda .ptc .rib .mc" << endl;
    cerr << "and .bgeo" << endl;
}


int main(int argc, char *argv[])
{
    //cerr << "running partioConvert" << endl;

    GU_Detail gdp;

    CMD_Args args;
    args.initialize(argc, argv);
    if (args.argc() != 3)
    {
        usage(argv[0]);
        return 1;
    }

    // Check if we are converting from a partio format.  If the source extension
    // is .pdb etc, we are converting from.  Otherwise we convert to.
    // By being liberal with our accepted extensions we will support
    // a lot more than just .bgeo since the built in gpdb .load() and .save()
    // will handle the issues for us.
    UT_String inputname, outputname;
    inputname.harden(argv[1]);
    outputname.harden(argv[2]);

    if (!strcmp(inputname.fileExtension(), ".bgeo")||!strcmp(inputname.fileExtension(), "bgeo.gz"))
    {
        gdp.load((const char *) inputname, 0);
        partioSave(outputname, &gdp,4);
    }
    else
    {
        partioLoad(inputname, &gdp,4);
        gdp.save((const char *) outputname, 0, 0);
    }

    return 0;
}

