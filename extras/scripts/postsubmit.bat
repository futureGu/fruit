
echo on

set PATH=C:\Windows\system32;C:\Windows;%PYTHON3_PATH%;%PYTHON3_PATH%\Scripts;%CMAKE_PATH%;

if not "%VCVARSALL_DIR%" == "" CALL "%VCVARSALL_DIR%\vcvarsall.bat" amd64

if not "%MINGW_PATH%" == "" SET PATH=%PATH%%MINGW_PATH%;

setx PATH "%PATH%"

mkdir C:\Fruit\build-%CONFIGURATION%
cd C:\Fruit\build-%CONFIGURATION%

cmake.exe -G "%CMAKE_GENERATOR%" .. -DCMAKE_BUILD_TYPE=%CONFIGURATION% %ADDITIONAL_CMAKE_ARGS% || exit /b 1

echo "Content of CMakeFiles\CMakeError.log:"
if exist "CMakeFiles\CMakeError.log" (
   type "CMakeFiles\CMakeError.log"
)

IF "%CMAKE_GENERATOR%"=="MinGW Makefiles" (
  mingw32-make -j12 || exit /b 1
) ELSE (
  msbuild ALL_BUILD.vcxproj || exit /b 1
)

pip3 install pytest
pip3 install pytest-xdist

cd tests
py.test -r a -n 1 || exit /b 1
