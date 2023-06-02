echo off
set name=%DATE:~0,4%%DATE:~5,2%%DATE:~8,2%%TIME:~0,2%%TIME:~3,2%%TIME:~6,2% 
7z.exe a -t7z back-%name%.7z .\*.* -r  -x!*Release -x!*Debug -x!*git -x!*ipch -x!*.db -x!*.zip -x!*.7z -x!*.rar -x!*.opendb -x!*.log* -x!*.doc -x!*.pdf -x!*.dll -x!*.exe -x!*.jar -x!*.dat -x!*.jpg -x!*.bmp -x!*.mp4 -x!*.avi -x!*.h264 -x!*bin -x!*build-* 
pause