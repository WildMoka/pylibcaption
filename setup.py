import multiprocessing
import os
import subprocess

import setuptools
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
from distutils.command import build_ext

libcaption_extension = Extension(
    name="pylibcaption",
    sources=["pylibcaption.pyx", "eia608_encoder.c", "better_eia608_encoder.c"],
    extra_objects=["./libcaption/libcaption.a"],
    libraries=["caption"],
    library_dirs=["libcaption"],
    include_dirs=["libcaption/caption"]
)


class LibcaptionBuildExt(build_ext.build_ext):
    def build_libcaption_static_lib(self):
        # Run configure/make
        abs_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'libcaption')

        def call(cmd):
            subprocess.check_call(cmd.split(' '), cwd=abs_path)

        # comment logging on stderr
        for file in ["cea708.c", "eia608.c", "mpeg.c"]:
            call('sed -i /^\s*fprintf(stderr.*;$/s|^|//|;/^\s*fprintf(stderr/,/);$/s|^|//| %s' % os.path.join(abs_path,
                                                                                                              'src/', file))
        # Run the autotools/make build to generate a python extension module
        call('cmake -DENABLE_RE2C=OFF -DCMAKE_C_FLAGS=-fPIC .')
        call('make -j%s' % (multiprocessing.cpu_count()))

    def run(self):
        self.build_libcaption_static_lib()

        # Import numpy here, only when headers are needed
        import numpy
        # Add numpy headers to include_dirs
        self.include_dirs.append(numpy.get_include())

        # Call super
        build_ext.build_ext.run(self)


with open("README.md", "r") as fh:
    long_description = fh.read()

setup(name="pylibcaption",
      description="Wrapper module for libcaption using numpy arrays interface",
      author="r3gis3r",
      author_email="regis@wildmoka.com",
      version="0.0.7",
      long_description=long_description,
      long_description_content_type="text/markdown",
      url="https://github.com/wildmoka/pylibcaption",
      packages=setuptools.find_packages(),
      ext_modules=cythonize([libcaption_extension]),
      py_modules=["pylibcaption"],
      cmdclass={
          "build_ext": LibcaptionBuildExt,
      },
      install_requires=['numpy'],
      classifiers=[
          "Development Status :: 3 - Alpha",
          "Intended Audience :: Developers",
          "License :: OSI Approved :: BSD License",
          "Programming Language :: Python :: 2",
          "Programming Language :: Python :: Implementation :: CPython",
          "Topic :: Software Development :: Libraries",
          "Topic :: Scientific/Engineering :: Image Recognition"
      ]
      )
