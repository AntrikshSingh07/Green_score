@echo off
echo ===== Green Score Project =====
echo.
echo Compiling project...
g++ -std=c++17 main.cpp company.cpp graph.cpp GreenScoreCalculator.cpp InfluenceCalculator.cpp ScalingFactor.cpp MaxFlowCalculator.cpp -o green_score_analysis

if %errorlevel% neq 0 (
    echo.
    echo Compilation failed. Please check the error messages above.
    pause
    exit /b %errorlevel%
)

echo.
echo Compilation successful!
echo.
echo Running green score analysis...
echo.
green_score_analysis

echo.
echo.
echo Analysis complete.
echo.
echo Would you like to also run the visualization? (y/n)
set /p choice="Choice: "
if /i "%choice%"=="y" (
    echo.
    echo Running visualization...
    python visualize.py
)

echo.
echo Done.
pause
