#!/bin/bash
# This script is in use by the Linux and OSX buildslaves to upload the package to openphdguiding.org

set -e
set -x

LATEST=`ls -1tr *.deb *.zip|tail -1`

if [ -z "$LATEST" ]; then
  echo "No package found with ls -1t *.deb *.zip"
  exit 1
fi

(echo "
cd upload
put $LATEST
quit
") | sftp phd2buildbot@openphdguiding.org

rm $LATEST

