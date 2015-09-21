@echo off

rem This script is in use by the Windows buildslaves to upload the package to openphdguiding.org
rem 
rem Before to run this script you need to install putty and add it to PATH.
rem As we cannot use pageant from the buildbot service we need to create 
rem a saved session named phd2buildbot in putty using the same user as the service.
rem Set the hostname, auto-login user name and private key in the session. 

set exitcode=1

for /F "usebackq" %%f in (`dir phd2-*-win32.exe /O-D /B` ) do (
  if EXIST %%f (
   (
   echo cd upload
   echo put %%f
   echo quit
   ) | psftp phd2buildbot
   set exitcode=%errorlevel%
   del %%f
  ) 
  goto :end
)

:end
exit /B %exitcode%

