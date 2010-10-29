import os
import platform


default_cxx="g++"

options=Options("SConstruct.options")

options.AddOptions(
    ('CXX','C++ compiler',default_cxx),
    ('mac','Is a mac',False),
    EnumOption("TYPE",
               "Type of build (e.g. optimize,debug)",
               "optimize",
               allowed_values=("profile","optimize","debug")),
    )

uos = platform.system()
ver = os.popen("fa.arch -r").read().strip()
arch = platform.machine()

variant_basename = '%s-%s-%s' % (uos, ver, arch)

AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          default=variant_basename,
          metavar='DIR',
          help='installation prefix')

env=Environment(options=options,
                prefix=GetOption('prefix'))

env.Append(CPPDEFINES=["PARTIO_USE_ZLIB"])

# --prefix is typically an absolute path, but we
# should also allow relative paths for convenience.

# prefix is the installation prefix, e.g. /usr
# variant_install_abs is the path to the install root.

prefix = GetOption('prefix')
if os.path.isabs(prefix):
    prefix_base = os.path.basename(prefix)
    variant_install_abs = prefix
else:
    prefix_base = prefix
    variant_install_abs = Dir(".").abspath + "/" + prefix

type=env["TYPE"]
prefix_base+="-%s"%type
variant_install_abs+="-%s"%type

# variant_build_abs is the path to the temporary build directory.
variant_build = os.path.join('build', prefix_base)
variant_build_abs = Dir(".").abspath+"/"+variant_build

if env["mac"]==True:
    env.Append(CPPDEFINES=["DARWIN"])
    #env.Append(LINKFLAGS=["-m32"])

if env["TYPE"]=="optimize":
    env.Append(CXXFLAGS="-fPIC -DNDEBUG -O3 -fno-strict-aliasing -Wall -Werror -Wstrict-aliasing=0  -mfpmath=sse -msse3".split())
if env["TYPE"]=="profile":
    env.Append(CXXFLAGS="-fPIC -fno-strict-aliasing -DNDEBUG -g -fno-omit-frame-pointer -O3 -Wall -Werror -Wstrict-aliasing=0  -mfpmath=sse -msse3".split())
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
