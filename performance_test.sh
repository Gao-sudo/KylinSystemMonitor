#!/bin/bash

echo "========================================="
echo "Performance Test for Kylin System Monitor"
echo "========================================="

PROJECT_DIR="/home/china/桌面/KylinSystemMonitor"
cd $PROJECT_DIR

# 编译项目
echo ""
echo "[1] Building project..."
make clean > /dev/null 2>&1
qmake > /dev/null 2>&1
make -j4 > /dev/null 2>&1

if [ ! -f ./KylinSystemMonitor ]; then
    echo "Build failed!"
    exit 1
fi

# 启动程序
echo ""
echo "[2] Starting application..."
./KylinSystemMonitor &
APP_PID=$!
echo "Application PID: $APP_PID"

sleep 3

# CPU 和内存使用情况
echo ""
echo "[3] Resource usage (10 seconds)..."
echo "Time       CPU%   MEM(MB)"
for i in {1..10}; do
    if kill -0 $APP_PID 2>/dev/null; then
        CPU=$(ps -p $APP_PID -o %cpu --no-headers 2>/dev/null | tr -d ' ')
        RSS=$(ps -p $APP_PID -o rss --no-headers 2>/dev/null | tr -d ' ')
        MEM_MB=$((RSS / 1024))
        echo "$(date +%H:%M:%S)  ${CPU}%    ${MEM_MB}MB"
    else
        echo "Application stopped"
        break
    fi
    sleep 1
done

# 停止程序
echo ""
echo "[4] Stopping application..."
kill $APP_PID 2>/dev/null
wait $APP_PID 2>/dev/null

# 内存检查
echo ""
echo "[5] Memory check with Valgrind..."
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --log-file=valgrind_perf.txt \
         ./KylinSystemMonitor &
VALGRIND_PID=$!
sleep 10
kill $VALGRIND_PID 2>/dev/null

echo ""
echo "[6] Memory leak summary:"
grep -E "LEAK SUMMARY|definitely lost|indirectly lost" valgrind_perf.txt 2>/dev/null

echo ""
echo "========================================="
echo "Performance test completed!"
echo "Reports:"
echo "  - valgrind_perf.txt"
echo "========================================="
