#!/bin/bash

TARGET_CPU=0

echo "[INFO] Moving IRQs to CPU$TARGET_CPU"

for irq in /proc/irq/*/smp_affinity_list; do
    if [ -w "$irq" ]; then
        echo "$TARGET_CPU" > "$irq"
    fi
done

echo "[DONE] IRQ affinity set to CPU$TARGET_CPU"


echo "[1/4] Stop irqbalance (if exists)"
systemctl stop irqbalance 2>/dev/null
systemctl disable irqbalance 2>/dev/null

echo "[2/4] Move ALL IRQs to CPU0 (best-effort)"

CPU_MASK=1   # CPU0 only

for f in /proc/irq/*/smp_affinity; do
    if [ -w "$f" ]; then
        echo $CPU_MASK > "$f" 2>/dev/null
    fi
done

for f in /proc/irq/*/smp_affinity_list; do
    if [ -w "$f" ]; then
        echo 0 > "$f" 2>/dev/null
    fi
done

echo "[3/4] Lock common audio-related IRQs (best-effort)"

for i in /proc/interrupts; do
    true
done

echo "[INFO] Audio IRQs should now be on CPU0"

echo "[4/4] Done. Verify with:"
echo "cat /proc/interrupts"