#!/bin/bash
# cpu_tree_live.sh: Hiển thị load + freq từng core dạng cây live

clear
echo "=== CPU Tree Live Viewer ==="
echo "Core | Load% | Frequency (kHz)"
echo "-----------------------------"

declare -a CPU_TIMES1

for c in $(seq 0 $(( $(nproc --all) - 1 ))); do
    # Lấy tần số hiện tại
    freq=$(cat /sys/devices/system/cpu/cpu${c}/cpufreq/scaling_cur_freq 2>/dev/null)
    [[ -z $freq ]] && freq=0

    # Đọc /proc/stat lần 1
    read cpu_label user nice system idle iowait irq softirq steal guest < /proc/stat
    total1=$((user + nice + system + idle + iowait + irq + softirq + steal))
    idle1=$((idle + iowait))
    CPU_TIMES1[$c]="$total1 $idle1"
done

# Sleep để tính delta
sleep 0.5

for c in $(seq 0 $(( $(nproc --all) - 1 ))); do
    read cpu_label user nice system idle iowait irq softirq steal guest < /proc/stat
    total2=$((user + nice + system + idle + iowait + irq + softirq + steal))
    idle2=$((idle + iowait))

    total1=${CPU_TIMES1[$c]% *}
    idle1=${CPU_TIMES1[$c]#* }

    total_delta=$((total2 - total1))
    idle_delta=$((idle2 - idle1))
    load=0
    if [ $total_delta -ne 0 ]; then
        load=$((100 * (total_delta - idle_delta) / total_delta))
    fi

    freq=$(cat /sys/devices/system/cpu/cpu${c}/cpufreq/scaling_cur_freq 2>/dev/null)
    [[ -z $freq ]] && freq=0

    prefix="├─"
    [[ $c -eq $(( $(nproc --all) - 1 )) ]] && prefix="└─"
    echo "$prefix CPU$c: $load% | ${freq} kHz"
done
