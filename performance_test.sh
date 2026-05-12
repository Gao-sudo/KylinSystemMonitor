#!/bin/bash

echo "========================================="
echo "Kylin System Monitor Performance Test"
echo "========================================="

PROJECT_DIR="/home/china/桌面/KylinSystemMonitor"
cd $PROJECT_DIR

# 1. 编译项目
echo ""
echo "[1] Building project..."
make clean
qmake
make -j4

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# 2. Valgrind 内存检测
echo ""
echo "[2] Running Valgrind memory check..."
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind_report.txt \
         ./KylinSystemMonitor &
VALGRIND_PID=$!

sleep 15
kill $VALGRIND_PID 2>/dev/null

echo "Valgrind report saved to valgrind_report.txt"

# 3. 分析 Valgrind 结果
echo ""
echo "[3] Valgrind Summary:"
echo "--------------------"
grep -E "definitely lost|indirectly lost|possibly lost|still reachable" valgrind_report.txt | head -10

# 4. Perf 性能分析
echo ""
echo "[4] Running perf performance analysis..."
sudo perf record -g -F 99 -o perf.data -- ./KylinSystemMonitor &
PERF_PID=$!

sleep 15
sudo kill $PERF_PID 2>/dev/null
sudo perf report -i perf.data --stdio > perf_report.txt

echo "Perf report saved to perf_report.txt"

# 5. Perf 结果摘要
echo ""
echo "[5] Perf Summary (Top 10 functions):"
echo "-----------------------------------"
grep -A 30 "Samples:" perf_report.txt | head -40

# 6. CPU 使用率分析
echo ""
echo "[6] CPU Usage Analysis:"
pidstat -p $$ 1 10 2>/dev/null

# 7. 内存使用分析
echo ""
echo "[7] Memory Usage Analysis:"
pmap $$ | tail -1 2>/dev/null

echo ""
echo "========================================="
echo "Performance analysis completed!"
echo "Reports:"
echo "  - valgrind_report.txt"
echo "  - perf_report.txt"
echo "========================================="
