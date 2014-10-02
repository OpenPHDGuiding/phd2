Option Explicit
Const LOCAL_APPLICATION_DATA = &H1c&
Const MY_DOCUMENTS = &H05&
Dim oFSO
Dim oShell
Dim oScript
Dim oFolder
Dim oMyDocsFolderItem
Dim oAppDataFolderItem
Dim startLoc
Dim endLoc

set oShell = CreateObject("Shell.Application")
set oFolder = oShell.Namespace(MY_DOCUMENTS)
set oMyDocsFolderItem = oFolder.Self
startLoc = oMyDocsFolderItem.Path & "\phd2\darks_defects"
set oFolder = oShell.Namespace(LOCAL_APPLICATION_DATA)
set oAppDataFolderItem = oFolder.Self
endLoc = oAppDataFolderItem.Path & "\phd2"
' msgbox (startLoc & " -> " & endLoc & "\darks_defects")
'
set oFSO = CreateObject("scripting.FileSystemObject")
if (oFSO.FolderExists(startLoc)) then
	' Need to build top-level destination folder structure ourselves
	if (not oFSO.FolderExists(endLoc)) then
		' msgbox("Creating AppData\Local\PHD2 directory")
	    set oFolder = oFSO.CreateFolder(endLoc)
	end if
	if (not oFSO.FolderExists(endLoc & "\darks_defects")) then
		oFSO.MoveFolder startLoc, endLoc & "\"
		' msgbox("Dark calibration files moved to new location...")
	' else
		' msgbox("Files already in new location...")
	end if
' else
	' msgbox("No files present...")
end if

