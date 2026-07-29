# Version Record

Last updated: 2026-07-29

## QEMU

| Item | Version | Source |
|---|---|---|
| Buildroot host QEMU | 10.2.0 | `/home/isa_codex/src/buildroot/output/host/bin/qemu-system-x86_64 --version` |
| System QEMU | 8.2.2, Debian `1:8.2.2+ds-0ubuntu1.17` | `qemu-system-x86_64 --version` |

Current Buildroot `start-qemu.sh` prepends `/home/isa_codex/src/buildroot/output/host/bin` to `PATH` unless `--use-system-qemu` is passed. Therefore the default generated launcher uses Buildroot host QEMU 10.2.0.

## Linux Kernel

| Item | Value | Source |
|---|---|---|
| Kernel version | 6.18.7 | `file output/images/bzImage` and Buildroot `.config` |
| Kernel image | `output/images/bzImage` | present in project output directory |
| Build directory | `/home/isa_codex/src/buildroot/output/build/linux-6.18.7` | Buildroot output tree |
| Kernel config | `board/qemu/x86_64/linux.config` | `BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE` |

Observed image metadata:

```text
Linux kernel x86 boot executable bzImage, version 6.18.7
```

## Buildroot

| Item | Value | Source |
|---|---|---|
| Buildroot version | 2026.02.2 | `git -C /home/isa_codex/src/buildroot describe --tags --always --dirty` |
| Buildroot commit | `71d1dddae12e7cbf322d96864d7757f459801f47` | `git -C /home/isa_codex/src/buildroot rev-parse HEAD` |
| Buildroot path | `/home/isa_codex/src/buildroot` | local source tree |
| Target architecture | x86_64 | `BR2_x86_64=y` |
| Root filesystem image | `output/images/rootfs.ext4` | present in project output directory |
| Root filesystem size | 60 MiB | `BR2_TARGET_ROOTFS_EXT2_SIZE="60M"` |
| Root filesystem format | ext4 | `BR2_TARGET_ROOTFS_EXT2_4=y` |

## Artifact Hashes

```text
bb314752121210e86ecb5b8d24537001d821a9bc3a0c4d05ecd6299194d6a678  output/images/bzImage
3959025b08a9739fca10dc99564175fca51e7e270974e08cb504dbd334bb8220  output/images/rootfs.ext4
```

## Boot Evidence

| Item | Result |
|---|---|
| Boot script | `scripts/run-qemu.sh` |
| QEMU machine | `q35` |
| Kernel image | `output/images/bzImage` |
| Root filesystem | `output/images/rootfs.ext4` |
| Guest root shell | reached |
| Guest `lspci` | success |

Observed `lspci` output:

```text
00:1f.2 Class 0106: 8086:2922
```

## Verified Commands

```bash
qemu-system-x86_64 --version
/home/isa_codex/src/buildroot/output/host/bin/qemu-system-x86_64 --version
git -C /home/isa_codex/src/buildroot describe --tags --always --dirty
git -C /home/isa_codex/src/buildroot rev-parse HEAD
find /home/isa_codex/src/buildroot/output/build -maxdepth 1 -type d -name 'linux*' -printf '%f\n'
file output/images/bzImage
rg '^(BR2_VERSION|BR2_LINUX_KERNEL|BR2_PACKAGE_HOST_QEMU|BR2_TARGET_ROOTFS_EXT2|BR2_x86_64)' /home/isa_codex/src/buildroot/.config
sha256sum output/images/bzImage output/images/rootfs.ext4
```
