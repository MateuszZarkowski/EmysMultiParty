

REM // NOTE: use DEPENDENCY WALKER  to check if some debug libraries may be missing
REM //       the best way to do it is to compare dependecies for release and debug DLLs.
REM -----------DEBUG----------------------
copy /Y UFiles\Debug\UFiles.dll .
..\2.7.5\bin\urbi-d.exe --port 54000 --host 0.0.0.0 _start.u

REM -----------RELEASE--------------------
REM cp UFiles\Release\UFiles.dll .
REM ..\2.7.5\bin\urbi.exe --port 54000 --host 0.0.0.0 _start.u


