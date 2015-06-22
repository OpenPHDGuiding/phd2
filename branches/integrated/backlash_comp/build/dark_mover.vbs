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
Dim retCode
Dim endLoc

On Error Resume Next
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
        endLoc = endLoc & "\darks_defects"
        if (not oFSO.FolderExists(endLoc)) then
            ' msgbox("Creating dark_defects folder")
            set oFolder = oFSO.CreateFolder(endLoc)
        end if
	if (Err.Number = 0) then
	    ' msgbox("Doing CopyFolder: " & startLoc & "->" & endLoc)
	    oFSO.CopyFolder startLoc, endLoc
            if (Err.Number = 0) then
	        ' msgbox("Dark calibration files copied to new location, now deleting source directory...")
                oFSO.DeleteFolder(startLoc)
                if (Err.Number = 0) then 
                    ' msgbox("Deleted old directory...")
                    retCode = 0
                else
                    ' msgbox("Delete old directory failed: " + Err.Description)
                         retCode = 1
                end if
            else
                ' msgbox("Copy got hosed: " + Err.Description)
                    retCode = 2
            end if
	else
	    ' msgbox("Could not create folder hierarchy: " + Err.Description)
            retCode =3
	end if
else
	' msgbox("No files present...")
        retCode = 4
end if
WScript.Quit retCode
