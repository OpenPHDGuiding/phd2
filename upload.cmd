@echo off

rem This script is in use by the Windows buildslaves to upload the package to openphdguiding.org

for /F "usebackq" %%f in (`dir phd2-*-win32.exe /O-D /B` ) do (
  if EXIST %%f (
   (
   echo cd upload
   echo put %%f
   echo quit
   ) | psftp phd2buildbot@openphdguiding.org
  ) 
  goto :end
)

:end

