#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

VNPU_VENDOR="${VNPU_VENDOR:-0x1b36}"
VNPU_DEVICE="${VNPU_DEVICE:-0x1000}"
VNPU_MODULE="${VNPU_MODULE:-vnpu_drv}"
VNPU_KO="${VNPU_KO:-$REPO_ROOT/linux-driver/vnpu-drv.ko}"
VNPU_DEVNODE="${VNPU_DEVNODE:-/dev/vnpu0}"
VNPUCTL="${VNPUCTL:-$REPO_ROOT/tools/vnpuctl}"
BDF="${1:-${VNPU_BDF:-}}"

log()
{
    printf '%s\n' "$*"
}

die()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

find_vnpu_bdf()
{
    for devpath in /sys/bus/pci/devices/*; do
        [ -r "$devpath/vendor" ] || continue
        [ -r "$devpath/device" ] || continue

        vendor="$(cat "$devpath/vendor")"
        device="$(cat "$devpath/device")"

        if [ "$vendor" = "$VNPU_VENDOR" ] && [ "$device" = "$VNPU_DEVICE" ]; then
            printf '%s\n' "${devpath##*/}"
            return 0
        fi
    done

    return 1
}

wait_for_vnpu_bdf()
{
    tries=0
    while [ "$tries" -lt 10 ]; do
        found="$(find_vnpu_bdf || true)"
        if [ -n "$found" ]; then
            printf '%s\n' "$found"
            return 0
        fi

        sleep 1
        tries=$((tries + 1))
    done

    return 1
}

wait_for_devnode()
{
    tries=0
    while [ "$tries" -lt 10 ]; do
        [ -e "$VNPU_DEVNODE" ] && return 0

        sleep 1
        tries=$((tries + 1))
    done

    return 1
}

runtime_reset_best_effort()
{
    if [ -x "$VNPUCTL" ] && [ -e "$VNPU_DEVNODE" ]; then
        log "runtime reset: $VNPUCTL reset --json"
        "$VNPUCTL" reset --json >/dev/null 2>&1 || true
    fi
}

unload_driver()
{
    if grep -q "^$VNPU_MODULE " /proc/modules; then
        log "unload driver: $VNPU_MODULE"
        if ! rmmod "$VNPU_MODULE"; then
            die "failed to unload $VNPU_MODULE; close processes using $VNPU_DEVNODE"
        fi
    else
        log "driver not loaded: $VNPU_MODULE"
    fi
}

load_driver()
{
    [ -f "$VNPU_KO" ] || die "kernel module not found: $VNPU_KO"

    log "load driver: $VNPU_KO"
    if ! insmod "$VNPU_KO"; then
        die "failed to load $VNPU_KO"
    fi
}

[ -d /sys/bus/pci/devices ] || die "PCI sysfs is not available"
[ -w /sys/bus/pci/rescan ] || die "PCI rescan is not writable; run as root"

if [ -z "$BDF" ]; then
    BDF="$(find_vnpu_bdf)" || die "VNPU PCI device $VNPU_VENDOR:$VNPU_DEVICE not found"
fi

[ -d "/sys/bus/pci/devices/$BDF" ] || die "PCI device not found: $BDF"
[ -w "/sys/bus/pci/devices/$BDF/remove" ] || die "PCI remove is not writable for $BDF; run as root"

log "VNPU BDF before reboot: $BDF"
runtime_reset_best_effort
unload_driver

log "remove PCI device: $BDF"
echo 1 > "/sys/bus/pci/devices/$BDF/remove"

log "rescan PCI bus"
echo 1 > /sys/bus/pci/rescan

NEW_BDF="$(wait_for_vnpu_bdf)" || die "VNPU PCI device did not reappear after rescan"
log "VNPU BDF after rescan: $NEW_BDF"

load_driver
wait_for_devnode || die "device node did not appear: $VNPU_DEVNODE"

log "device node ready: $VNPU_DEVNODE"
log "VNPU device reboot complete"
