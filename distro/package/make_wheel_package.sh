#!/bin/bash
set -ex

scriptDir=$(cd $(dirname $0) && pwd)

install_patchelf()
{
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
patchelfExe=$scriptDir/patchelf-install/bin/patchelf

if [ ! -f "$patchelfExe" ]; then
  echo "Installing patchelf"
  install_patchelf
fi

if [ ! -d "$packageDir" ]; then
  echo "Package dir not found: $packageDir"
  exit 1
fi

if [ ! -f "$patchelfExe" ]; then
  echo "patchelf not found: $patchelfExe"
  echo 1
fi

cd $packageDir


# remove unused files
echo "Removing unused files"
rm -rf bin include plugins share lib/cmake lib/pkgconfig lib/*.a lib/python3*/site-packages/urdf_parser_py lib/vtk lib/vtk-9.2

# remove pycache and symlinks
find . -name __pycache__ | xargs rm -r
#find . -type l | xargs rm


cd lib
mv libvtkDRCFilters* libddApp* libctkPythonConsole* libPythonQt* libQtPropertyBrowser* python3*/site-packages/director/
mv libvtk* python3*/site-packages/vtkmodules/

cd python3*/site-packages

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
