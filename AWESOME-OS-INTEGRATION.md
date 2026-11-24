# Awesome-OS Integration — `awesome-os/siliconforest-m1n1`

> **Upstream Origin:** [`chriskearney/siliconforest-m1n1`](https://github.com/chriskearney/siliconforest-m1n1) (fork of [`AsahiLinux/m1n1`](https://github.com/AsahiLinux/m1n1), MIT License, Copyright © The Asahi Linux Contributors & Chris Kearney; third-party licenses preserved in [`LICENSE`](LICENSE) and [`3rdparty_licenses/`](3rdparty_licenses/)).

`awesome-os/siliconforest-m1n1` preserves Chris Kearney's Apple Silicon `m1n1` stage-1/stage-2 bootloader and experimentation playground intact while adding a **C11 `B8G_ABI_MAGIC_V3` (`0x33564742`) + 64-byte aligned `B8G_ARENA_V1`** boot handoff header ([`include/b8g_m1n1_abi.h`](include/b8g_m1n1_abi.h)) for the Awesome-OS 4-layer stack.

---

## Pairing Matrix

| Awesome-OS Repository | Role | How `awesome-os/siliconforest-m1n1` Pairs With It |
| :--- | :--- | :--- |
| **`awesome-os/t2xnu`** (`T2macOS`) | Open Darwin / XNU kernel & Apple platform bring-up | Provides stage-1/stage-2 Apple Silicon chainloading, ADT/boot-args handoff, and `B8G_ABI_MAGIC_V3` boot descriptors (`include/b8g_m1n1_abi.h`) to boot Darwin/XNU kernels (`awesome-os/siliconforest` and `awesome-os/t2xnu`) natively without a hypervisor. |
| **`awesome-os/t2linux`** (`ARM64`) | Linux kernel & driver distribution for Apple T2 and Apple Silicon (ARM64) | Boots `t2linux` ARM64 `Image.gz` + FDT + initramfs payloads via standard `m1n1` payload concatenation while exposing the same 64-byte aligned `B8G_ARENA_V1` warm-RAM and framebuffer handoff struct. |
| **`awesome-os/b8g-ghost` `@ cas-v1`** | Content-addressed `.b8p` CAS image & warm-RAM checkpointing store | Anchors `m1n1` stage-2 payloads, kernel collections, and `cas-v1` initramfs/warm-RAM epochs in `B8gM1n1HandoffV3`, allowing verified $O(1)$ boot rollback and deterministic multi-OS selection. |
| **`awesome-os/siliconforest`** | Native Apple Silicon Darwin/XNU OS + Asahi kexts | Consumes the `B8gM1n1HandoffV3` and `B8gArenaV1Header` structures during early kernel initialization and DCP framebuffer bring-up. |

---

## C11 ABI Bridge (`include/b8g_m1n1_abi.h`)

The header [`include/b8g_m1n1_abi.h`](include/b8g_m1n1_abi.h) provides:

- `B8G_ABI_MAGIC_V3` (`0x33564742u`) and `B8G_ARENA_V1_ALIGN` (`64u`) compile-time invariants verified with C11 `_Static_assert`.
- `B8gArenaV1Header` (64-byte aligned) matching `awesome-os/siliconforest` and `sectium-cloud-control-plane`.
- `B8gM1n1HandoffV3` (64-byte aligned) carrying ADT/FDT physical addresses, framebuffer base/geometry, target OS selector (`t2xnu` / `t2linux` / `b8g-ghost`), and `B8G_ARENA_V1` pointer.

### Local Verification

```sh
gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only include/b8g_m1n1_abi.h
```
