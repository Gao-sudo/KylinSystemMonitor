#!/bin/bash

echo "========================================="
echo "CPU Profiling for Kylin System Monitor"
echo "========================================="

PROJECT_DIR="/home/china/桌面/KylinSystemMonitor"
cd $PROJECT_DIR

# 1. 编译项目
echo ""
echo "[1] Building project..."
make -j4 2>/dev/null

if [ ! -f ./KylinSystemMonitor ]; then
    echo "Build failed or executable not found"
    exit 1
fi

# 2. 后台运行程序
echo ""
echo "[2] Starting KylinSystemMonitor in background..."
./KylinSystemMonitor &
PID=$!

sleep 3

# 3. 检查进程是否运行
if ! kill -0 $PID 2>/dev/null; then
    echo "Failed to start application"
    exit 1
fi

echo "Application running with PID: $PID"

# 4. 使用 top 监控 CPU
echo ""
echo "[3] CPU usage (top) - 10 seconds..."
top -b -n 5 -d 1 -p $PID 2>/dev/null | grep -E "$PID|%Cpu"

# 5. 使用 ps 查看 CPU 使用率
echo ""
echo "[4] CPU usage (ps) - 10 seconds..."
for i in {1..5}; do
    CPU=$(ps -p $PID -o %cpu --no-headers 2>/dev/null)
    if [ -n "$CPU" ]; then
        echo "$(date +%H:%M:%S) - CPU: $CPU%"
    fi
    sleep 2
done

# 6. 使用 pidstat（如果可用）
echo ""
echo "[5] CPU statistics (pidstat)..."
if command -v pidstat &> /dev/null; then
    pidstat -p $PID 1 5 2>/dev/null
else
    echo "pidstat not available, install sysstat: sudo apt install sysstat"
fi

# 7. 使用 perf（如果可用）
echo ""
echo "[6] Performance counters (perf)..."
if command -v perf &> /dev/null; then
    echo "Collecting perf data for 5 seconds..."
    sudo perf stat -e cycles,instructions,cache-misses,cache-references \
         -p $PID 2>&1 | head -20
else
    echo "perf not available, install: sudo apt install linux-tools-common"
fi

# 8. 停止程序
echo ""
echo "[7] Stopping application..."
kill $PID 2>/dev/null
sleep 1

echo ""
echo "CPU profiling completed!"
