#!/usr/bin/env python
"""
    hdk-config.py - a tool that runs hcustom and makes the information
    available in a variety of ways for alternative build systems like
    make, cmake and scons.

    author: Drew.Whitehouse@anu.edu.au, Feb 2009

    usage: It is assumed that you have the HDK setup properly, ie you
    can compile DSO's at the command line, like

        > hcustom SOP_Star.C

    At that point type "python hdk-config.py -h".

"""
import os
import sys
import time
import socket
import platform
import optparse
from subprocess import Popen,PIPE
from tempfile import NamedTemporaryFile,gettempdir
from optparse import OptionParser
import pprint
pp = pprint.PrettyPrinter(indent=4)

class HcustomParseError(Exception): pass

debug = False

class HcustomInfo:

    def __init__(self,debug=False,notag=False,standalone=False):

        self.debug = debug
        self.standalone = standalone
        self.notag = notag

        # these are filled in when we run hcustom 
        self.tmpfile_name = None

        self.compile = None  # the raw compile command
        self.link = None     # the raw link command

        if not self.run_hcustom():
            raise RuntimeError("Couldn't run hcustom, are you sure you have the HDK setup properly ?")

        # all the following attributes are filled in by parse(), but they are left in the platform specific form ie -Ifoo or /Ifoo 
        
        self._compiler = None # the compiler used
        self._optimisations = [] # eg -O2
        self._defines = []  # eg -Dfoo ...
        self._warnings = [] # eg -Wbar ...
        self._options = []   # eg -fbas ...
        self._include_dirs = []

        self._linker = None   # the linker used
        self._libraries = [] 
        self._library_dirs = []
        self._frameworks = []
        self._framework_dirs = []
        self._ldopts = []

        self._arch = None
        self._machine_options = []

        self._parse()


    # these are classes externally visible methods
    
    def cxx(self):
        return self._compiler

    def cxxflags(self):
        res = self._defines
        res += self._optimisations + self._warnings + self._options + self._machine_options
        res += self._include_dirs
        if self._arch:
            res.append('-arch %s' % self._arch)
        return " ".join(res)

    def linker(self):
        return self._linker

    def ldflags(self):
        res = self._ldopts + self._library_dirs + self._libraries + self._machine_options
        res += ['-framework %s' % f for f in self._frameworks]
        return " ".join(res)

    def include_dirs(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            return [i[2:] for i in self._include_dirs]
        else:
            raise RuntimeError("win32 include dirs not implemented yet")

    def library_dirs(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            return [i[2:] for i in self._library_dirs]
        else:
            raise RuntimeError("win32 library dirs not implemented yet")

    def libraries(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            return [i[2:] for i in self._libraries]
        else:
            raise RuntimeError("win32 libraries not implemented yet")

    def framework_dirs(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            return [i[2:] for i in self._framework_dirs]
        else:
            raise RuntimeError("win32 library dirs not implemented yet")

    def frameworks(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            return self._frameworks
        else:
            raise RuntimeError("win32 libraries not implemented yet")

    def definitions(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            res = []
            if self._arch:
                res.append("-arch %s" % self._arch)
            res.extend(self._machine_options)
            res.extend(self._defines)
            res.extend(self._warnings)
            res.extend(self._options)
            res.extend(self._optimisations)
            return res
        else:
            raise RuntimeError("win32 definitions not implemented yet")

    def _parse(self):

        self._compiler = self.compile[0]
        self._linker = self.link[0]

        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            self._parse_unix()
        elif platform.system() == 'NT':
            self.parse_windows()
        else:
            raise RuntimeError("Unknown system \"%s\"" % platform.system())


    def _parse_unix(self):

        if debug:
            print '\n\nDBG: >>>>>>>>> compile',self.compile

        self._sanitise_unix()
        
        # parse the compile command

        # keep track of the flags we've handled
        cflags_seen = set()

        compiler = self.compile[0]
        cflags_seen.add(compiler)

        try:
            # get rid of the "-c" directive
            index = self.compile.index('-c')
            cflags_seen.add('-c')

            # get rid of the target object file
            index = self.compile.index('-o')
            cflags_seen.update(self.compile[index:index+2])
        except ValueError:
            raise HcustomParseError("unexpected format in compile string")

        try:
            index = self.compile.index('-arch')
            cflags_seen.update(self.compile[index:index+2])
            arch = self.compile[index+1]
        except ValueError:
            arch = None

        inc = [i for i in self.compile if i.startswith('-I')]
        defs = [i for i in self.compile if i.startswith('-D')]
        warn = [i for i in self.compile if i.startswith('-W')]
        option = [i for i in self.compile if i.startswith('-f')]
        opt = [i for i in self.compile if i.startswith('-O') or i=='-g']
        mopts = [i for i in self.compile if i.startswith('-m')]

        if debug:
            print 'DBG: defs',defs
            print 'DBG: warn',warn
            print 'DBG: option',option
            print 'DBG: inc',inc
            print 'DBG: opt',opt

        # check we have handled all the arguments
        cflags_seen.update(defs + warn+ opt + option + opt + mopts + inc)
        for  c in self.compile:
            if not c in cflags_seen:
                print >> sys.stderr,'WARNING: unhandled compile option ->',c

        self._defines = defs
        self._warnings = warn
        self._options = option
        self._include_dirs = inc
        self._optimisations = opt
        self._arch = arch
        self._machine_options = mopts

        # parse the link command
        if debug:
            print '\n\nDBG: >>>>>>>>> link',self.link

        lflags_seen = set()
        try:
            linker  = self.link[0]
            lflags_seen.add(linker)

            # get rid of the link target 
            index = self.link.index('-o')
            lflags_seen.update(self.link[index:index+2])

        except ValueError:
            raise HcustomParseError("unexpected format in link string")

        # get rid of the link target
        try:
            if not self.standalone:
                index = self.link.index('-bundle')
                lflags_seen.update(self.link[index:index+2])
        except ValueError:
            pass
        
        # get rid of the architecture
        try:
            index = self.link.index('-arch')
            lflags_seen.update(self.link[index:index+2])
        except ValueError:
            pass

        ldirs = [i for i in self.link if i.startswith('-L')]
        ofiles = [i for i in self.link if i.endswith('.o')]
        libs = [i for i in self.link if i.startswith('-l')]
        lopts = [i for i in self.link if (i.startswith('-f') and not i.startswith('-framework')
                                          or i=='-shared' 
                                          or i.startswith('-Wl,')
                                          or i.startswith('-rpath'))]
        fdirs = [i for i in self.link if i.startswith('-F')]
        mopts = [i for i in self.compile if i.startswith('-m')]
        fwks = [j for i,j in zip(self.link,self.link[1:]) if i == '-framework']
        lflags_seen.add('-framework')

        if '-shared' in self.link:
            lflags_seen.add('-shared')

        if debug:
            print 'DBG: ofiles',ofiles
            print 'DBG: libs',libs
            print 'DBG: ldirs',ldirs
            print 'DBG: lopts',lopts
            print 'DBG: framework_dirs',fdirs
            print 'DBG: frameworks',fwks
            print 'DBG: machine options',mopts

        # check we have handled all the arguments
        lflags_seen.update(ldirs+libs+lopts+ofiles+fwks+fdirs+mopts)
        for  l in self.link:
            if not l in lflags_seen:
                print >> sys.stderr,'WARNING: unhandled link option ->',l

        self._libraries = libs
        self._library_dirs = ldirs
        self._frameworks = fwks
        self._framework_dirs = fdirs
        self._ldopts = lopts

    def _sanitise_unix(self):
        """
        Go through the flags in self.compile and self.link and
        regularise things to make them easier to parse later.
        """

        # compile flags
        res = []
        lst = list(self.compile)
        while lst:
            head = lst.pop(0)
            if head == '-I':
                res.append('-I%s' % lst.pop(0))
            elif 'UT_DSO_TAGINFO' in head:
                if not self.notag:
                    res.append(head.replace('\'','').replace('"','\\"'))
            else:
                res.append(head)
        self.compile = res
                
        # link flags
        res = []
        lst = list(self.link)
        while lst:
            head = lst.pop(0)
            if head == '-L':
                res.append('-L%s' % lst.pop(0))
            else:
                res.append(head)
        self.link = res

        
    def run_hcustom(self):

        # write a temporary file to tempdir and run hcustom on it
        tmpdir = os.getcwd()
        dir = gettempdir()

        try:
            os.chdir(dir)
            tmpfile = NamedTemporaryFile(suffix='.C')
            tmpfile_src = os.path.basename(tmpfile.name)
            tmpfile.write("void foo() {}\n")
            tmpfile.flush()
            
            cmd = ["hcustom","-e","-i",dir]
            if self.debug:
                cmd.append("-g")
            if self.standalone:
                cmd.append("-s")
            cmd.append(tmpfile_src)

            try:
                output = Popen(cmd, stdout=PIPE).communicate()[0]
            except OSError:
                return False
            del tmpfile

        finally:
            os.chdir(tmpdir)

        if debug:
            print 'DBG: *****************\n%s\nDBG: *****************' % output
            
        lines = output.split('\n')

        # remove non compile and link lines
        tmp = []
        for l in lines:
            if 'Install directory' in l:
                continue
            elif "Linking with" in l:
                continue
            elif "Making" in l:
                continue
            elif l=="":
                continue
            else:
                tmp.append(l)
        lines = tmp    

        # we should have two lines left - the first a compile, the other a link
        if self.debug:
            print '\n\n*******************************************************'
            i = 0
            for l in lines:
                print i,'>>>>>> ',l
                i += 1
            print '\n*******************************************************'

        if self.standalone and platform.system() == 'Darwin' and len(lines) == 3:
            del lines[1]
            print 'WARNING: standalone is not supported for OSX at the moment'

        assert len(lines) == 2
        self.compile = lines[0].split()
        self.link = lines[1].split()

        # remove the source file from the compile string
        self.compile.remove(tmpfile_src)

        return True

def hih():
    """
    Returns the users houdini config directory, eg $HOME/houdini10.0
    on linux. For some reason the Mac is treated differently.
    """
    if platform.system() == 'Linux' or platform.system=='NT':
        return os.getenv("HIH")
    elif platform.system() == "Darwin":
        return os.path.join(os.getenv("HOME"),"Library","Preferences","houdini",
                            os.getenv("HOUDINI_MAJOR_RELEASE")+"."+os.getenv("HOUDINI_MINOR_RELEASE"))
    else:
        raise RuntimeError("unknown platform")
    

def write_cmakefile(file):
    """
    Writes a cmake FindXXX style module for the HDK.
    """

    header = \
"""
#
#    Locate the HDK environment
#
# This module defines 
# HDK_FOUND,
# HDK_CXX_COMPILER
# HDK_INCLUDE_DIRS,
# HDK_DEFINITIONS,
# HDK_LIBRARY_DIRS,
# HDK_LIBRARIES,
# HDK_DSO_INSTALL_DIR
#
# For OSX, we have the following as well ...
#
# HDK_FRAMEWORK_DIRS,
# HDK_FRAMEWORKS,
#

SET(HDK_FOUND 1)
"""

    # get the hcustom info, no (debug, notag, no standalone)
    hcinfo = HcustomInfo(False,True,False)
    
    file.write(header)
    file.write("\n")

    file.write("set(HDK_CXX_COMPILER %s)\n" % hcinfo.cxx())

    if platform.system() == 'Darwin':
        file.write("set(CMAKE_OSX_ARCHITECTURES x86_64)\n")

    file.write("set(HDK_INCLUDE_DIRS %s)\n" % " ".join(hcinfo.include_dirs()))

    # note how add the definition to block the accidental inclusion of <UT/UT_DSOVersion.h>
    file.write("\n")
    file.write("# the following are for compiling dso's\n")
    file.write("set(HDK_DEFINITIONS %s)\n" % " ".join(hcinfo.definitions()+['-D__UT_DSOVersion__']))
    file.write("set(HDK_LIBRARY_DIRS %s)\n" % " ".join(hcinfo.library_dirs()))

    # deal with frameworks for osx
    file.write("set(HDK_FRAMEWORK_DIRS %s)\n" % " ".join(hcinfo.framework_dirs()))
    file.write("set(HDK_FRAMEWORKS %s)\n" % " ".join(hcinfo.frameworks()))

    libtmp = []
    for f in hcinfo.frameworks():
        file.write('FIND_LIBRARY(%s_LIBRARY %s)\n' % (f.upper(),f))
        libtmp.append('${%s_LIBRARY}' % f.upper())

    libtmp.extend(hcinfo.libraries())

    # TBD: deal with RPATH ???

    # need to add the frameworks here
    file.write("set(HDK_LIBRARIES %s)\n" % " ".join(libtmp))

    # where to install dso's ?
    file.write("set(HDK_HIH_DIR %s)\n" % hih())
    file.write("\n\n")

    # now for about a standalone version ?
    
    hcinfo = HcustomInfo(False,True,True)
    file.write("# the following are for compiling dso's\n")
    file.write("set(HDK_STANDALONE_DEFINITIONS %s)\n" % " ".join(hcinfo.definitions()+['-D__UT_DSOVersion__']))
    file.write("set(HDK_STANDALONE_LIBRARY_DIRS %s)\n" % " ".join(hcinfo.library_dirs()))

    # deal with frameworks for osx
    file.write("set(HDK_STANDALONE_FRAMEWORK_DIRS %s)\n" % " ".join(hcinfo.framework_dirs()))
    file.write("set(HDK_STANDALONE_FRAMEWORKS %s)\n" % " ".join(hcinfo.frameworks()))

    libtmp = []
    for f in hcinfo.frameworks():
        file.write('FIND_LIBRARY(%s_LIBRARY %s)\n' % (f.upper(),f))
        libtmp.append('${%s_LIBRARY}' % f.upper())

    libtmp.extend(hcinfo.libraries())

    # TBD: deal with RPATH ???

    # need to add the frameworks here
    file.write("set(HDK_STANDALONE_LIBRARIES %s)\n" % " ".join(libtmp))
    


def make_sesitag(file):
    """
    Generates a small C++ source file that has an up to date
    UT_DSO_TAGINFO defined and includes UT/UT_DSOVersion.h.
    """
    taginfo = "compiled on: %s\n         by: %s@%s" % (time.asctime(),os.getenv("LOGNAME"),socket.gethostname())
    p = Popen(["sesitag","-m"],stdin=PIPE,stdout=PIPE)
    output = p.communicate(taginfo)[0]

    # pull out the tag
    tagstr = output[len('''"-DUT_DSO_TAGINFO=\'"''')-1:-2]

    # now write the file
    file.write("//\n")
    file.write("// Warning: do not edit, this file was written automatically by hdk-config.py.\n")
    file.write("//\n")
    file.write("#ifdef __UT_DSOVersion__\n")
    file.write("#undef  __UT_DSOVersion__\n")
    file.write("#endif\n")
    file.write("#define UT_DSO_TAGINFO \"%s\"\n" % tagstr)
    file.write("#include <UT/UT_DSOVersion.h>\n")

def main():

    global debug
    
    if not os.getenv("HFS"):
        print "error: check that houdini is installed properly, currently $HFS isn't defined."
        sys.exit(1)
        
    usage = "Usage: %prog [options]"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("-C","--cmake",action="store",type="string",dest="cmakefile",
                      help="write out the compile defs to a cmake package module (file \"-\" means write to stdout)")

    parser.add_option("-T","--tagfile",action="store",type="string",dest="tagfile",
                      help="generate a C++ file with a sesitag defined, ie no need to compile -DUT_DSO_VERSION  (file \"-\" means write to stdout)")

    parser.add_option("--cxx",action="store_true",dest="cxx",
                      help="return C++ compiler used to compile HDK")

    parser.add_option("--cxxflags",action="store_true",dest="cxxflags",
                      help="return flags to compile C++ for HDK projects")

    parser.add_option("--linker",action="store_true",dest="linker",
                      help="return C++ linker used to link HDK projects")

    parser.add_option("--ldflags",action="store_true",dest="ldflags",
                      help="return flags to link C++ using the HDK")

    parser.add_option("-g","--debug",action="store_true",dest="debug",
                      help="compile the program with debug information")

    parser.add_option("-s","--standalone",action="store_true",dest="standalone",
                      help="compile the program to be standalone")

    parser.add_option("--notag",action="store_true",dest="notag",
                      help="do not include the UT_DSO_TAGINFO in flags")

    parser.add_option("--hih",action="store_true",dest="hih",
                      help="returns $HIH or equivalent (hcustom install home?)")

    parser.add_option("-D",action="store_true",dest="noisy",
                      help="verbose output for debugging this tool")

    options,args = parser.parse_args()

    if options.noisy:
        debug = True

    # write out a tagfile ?
    if options.tagfile:
        if options.tagfile == "-":
            file = sys.stdout
        else:
            file = open(options.tagfile,"w")
        make_sesitag(file)

    # write cmake defs ?
    if options.cmakefile:
        if options.cmakefile == "-":
            file = sys.stdout
        else:
            file = open(options.cmakefile,"w")
        write_cmakefile(file)

    else:

        # get the hcustom info
        hcinfo = HcustomInfo(options.debug,options.notag,options.standalone)

        if options.cxx:
            print hcinfo.cxx(),
        if options.cxxflags:
            print hcinfo.cxxflags(),
        if options.linker:
            print hcinfo.linker(),
        if options.ldflags:
            print hcinfo.ldflags(),
        if options.hih:
            print hih(),

if __name__ == '__main__':

    main()



