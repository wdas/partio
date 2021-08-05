#!/usr/bin/env python
# -*- coding:utf-8 -*-

from setuptools import setup, Extension
from setuptools.command.install_lib import install_lib
from setuptools.command.build_ext import build_ext

# from cmake_build_extension import BuildExtension, CMakeExtension
import shutil
import platform

from pathlib import Path
import os
import subprocess
import shutil
import sys
from distutils.sysconfig import get_python_inc, get_config_var


class CMakeExtension(Extension):
    def __init__(
        self, name, source_dir=str(Path(".").absolute()), cmake_configure_options=[],
    ):
        super().__init__(name=name, sources=[])
        self.cmake_configure_options = cmake_configure_options
        self.cmake_build_type = "RelWithDebInfo"
        self.source_dir = source_dir


class BuildCMakeExtension(build_ext):
    def run(self):
        cmake_extensions = [e for e in self.extensions if isinstance(e, CMakeExtension)]
        if len(cmake_extensions) == 0:
            raise ValueError("No CMakeExtension objects found")

        # Check that CMake is installed
        if shutil.which("cmake") is None:
            raise RuntimeError("Required command 'cmake' not found")

        for ext in cmake_extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        # CMake configure arguments
        configure_args = [
            "-DCMAKE_BUILD_TYPE={}".format(ext.cmake_build_type),
            "-DPARTIO_ORIGIN_RPATH=ON",
            "-DCMAKE_INSTALL_PREFIX:PATH=build/install",
        ]
        build_args = ["--config", ext.cmake_build_type]
        # Extend the configure arguments with those passed from the extension
        configure_args += ext.cmake_configure_options

        if platform.system() == "Windows":
            configure_args += []
        else:
            configure_args += []

        # Get the absolute path to the build folder
        build_folder = str(
            Path(".").absolute()
            / "{build_temp}_{name}".format(build_temp=self.build_temp, name=ext.name)
        )

        # Make sure that the build folder exists
        Path(build_folder).mkdir(exist_ok=True, parents=True)

        # 1. Compose CMake configure command
        configure_command = [
            "cmake",
            "-S",
            ext.source_dir,
            "-B",
            build_folder,
        ] + configure_args

        # 2. Compose CMake build command
        build_command = ["cmake", "--build", build_folder] + build_args

        # 3. Compose CMake install command
        install_command = ["cmake", "--install", build_folder]

        print("")
        print("==> Configuring:")
        print(" ".join(configure_command))
        print("")
        print("==> Building:")
        print(" ".join(build_command))
        print("")
        print("==> Installing:")
        print(" ".join(install_command))
        print("")

        # Call CMake
        subprocess.check_call(configure_command)
        subprocess.check_call(build_command)
        subprocess.check_call(install_command)


PARTIO_EXT = CMakeExtension(
    name="partio",
    cmake_configure_options=[
        "-DPYTHON_INCLUDE_DIR={} ".format(get_python_inc()),
        "-DPYTHON_LIBRARY={} ".format(get_config_var("LIBDIR")),
        "-DPYTHON_EXECUTABLE={} ".format(sys.executable),
    ],
)


class InstallLibs(install_lib):
    def run(self):
        self.announce("+++ InstallLibs", level=3)
        self.skip_build = True

        # Everything under self.build_dir will get moved into site-packages
        # so move all things we want installed there
        lib_dir = self.build_dir
        write_dir = Path(lib_dir) / "partio"

        os.makedirs(write_dir, exist_ok=True)

        # copy equired files
        python_version = "python{major}.{minor}".format(
            major=sys.version_info[0], minor=sys.version_info[1]
        )
        base_path = Path.cwd() / "build" / "install" / "lib64"
        to_install = [
            base_path / "libpartio.so.1",
            base_path / python_version / "site-packages" / "partio.py",
            base_path / python_version / "site-packages" / "_partio.so",
        ]
        for lib in to_install:
            filename = Path(lib).name
            target = str(write_dir / filename)
            if os.path.isfile(target):
                os.remove(target)
            shutil.move(str(lib), str(write_dir))

        # create init file
        with open(str(write_dir / "__init__.py"), "w") as f:
            f.write("from .partio import *\n")
        self.announce("+++ custom done", level=3)
        super().run()


setup(
    name="partio",
    description="A Python wrapper for partio",
    version="1.14.0",
    packages=[],
    ext_modules=[PARTIO_EXT],
    cmdclass={"build_ext": BuildCMakeExtension, "install_lib": InstallLibs,},
    python_requires=">=3.6",
)
