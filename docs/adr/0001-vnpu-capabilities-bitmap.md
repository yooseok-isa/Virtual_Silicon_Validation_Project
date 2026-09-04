# ADR 0001: VNPU Capabilities Bitmap

Date: 2026-09-03

## Status

Accepted

## Context

The VNPU register map includes a read-only `CAPABILITIES` register at BAR0
offset `0x008`. Revision A originally treated this register as reserved and
returned `0`. Revision B adds hardware-visible behavior such as vector length
16 and `JOB_ID`, while preserving length-8 operation for compatibility. Software
needs a compact way to verify that the expected features are present after it
identifies the hardware revision.

## Decision

`CAPABILITIES` is now defined as a feature bitmap from Revision B onward.
Revision A keeps returning `0x00000000` for compatibility with existing
Revision A behavior. Revision B returns `0x0000007D`, advertising signed INT8
dot product, vector length 16, `JOB_ID`, fault injection, completion IRQ, and
error IRQ support. Revision B also accepts vector length 8 as a revision-defined
compatibility behavior without changing the expected capability value from
`0x0000007D`.

Software must validate `REVISION` first. For Revision A, `CAPABILITIES == 0` is
valid. For Revision B, the driver and HAL must verify the documented Revision B
capability bits before exposing Revision B functionality.

## Consequences

- Revision A tests and evidence remain valid.
- Revision B feature checks become explicit instead of implied only by revision.
- Future revisions can add feature bits without changing the basic register
  layout.
