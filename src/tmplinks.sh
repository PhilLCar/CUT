#! /usr/bin/env sh

# Temporary
CUT_HOME=/home/phil/Program/Utilities

cd $CUT_HOME
echo $(pwd)
exit

rm -rf .cut
mkdir .cut
mkdir .cut/inc
mkdir .cut/obj
mkdir .cut/lib
for f in $(ls); do
  cd $f
  
  for include in $(ls); do
    ln -s $CUT_HOME/$f/inc/$include $CUT_HOME/.cut/
  done
  ln -sf $CUT_HOME/.cut $CUT_HOME/$f/
  cd ../..
done