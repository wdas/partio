#!/usr/bin/env python
# -*- coding:utf-8 -*-

from setuptools import setup
from cmake_build_extension import BuildExtension, CMakeExtension
import shutil
import subprocess
from pathlib import Path
import os


# create the output dirs
os.makedirs("build_partio/src/py", exist_ok=True)

PARTIO_EXT =  CMakeExtension(
    name="partio",
    install_prefix="partio",
    source_dir=".",
    cmake_configure_options=[
    ],
)

class BuildPartioExtension(BuildExtension):
    def __init__(self, *args, **kwars) -> None:
        super().__init__(*args, **kwars)

    def run(self) -> None:
        super().run()
        if shutil.which("patchelf") is None:
            raise RuntimeError("Required command 'patchelf' not found")

    def build_extension(self, ext: CMakeExtension) -> None:
        self.build_temp = "build"
        super().build_extension(ext)
        # Get the absolute path to the build folder
        build_folder = str(Path(".").absolute() / f"{self.build_temp}_{ext.name}")
        shared_object_path = os.path.join(build_folder, "src", "py", "_partio.so")
        # directly set the path of the shared object into the shared object created from swig
        patch_command = [
            "patchelf", 
            "--replace-needed", 
            "libpartio.so.1", 
            f"{build_folder}/src/lib/libpartio.so.1", 
            shared_object_path
        ]
        print("Patching shared obbject to directly include the libpartio file")
        print(f" => {patch_command}")
        subprocess.check_call(patch_command)

setup(
    name='partio',
    description='A Python wrapper for partio',
    packages=[],
    package_dir={'': 'build_partio/src/py'},
    ext_modules=[PARTIO_EXT],
    cmdclass={
        'build_ext': BuildPartioExtension
    },
    install_requires=["cmake-build-extension"],
    python_requires='>=3.4',

)