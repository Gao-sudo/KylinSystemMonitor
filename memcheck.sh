#!/bin/bash

echo "========================================="
echo "Memory Leak Detection for Kylin System Monitor"
echo "========================================="

PROJECT_DIR="/home/china/桌面/KylinSystemMonitor"
cd $PROJECT_DIR

# 1. 编译项目
echo ""
echo "[1] Building project..."
make clean > /dev/null 2>&1
qmake > /dev/null 2>&1
make -j4 > /dev/null 2>&1

if [ ! -f ./KylinSystemMonitor ]; then
    echo "Build failed or executable not found"
    exit 1
fi

# 2. 使用 Valgrind 检测内存泄漏
echo ""
echo "[2] Running Valgrind memcheck (15 seconds)..."
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind_memcheck.txt \
         ./KylinSystemMonitor &
PID=$!

sleep 15
kill $PID 2>/dev/null

# 3. 显示结果
echo ""
echo "[3] Valgrind Memory Leak Summary:"
echo "-----------------------------------"
if [ -f valgrind_memcheck.txt ]; then
    grep -E "LEAK SUMMARY|definitely lost|indirectly lost|possibly lost" valgrind_memcheck.txt
    echo ""
    echo "Full report saved to: valgrind_memcheck.txt"
else
    echo "Valgrind report not generated"
fi

echo ""
echo "Memory check completed!"
