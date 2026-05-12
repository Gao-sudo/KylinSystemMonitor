#!/bin/bash

echo "Memory Leak Detection..."

# 使用 Valgrind 详细检测
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind_detailed.txt \
         ./KylinSystemMonitor &

PID=$!
sleep 20
kill $PID

echo "Detailed memory report saved to valgrind_detailed.txt"

# 分析内存泄漏
echo ""
echo "Memory Leak Summary:"
grep -E "LEAK SUMMARY|definitely lost|indirectly lost" valgrind_detailed.txt
