'''
Copyright (c) 2019, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
Written by Harsh Bhatia (hbhatia@llnl.gov) and Peer-Timo Bremer (bremer5@llnl.gov)
LLNL-CODE-763493. All rights reserved.

This file is part of MemSurfer, Version 1.0.
Released under GNU General Public License 3.0.
For details, see https://github.com/LLNL/MemSurfer.
'''

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

import os, sys
import glob
import numpy
from setuptools import setup, find_packages, Extension

# ------------------------------------------------------------------------------
# Read the correct paths for the depenencies from the environment
# ------------------------------------------------------------------------------
def read_paths(default):

    try:
        VTK_ROOT = os.path.expandvars(os.environ['VTK_ROOT'])
    except KeyError:
        VTK_ROOT = default

    try:
        CGAL_ROOT = os.path.expandvars(os.environ['CGAL_ROOT'])
    except KeyError:
        CGAL_ROOT = default

    try:
        BOOST_ROOT = os.path.expandvars(os.environ['BOOST_ROOT'])
    except KeyError:
        BOOST_ROOT = default

    return VTK_ROOT, CGAL_ROOT, BOOST_ROOT

def choose_path(root, libname):

    options = ['lib', 'lib64']
    extns = ['so', 'dylib']

    for o in options:
      for e in extns:
        if os.path.isfile(os.path.join(root, o, '{}.{}'.format(libname, e))):
          return os.path.join(root, o)

    raise Exception('choose_path({},{}) failed!'.format(root, libname))

# ------------------------------------------------------------------------------
# Read the correct paths for the code, including the dependencies
# ------------------------------------------------------------------------------
SRC_PATH = os.path.split(os.path.abspath(sys.argv[0]))[0]
VTK_ROOT, CGAL_ROOT, BOOST_ROOT = read_paths(os.path.join(SRC_PATH, 'external'))

# cgal gets installed in lib or lib64
CGAL_lpath = choose_path(CGAL_ROOT, 'libCGAL')
VTK_lpath = os.path.join(VTK_ROOT, 'lib')
CGAL_ipath = os.path.join(CGAL_ROOT, 'include')
VTK_ipath = os.path.join(VTK_ROOT, 'include', 'vtk-8.1')
EIG_ipath = os.path.join(CGAL_ROOT, 'include', 'eigen3')
#BOOST_ipath = os.path.join(BOOST_ROOT, 'include')
BOOST_ipath = '/opt/local/include'

'''
print 'VTK_lpath', VTK_lpath
print 'VTK_ipath', VTK_ipath
print 'CGAL_lpath', CGAL_lpath
print 'CGAL_ipath', CGAL_ipath
print 'EIG_ipath', EIG_ipath
print 'BOOST_ipath', BOOST_ipath
'''
# ------------------------------------------------------------------------------
# Collect the names of libraries to link to and code to compile
# ------------------------------------------------------------------------------

# cpp code
headers = os.path.join(SRC_PATH, 'libsurfer', 'include')
sources = glob.glob(os.path.join(SRC_PATH, 'libsurfer', 'src', '*.cpp'))
sources.append(os.path.join(SRC_PATH, 'memsurfer', 'pysurfer.i'))

# external libs
libs = glob.glob(os.path.join(VTK_lpath, 'libvtk*'))
libs = [l for l in libs if os.path.islink(l)]
libs = [os.path.basename(l) for l in libs]
libs = [l[3:l.rfind('.')] for l in libs]
libs.append('CGAL')

# define the extension module
ext_mod =  Extension('_pysurfer',
                     sources = sources,
                     include_dirs=[headers, numpy.get_include(),
                                   BOOST_ipath, EIG_ipath, CGAL_ipath, VTK_ipath],
                     libraries=libs,
                     library_dirs=[VTK_lpath, CGAL_lpath],
                     language = 'c++',
                     swig_opts=['-c++', '-I'+headers],
                     define_macros=[('VTK_AVAILABLE',1), ('CGAL_AVAILABLE',1)],
                     extra_compile_args=['-std=c++11',
                                         '-Wno-inconsistent-missing-override',
                                         '-Wno-deprecated-declarations',
                                         '-Wno-unknown-pragmas'],
                     extra_link_args=['-std=c++11']
                    )

# set up!
setup(name='memsurfer',
      version='0.1.0',
      description='Python tool to compute bilayer membranes.',
      author='Harsh Bhatia',
      author_email='hbhatia@llnl.gov',
      packages=find_packages(),
      package_data={ 'memsurfer': ['_pysurfer.so'] },
      ext_modules=[ext_mod]
)
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
