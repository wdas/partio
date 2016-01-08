import os
import platform

def kernel_version():
    return os.popen("uname -r").read().strip().split('-')[0]

default_cxx="g++"
default_cxxflags=""

options=Variables("SConstruct.options")

uos = platform.system()
ver = kernel_version()
arch = platform.machine()

options.AddVariables(
    ('CXX','C++ compiler',default_cxx),
    ('CXXFLAGS','C++ compiler flags',default_cxxflags),
    ('mac','Is a mac', uos == 'Darwin'),
    EnumVariable("TYPE",
               "Type of build (e.g. optimize,debug)",
               "optimize",
               allowed_values=("profile","optimize","debug")),
    )

variant_basename = '%s-%s-%s' % (uos, ver, arch)

AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          default='',
          metavar='DIR',
          help='installation prefix')

env=Environment(options=options,
                prefix=GetOption('prefix'))

type=env["TYPE"]
variant_basename += "-%s"%type
env.Append(CPPDEFINES=["PARTIO_USE_ZLIB"])

# --prefix is typically an absolute path, but we
# should also allow relative paths for convenience.

# prefix is the installation prefix, e.g. /usr
# variant_install_abs is the path to the install root.

prefix = GetOption('prefix') or os.path.join('dist', variant_basename)
if os.path.isabs(prefix):
    variant_install_abs = prefix
else:
    variant_install_abs = os.path.join(Dir(".").abspath, prefix)

# variant_build_abs is the path to the temporary build directory.
variant_build = os.path.join('build', variant_basename)
variant_build_abs = os.path.join(Dir(".").abspath, variant_build)

if env["mac"]==True:
    env.Append(CPPDEFINES=["__DARWIN__"])
    #env.Append(LINKFLAGS=["-m32"])

if env["TYPE"]=="optimize":
    env.Append(CXXFLAGS=" -fPIC -DNDEBUG -O3 -fno-strict-aliasing -Wall -Wstrict-aliasing=0 -mfpmath=sse -msse3".split())
if env["TYPE"]=="profile":
    env.Append(CXXFLAGS=" -fPIC -fno-strict-aliasing -DNDEBUG -g -fno-omit-frame-pointer -O3 -Wall -Werror -Wstrict-aliasing=0  -mfpmath=sse -msse3".split())
elif env["TYPE"]=="debug":
    env.Append(CXXFLAGS=" -fPIC -g -Wall -Werror -Wstrict-aliasing=0")


VariantDir(variant_build, '.', duplicate=0)


def GetInstallPath():
    return variant_install_abs

Export("env variant_build variant_build_abs variant_install_abs GetInstallPath")

env.SConscript(variant_build+"/src/lib/SConscript")
env.SConscript(variant_build+"/src/tools/SConscript")
env.SConscript(variant_build+"/src/tests/SConscript")
env.SConscript(variant_build+"/src/doc/SConscript")
env.SConscript(variant_build+"/src/py/SConscript")

