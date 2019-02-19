cftisio requires a patch to build with MS Studio 2013
  cfitsio-snprintf.patch
thirdparty.cmake expects the patch to already have been applied, so
when upgrading cfitsio make sure to apply the patch and commit the
patched cfitsio sources to the repo
 - AG
