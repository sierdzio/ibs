#!/bin/bash

# Bail on errors.
# set -e

if [ "${1}" = "-h" ] || [ "${1}" = "--help" ]; then
  echo "Usage: run-compilation-tests.sh -q qt-directory -i ibs-exe-path "
  echo "[-j jobs] [-m qmake-path]"
  echo ""
  echo "Compiles all tests located in ibs/testData using -j jobs, -q Qt directory,"
  echo "-i ibs path and optional -m qmake path."
  echo ""
  echo "Where skin is optional. If specified, results will be limited to specified"
  echo "skin and global changes."
  exit
fi

JOBS="1"
QTDIR=""
IBSEXE=""
QMAKEEXE=""
LOG="$PWD/compilation-summary.log"
DETAILS="$PWD/compilation-details.log"

while getopts "j:q:i:m:" opt ;
do
  case $opt in
  j) JOBS=$OPTARG
    ;;
  q) QTDIR=$OPTARG
    ;;
  i) IBSEXE=$OPTARG
    ;;
  m) QMAKEEXE=$OPTARG
    ;;
  :)
    echo "Option -$OPTARG requires an argument."
    exit 1
    ;;
  esac
done

cleanUp() {
  rm -f .ibs.cache *.o moc_* .qmake* Makefile
}

# Clear log file
echo "" > $LOG
echo "" > $DETAILS

# Remove build dir
rm -rf build/

for dir in ../testData/* ; do
  SOURCE=../../../testData/$dir
  CURRENT=build/$dir
  # echo "Source: $SOURCE destination: $CURRENT"
  mkdir -p $CURRENT
  cd $CURRENT
  echo "Entered: $dir"
  # echo "Cleanup"
  # TODO: also remove moc def file
  cleanUp
  ts=$(date +%s%N)
  $IBSEXE -j $JOBS --qt-dir $QTDIR $SOURCE/main.cpp >> $DETAILS 2>&1
  EXIT_CODE=$?
  tt=$((($(date +%s%N) - $ts)/1000000))
  echo "IBS compiling:\t$dir/main.cpp\t$tt" | tee --append $LOG $DETAILS # >/dev/null

  if [ "$EXIT_CODE" != "0" ]; then
    echo "IBS failed with: $EXIT_CODE after $tt"
    exit $EXIT_CODE
  fi

  if [ -f "$QMAKEEXE" ]; then
    ts=$(date +%s%N)
    $QMAKEEXE $SOURCE/ && make -j $JOBS >> $DETAILS 2>&1
    EXIT_CODE=$?
    tt=$((($(date +%s%N) - $ts)/1000000))
    echo "QMAKE compiling:\t$dir\t$tt" | tee --append $LOG $DETAILS # >/dev/null

    if [ "$EXIT_CODE" != "0" ]; then
      echo "QMAKE failed with: $EXIT_CODE after $tt"
      exit $EXIT_CODE
    fi
  fi
  cleanUp
  echo "Finished: $dir"
  cd ../..
done

cat $LOG

# Remove build dir
rm -rf build/

echo "Done"
