#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

SOURCE_FILE="${VNPU_SOURCE_FILE:-$REPO_ROOT/qemu-device/src/vnpu.c}"
PATCH_FILE="${VNPU_PATCH_FILE:-$SCRIPT_DIR/0001-hw-misc-add-vnpu-pci-device.patch}"
CHECK_ONLY=0
VERIFY_QEMU_DIR=""

usage() {
    cat <<EOF
Usage: $(basename "$0") [--check] [--verify-qemu <qemu-source-dir>]
       $(basename "$0") [--source <vnpu.c>] [--output <patch-file>]

Regenerates the single QEMU VNPU patch from qemu-device/src/vnpu.c.

Options:
  --check                   Exit nonzero if the patch file is stale.
  --verify-qemu <dir>       Run 'patch --dry-run -p1' in a QEMU source tree.
  --source <vnpu.c>         Override the source vnpu.c file.
  --output <patch-file>     Override the output patch file.
  -h, --help                Show this help.

Environment:
  VNPU_SOURCE_FILE          Source vnpu.c path.
  VNPU_PATCH_FILE           Output patch path.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --check)
            CHECK_ONLY=1
            ;;
        --verify-qemu)
            if [ "$#" -lt 2 ]; then
                echo "error: --verify-qemu requires a directory" >&2
                exit 2
            fi
            VERIFY_QEMU_DIR="$2"
            shift
            ;;
        --source)
            if [ "$#" -lt 2 ]; then
                echo "error: --source requires a file" >&2
                exit 2
            fi
            SOURCE_FILE="$2"
            shift
            ;;
        --output)
            if [ "$#" -lt 2 ]; then
                echo "error: --output requires a file" >&2
                exit 2
            fi
            PATCH_FILE="$2"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

generate_patch() {
    local output_file="$1"
    local blob_hash
    local line_count

    if [ ! -f "$SOURCE_FILE" ]; then
        echo "error: source file not found: $SOURCE_FILE" >&2
        exit 1
    fi

    blob_hash="$(git hash-object "$SOURCE_FILE")"
    blob_hash="${blob_hash:0:7}"
    line_count="$(wc -l < "$SOURCE_FILE")"
    mkdir -p "$(dirname "$output_file")"

    {
        cat <<'PATCH_HEADER'
diff --git a/hw/misc/Kconfig b/hw/misc/Kconfig
index fccd735..b16e51e 100644
--- a/hw/misc/Kconfig
+++ b/hw/misc/Kconfig
@@ -30,6 +30,11 @@ config EDU
     default y if TEST_DEVICES
     depends on PCI && MSI_NONBROKEN
 
+config VNPU
+    bool
+    default y if TEST_DEVICES
+    depends on PCI
+
 config I2C_ECHO
     bool
     default y if TEST_DEVICES
diff --git a/hw/misc/meson.build b/hw/misc/meson.build
index b1d8d8e..b2c5156 100644
--- a/hw/misc/meson.build
+++ b/hw/misc/meson.build
@@ -1,5 +1,6 @@
 system_ss.add(when: 'CONFIG_APPLESMC', if_true: files('applesmc.c'))
 system_ss.add(when: 'CONFIG_EDU', if_true: files('edu.c'))
+system_ss.add(when: 'CONFIG_VNPU', if_true: files('vnpu.c'))
 system_ss.add(when: 'CONFIG_FW_CFG_DMA', if_true: files('vmcoreinfo.c'))
 system_ss.add(when: 'CONFIG_ISA_DEBUG', if_true: files('debugexit.c'))
 system_ss.add(when: 'CONFIG_ISA_TESTDEV', if_true: files('pc-testdev.c'))
diff --git a/hw/misc/vnpu.c b/hw/misc/vnpu.c
new file mode 100644
PATCH_HEADER
        printf 'index 0000000..%s\n' "$blob_hash"
        cat <<'PATCH_HEADER'
--- /dev/null
+++ b/hw/misc/vnpu.c
PATCH_HEADER
        printf '@@ -0,0 +1,%s @@\n' "$line_count"
        sed 's/^/+/' "$SOURCE_FILE"
    } > "$output_file"
}

if [ "$CHECK_ONLY" -eq 1 ]; then
    tmp_file="$(mktemp)"
    trap 'rm -f "$tmp_file"' EXIT
    generate_patch "$tmp_file"

    if cmp -s "$PATCH_FILE" "$tmp_file"; then
        echo "patch is up to date: $PATCH_FILE"
    else
        echo "error: patch is stale: $PATCH_FILE" >&2
        diff -u "$PATCH_FILE" "$tmp_file" || true
        exit 1
    fi
else
    generate_patch "$PATCH_FILE"
    echo "updated patch: $PATCH_FILE"
fi

if [ -n "$VERIFY_QEMU_DIR" ]; then
    if [ ! -d "$VERIFY_QEMU_DIR" ]; then
        echo "error: QEMU source directory not found: $VERIFY_QEMU_DIR" >&2
        exit 1
    fi

    patch --dry-run -d "$VERIFY_QEMU_DIR" -p1 < "$PATCH_FILE"
fi
