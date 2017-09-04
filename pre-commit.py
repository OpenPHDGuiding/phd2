#! /usr/bin/env python
from __future__ import print_function, with_statement

import os, sys
import subprocess
import errno
import io, codecs

class Filedata:

    def __init__(self, f):
        self.dosEndings = 0
        self.unixEndings = 0
        self.macEndings = 0
        self.tabs = 0
        self.trailingSpaces = 0

        self.process(f)

    def process(self, f):
        for line in f:
            #print ("dos={0} mixed={1} ".format(dosEnding, mixedEnding), end="")
            #if len(line) > 1:
            #    print("{0} {1} {2} {3}".format(len(line), ord(line[-2]), ord(line[-1]), line.strip()))
            #else:
            #    print("{0} {1}".format(len(line), line))
            if len(line) > 0:
                if ord(line[-1]) == 10:
                    if len(line) > 1 and ord(line[-2]) == 13 and ord(line[-1]) == 10:
                        self.dosEndings += 1
                        line = line[:-1]
                    else:
                        self.unixEndings += 1
                elif ord(line[-1]) == 13:
                    self.macEndings += 1
                line = line[:-1]
                if line.find("\t") >= 0:
                    self.tabs += 1
                if line.endswith(" "):
                    self.trailingSpaces += 1
        return

def printList(label, nameList):
    if len(nameList) > 0:
        first = True
        print(label, end="")
        for fileName in nameList:
            if not first:
                print(",", end="")
            print("{0}".format(fileName), end="")
            first = False
    print("\n")

def interestingExtension(fileName):
    lowerName = fileName.lower()
    return lowerName.endswith(".cs") or lowerName.endswith(".c") or lowerName.endswith(".cpp") or lowerName.endswith(".h")


def processFile(f):
    return Filedata(f)

def processNamedFile(fileName):
    fileData = None

    try:
        with io.open(fileName, "rb") as f:
            fileData = processFile(f)
    except IOError as ex:
        if (ex.errno != errno.ENOENT):
            print("open failed with errno:" + str(ex.errno))

    return fileData

def enforcePolicies(fileData, fileName):
    error = False

    if fileData:
        if fileData.dosEndings > 0 and fileData.unixEndings > 0:
            print("Rejected - mixed newlines: {0}".format(fileName))
            error = True
        elif fileData.dosEndings > 0:
            pass
        elif fileData.unixEndings > 0:
            print("Rejected - unix newlines: {0}".format(fileName))
            error = True
        elif fileData.macEndings > 0:
            print("Rejected - old mac newlines: {0}".format(fileName))
            error = True
        if fileData.tabs > 0:
            print("Rejected - contains tabs: {0}".format(fileName))
            error = True
        if fileData.trailingSpaces > 0:
            print("Rejected - trailing spaces: {0}".format(fileName))
            error = True
    return error

def processDirTree(startDir):
    error = False
    for root, dirs, files in os.walk(startDir):
        for fileName in files:
            if interestingExtension(fileName):
                fullPath = os.path.join(root, fileName)
                fileData = processNamedFile(fullPath)
                error = enforcePolicies(fileData, fullPath) or error
    return error


def usage():
    print("usage: {0}: [options] <diretory> [<directory>...]".format(sys.argv[0]), file=sys.stderr);

def runCmd(cmd):
    try:
        #print("running {0}".format(cmd))
        cmdProcess = subprocess.Popen(cmd,shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (stdoutData, stderrData) = cmdProcess.communicate(None)
        return (cmdProcess.returncode, stdoutData, stderrData)
    except:
        raise

def runCmdOK(cmd):
    (returnCode, stdoutData, stderrData) = runCmd(cmd)
    if returnCode != 0:
        print("{0} returned non-zero status.".format(cmd))
        if len(stderrData):
            print("stderr output was " + stderrData)
        sys.exit(returnCode)
    return stdoutData

def runCmdBool(cmd):
    (returnCode, stdoutData, stderrData) = runCmd(cmd)
    return returnCode == 0

def checkFiles(fileNames):
    invalidEnding = False
    for fileName in fileNames:
        try:
            if interestingExtension(fileName):
                #print("checking file {0}".format(fileName))
                (returnCode, stdoutData, stderrData) = runCmd("git show HEAD:{0}".format(fileName))
                if returnCode == 0:
                    if stdoutData.startswith(codecs.BOM_UTF16_LE) or stdoutData.startswith(codecs.BOM_UTF16_BE):
                        encoding = "utf-16"
                    elif stdoutData.startswith(codecs.BOM_UTF8):
                        encoding = "utf-8-sig"
                    else:
                        encoding = "Latin-1"
                    #print("checking file {0} bytes={1} encoding={2}".format(fileName, stdoutData[:4], encoding))
                    oldEnding = processFile(io.StringIO(unicode(stdoutData, encoding)))
                    newEnding = processNamedFile(fileName)
                    #print(fileName, oldEnding, newEnding)

                    invalidEnding = enforcePolicies(newEnding, fileName) or invalidEnding
        except Exception as ex:
            print("caught exception {0} processing file {1}".format(ex, fileName), file=sys.stderr)
    return invalidEnding

def checkCommit():
    if runCmdBool("git rev-parse --quiet --verify HEAD"):
        against = "HEAD"
    else:
        against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"

    stdoutData = runCmdOK("git diff --cached --name-only {0}".format(against))
    files = stdoutData.split()
    reject = checkFiles(files)

    if len(files) == 0:
        print("No cached git files found -- did you forget a directory name?")

#    if not reject:
#        reject = not runCmdBool("git diff-index --check --cached {0} --".format(against))
#        if reject:
#            print("[Rejected by Policy - whitespace issue]: found by git diff-index --check --cached {0} --".format(against))

    return reject

def main():
    argc = len(sys.argv)
    if argc == 1:
        sys.exit(checkCommit())
    elif argc == 2:
        dirName = sys.argv[1]
        print("processing directory " + dirName)
        processDirTree(dirName)
    else:
        print("too many arguments", file=sys.stderr)

if __name__ == "__main__":
    main()
