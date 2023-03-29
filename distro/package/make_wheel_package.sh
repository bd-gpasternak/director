#!/bin/bash
set -ex #Exit if even if one command fails

scriptDir=$(cd $(dirname $0) && pwd)
echo "*** Running script from directory: ${scriptDir} ***"


install_patchelf()
{
  echo "***Installing patchelf in ${scriptDir} ***"
  cd $scriptDir
  wget http://nixos.org/releases/patchelf/patchelf-0.8/patchelf-0.8.tar.gz
  tar -zxf patchelf-0.8.tar.gz
  pushd  patchelf-0.8
  ./configure --prefix=$scriptDir/patchelf-install
  make install
  popd
  rm -rf patchelf-0.8 patchelf-0.8.tar.gz
}

packageDir=$scriptDir/$1
echo "*** Package directory: ${packageDir} ***"
patchelfExe=$scriptDir/patchelf-install/bin/patchelf

if [ ! -f "$patchelfExe" ]; then
  install_patchelf
fi

if [ ! -d "$packageDir" ]; then
  echo "*** Package directory not found: ${packageDir} ***"
  exit 1
fi

if [ ! -f "$patchelfExe" ]; then
  echo "*** Patchelf not found: $patchelfExe ***"
  echo 1
fi

cd $packageDir
# remove unused files
echo "*** Removing unused files ***"
rm -rf bin include plugins share lib/cmake lib/pkgconfig lib/*.a lib/python3*/site-packages/urdf_parser_py lib/vtk lib/vtk-9.2

# remove pycache and symlinks
echo "*** Removing pycache and symlinks ***"
find . -name __pycache__ -print0 | xargs -0 rm -rf
# Disabling removal of symlinks since this leaves lib*.so.9.2.2 in vtkmodules which does not work in rt
# find . -type l -print0 | xargs -0 rm -f


cd lib
libDir=$(pwd)
echo "*** Moving files in ${libDir} to python3*/site-packages/ ***"
mv libvtkDRCFilters* libddApp* libctkPythonConsole* libPythonQt* libQtPropertyBrowser* python3*/site-packages/director/
mv libvtk* python3*/site-packages/vtkmodules/

cd python3*/site-packages
sitePkgDir=$(pwd)
echo "*** Creating vtk dir in ${sitePkgDir} and copying vtk.py as __init__.py ***"
mkdir vtk
mv vtk.py vtk/__init__.py

cd director
for f in $(ls lib* *.so | sort -u);
do
  if [[ ! -L $f ]]; then
    $patchelfExe --set-rpath '$ORIGIN:$ORIGIN/../vtkmodules' --force-rpath $f
  fi
done


cd ../vtkmodules
for f in $(ls lib* *.so | sort -u);
do
  if [[ ! -L $f ]]; then
    $patchelfExe --set-rpath '$ORIGIN' --force-rpath $f
  fi
done
