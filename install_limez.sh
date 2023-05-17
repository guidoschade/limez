#!/bin/sh

xcp()
{
  # parameter check
  if [ ! $# -eq 2 ]; then
    echo "usage: cp src-file dst-file/dir"
    exit 1
  fi 
 
  # check if file exists
  if [ ! -f $1 ]; then
    echo "error: file/dir $1 does not exist"
    exit 1
  fi

  cp $1 $2 || { echo "error: cp failed"; exit 1 ; }
}

# parameter check
if [ ! $# -eq 3 ]; then
  echo "usage: $0 <location> <user> <group>"
  echo "example: $0 /usr/local/limez limez limez"
  echo "         (the user and group has to exist)"
  exit 1
fi 

echo "creating directory and copying files ..."

strip src/limez
mkdir $1 || { echo "error: failed to create directory $1"; exit 1 ; }
chmod 775 $1
chmod g+s $1
xcp src/limez $1/
chmod 755 $1/limez

xcp INSTALL $1/
xcp README $1/
xcp COPYING $1/
xcp AUTHORS $1/
xcp CHANGELOG $1/
xcp TODO $1/

echo "..."

mkdir $1/spool
mkdir $1/done
mkdir $1/lists
mkdir $1/scripts
chmod 775 $1/spool $1/done $1/lists $1/scripts

touch $1/limez.config
touch $1/cache.bin
touch $1/limez.log
chmod 664 $1/limez.log $1/limez.config $1/cache.bin

xcp scripts/limez.server $1/scripts/
chown -R $2:$3 $1

echo "Installation succeeded."
echo ""
echo "Now you have to 'cd' to '$1'"
echo "and issue the command: ./limez -v"
echo "or ./limez -h to check configuration."
echo ""
echo "please follow the hints in the INSTALL file"
echo ""
