
@echo off
REM windows magic to get date indepentent of locale 
for /F "usebackq tokens=1,2 delims==" %%i in (`wmic os get LocalDateTime /VALUE 2^>NUL`) do if '.%%i.'=='.LocalDateTime.' set ldt=%%j
set ldt=%ldt:~0,4%-%ldt:~4,2%-%ldt:~6,2%_%ldt:~8,2%%ldt:~10,2%
@echo on

rmdir /q log
mkdir log
copy /Y start.log log\
copy /Y server.log log\
copy /Y ~features.txt log\

move /Y log AllLogs\%ldt%
