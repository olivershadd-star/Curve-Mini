@echo off
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release --target curve_tests motion_tests lane_math_tests processor_tests CurveMini_VST3 CurveMini_Standalone --parallel 2
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir build -C Release --output-on-failure
