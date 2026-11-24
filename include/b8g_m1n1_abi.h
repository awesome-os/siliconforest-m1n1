#ifndef B8G_M1N1_ABI_H
#define B8G_M1N1_ABI_H

/*
 * Awesome-OS B8G_ABI_MAGIC_V3 + 64-byte aligned B8G_ARENA_V1 C11 boot handoff
 * header for awesome-os/siliconforest-m1n1 (upstream: chriskearney/siliconforest-m1n1, MIT).
 *
 * Bridges m1n1 Apple Silicon stage-1/stage-2 boot handoff (ADT/FDT, framebuffer,
 * warm-RAM arena) to b8g-abi across awesome-os/t2xnu, awesome-os/t2linux (ARM64),
 * awesome-os/siliconforest, and awesome-os/b8g-ghost @ cas-v1.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define B8G_ALIGNAS(n) alignas(n)
#define B8G_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#include <stdalign.h>
#define B8G_ALIGNAS(n) _Alignas(n)
#define B8G_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

#define B8G_ABI_MAGIC_V3         0x33564742u /* "BGV3" */
#define B8G_ARENA_V1_MAGIC       0x31413842u /* "B8A1" */
#define B8G_ARENA_V1_ALIGN       64u
#define B8G_M1N1_ABI_VER         3u

#if defined(_WIN32) || defined(__CYGWIN__)
#  define B8G_M1N1_EXPORT __declspec(dllexport)
#else
#  define B8G_M1N1_EXPORT __attribute__((visibility("default")))
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define B8G_M1N1_UNUSED __attribute__((unused))
#else
#  define B8G_M1N1_UNUSED
#endif

typedef enum B8gM1n1BootTarget {
    B8G_M1N1_TARGET_SILICONFOREST_XNU = 1, /* awesome-os/siliconforest & t2xnu */
    B8G_M1N1_TARGET_T2LINUX_ARM64     = 2, /* awesome-os/t2linux (ARM64) */
    B8G_M1N1_TARGET_B8G_GHOST_CAS_V1  = 3, /* awesome-os/b8g-ghost @ cas-v1 */
    B8G_M1N1_TARGET_CHAINLOAD_M1N1    = 4  /* Stage-2 m1n1 chainload */
} B8gM1n1BootTarget;

typedef enum B8gM1n1Error {
    B8G_M1N1_OK              = 0,
    B8G_M1N1_ERR_INVALID_ARG = -1,
    B8G_M1N1_ERR_BAD_MAGIC   = -2,
    B8G_M1N1_ERR_UNALIGNED   = -3
} B8gM1n1Error;

/*
 * Canonical 64-byte aligned B8G_ARENA_V1 header shared between m1n1 handoff,
 * SiliconForest/t2xnu, t2linux ARM64, and b8g-ghost cas-v1 checkpoints.
 */
typedef struct B8gArenaV1Header {
    B8G_ALIGNAS(64) uint32_t arena_magic; /* B8G_ARENA_V1_MAGIC */
    uint32_t abi_magic;       /* B8G_ABI_MAGIC_V3 */
    uint32_t version;         /* B8G_M1N1_ABI_VER */
    uint32_t flags;           /* Arena capability flags */
    uint64_t base_phys_addr;  /* Physical base address (64-byte aligned) */
    uint64_t byte_length;     /* Total arena size in bytes */
    uint64_t write_cursor;    /* Monotonic ring/arena write offset */
    uint64_t cas_v1_epoch;    /* b8g-ghost @ cas-v1 checkpoint epoch */
    uint64_t reserved[2];     /* Pad to exact 64-byte cacheline */
} B8gArenaV1Header;

B8G_STATIC_ASSERT(sizeof(B8gArenaV1Header) == 64u,
                  "B8gArenaV1Header must be exactly 64 bytes");
B8G_STATIC_ASSERT(alignof(B8gArenaV1Header) == 64u,
                  "B8gArenaV1Header must be 64-byte aligned");

/*
 * 64-byte aligned m1n1 -> OS kernel handoff record (128 bytes = 2 cachelines).
 */
typedef struct B8gM1n1HandoffV3 {
    B8G_ALIGNAS(64) uint32_t abi_magic;   /* B8G_ABI_MAGIC_V3 */
    uint32_t abi_version;     /* B8G_M1N1_ABI_VER */
    uint32_t boot_target;     /* B8gM1n1BootTarget */
    uint32_t boot_flags;      /* Warm-resume / chainload / debug flags */
    uint64_t adt_phys_addr;   /* Apple Device Tree physical base */
    uint64_t fdt_phys_addr;   /* Flattened Device Tree physical base (t2linux) */
    uint64_t fb_phys_addr;    /* Boot framebuffer physical base */
    uint32_t fb_width;        /* Framebuffer width in pixels */
    uint32_t fb_height;       /* Framebuffer height in pixels */
    uint32_t fb_stride;       /* Framebuffer stride in bytes */
    uint32_t fb_bpp;          /* Bits per pixel */
    uint64_t arena_phys_addr; /* Physical address of B8gArenaV1Header */
    uint64_t arena_size;      /* Arena size in bytes */
    uint64_t cas_digest_lo;   /* Low 64 bits of b8g-ghost cas-v1 payload digest */
    uint64_t cas_digest_hi;   /* High 64 bits of b8g-ghost cas-v1 payload digest */
    uint64_t reserved[5];     /* Pad to exact 128 bytes (2 cachelines) */
} B8gM1n1HandoffV3;

B8G_STATIC_ASSERT(sizeof(B8gM1n1HandoffV3) == 128u,
                  "B8gM1n1HandoffV3 must be exactly 128 bytes");
B8G_STATIC_ASSERT(alignof(B8gM1n1HandoffV3) == 64u,
                  "B8gM1n1HandoffV3 must be 64-byte aligned");

static inline B8G_M1N1_UNUSED int32_t b8g_m1n1_validate_handoff(const B8gM1n1HandoffV3 *handoff) {
    if (!handoff) {
        return B8G_M1N1_ERR_INVALID_ARG;
    }
    if (((uintptr_t)handoff & (B8G_ARENA_V1_ALIGN - 1u)) != 0u) {
        return B8G_M1N1_ERR_UNALIGNED;
    }
    if (handoff->abi_magic != B8G_ABI_MAGIC_V3 ||
        handoff->abi_version != B8G_M1N1_ABI_VER) {
        return B8G_M1N1_ERR_BAD_MAGIC;
    }
    return B8G_M1N1_OK;
}

static inline B8G_M1N1_UNUSED int32_t b8g_m1n1_validate_arena(const B8gArenaV1Header *hdr) {
    if (!hdr) {
        return B8G_M1N1_ERR_INVALID_ARG;
    }
    if (((uintptr_t)hdr & (B8G_ARENA_V1_ALIGN - 1u)) != 0u) {
        return B8G_M1N1_ERR_UNALIGNED;
    }
    if (hdr->arena_magic != B8G_ARENA_V1_MAGIC ||
        hdr->abi_magic != B8G_ABI_MAGIC_V3) {
        return B8G_M1N1_ERR_BAD_MAGIC;
    }
    return B8G_M1N1_OK;
}

B8G_M1N1_EXPORT uint32_t b8g_m1n1_abi_magic(void);
B8G_M1N1_EXPORT int32_t b8g_m1n1_prepare_handoff(B8gM1n1HandoffV3 *out_handoff,
                                                 B8gArenaV1Header *arena,
                                                 uint32_t boot_target);

#undef B8G_ALIGNAS
#undef B8G_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif /* B8G_M1N1_ABI_H */
