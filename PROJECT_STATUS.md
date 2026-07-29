# PROJECT_STATUS

## Milestone0 (Current)
M0 - Reproducible QEMU guest environment

### Status
- QEMU/Linux/Buildroot versions: done
- Buildroot guest boot: done
- Root shell: done
- Guest `lspci`: done
- Custom VNPU PCI device: not started

### Evidence
- Boot script: `scripts/run-qemu.sh`
- Observed `lspci`: `00:1f.2 Class 0106: 8086:2922`

## Milestone1
M1 - implement minimal QEMU PCI device skeleton.
