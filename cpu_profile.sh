#!/bin/bash

echo "CPU Profiling..."

# 使用 perf stat 统计
sudo perf stat -e cycles,instructions,cache-misses,cache-references \
                -p $(pgrep -f KylinSystemMonitor) sleep 10

# 使用 top 记录 CPU 使用率
top -b -n 5 -d 1 | grep -E "KylinSystemMonitor|%Cpu" > cpu_usage.log

echo "CPU profile saved to cpu_usage.log"
