#!/usr/bin/python

import os

ndk_dir      = '/home/data/android/android-ndk-r10e'
platform     = 'android-17'
gcc_ver      = '4.9'
gcc_inc      = ndk_dir + '/sources/cxx-stl/gnu-libstdc++/' + gcc_ver + '/include'
libs_dir     = ndk_dir + '/sources/cxx-stl/gnu-libstdc++/' + gcc_ver + '/libs'
platform_inc = ndk_dir + '/platforms/' + platform + '/arch-arm/usr/include/'


for fn in os.listdir(libs_dir):
    libs_inc = libs_dir + '/' + fn + '/include/'
    cmd = './b2 --reconfigure include=' + gcc_inc \
          + ' include=' + libs_inc \
          + ' include=' + platform_inc \
          + ' install --libdir=stage/lib/' + fn
    if not os.path.isfile(fn):
        print (cmd)


