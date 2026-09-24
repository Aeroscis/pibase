/*
 * pibase - the PI family base layer
 *
 * One dependency-free C header holding the vocabulary that every PI library
 * shares, and deliberately nothing else:
 *
 *   platform / ABI plumbing   PI_PLATFORM_*, PI_EXPORT, PI_IMPORT, PI_LOCAL, PI_CALL
 *   the result-code list      PiResult, PI_OK / PI_FAIL / PI_E_* / PI_SUCCEEDED / PI_FAILED
 *   the 128-bit identifier    PiGuid, PI_GUID(), pi_guid_equal()
 *   the native window handle  PiNativeWindow, PI_INVALID_WINDOW, PI_IS_VALID_WINDOW
 *   the COM-style root        IPiUnknown(Vtbl), PiRefCountedBase, pi_refcounted_*()
 *
 * ---------------------------------------------------------------------------
 * Why this is its own layer, and how names are scoped
 * ---------------------------------------------------------------------------
 *
 * PI is a family of libraries, each with its own name, its own headers and its
 * own release cycle. The names above belong to none of them: a result code, a
 * GUID, a window handle and an interface root mean the same thing in every
 * member, and every member needs them in the signatures of its own API. They
 * live here so that two members can meet at a boundary without either one
 * having to depend on the other.
 *
 * The naming rule follows from that, and this header is where it is stated:
 *
 *   family root vocabulary    bare pi_ / Pi / PI_         (defined here)
 *   a member's own names      pi_<member>_ / Pi<Member> / PI_<MEMBER>_
 *
 * So `PiResult` is family-level and stays bare, while a member prefixes
 * everything it adds itself. A reader can tell from the identifier alone
 * whether a name is shared across the family or owned by one member.
 *
 * ---------------------------------------------------------------------------
 * Header-only, on purpose
 * ---------------------------------------------------------------------------
 *
 * Everything here is a type, a macro, or a `static inline` function. Depending
 * on pibase therefore adds no binary, no link step and - the point of the
 * exercise - no binary ABI that could ever need freezing. This is the one layer
 * that must never break, so it is the one layer with no ABI to break: the
 * contract is this header, and a consumer recompiles against it.
 *
 * That is also the test for anything proposed for inclusion. A name earns a
 * place here only by appearing in the signatures of more than one member's API.
 * Containers, strings, allocators, threads, I/O and parsers all fail that test:
 * they are features a library may offer, not vocabulary the family shares, and
 * a base layer that starts absorbing them stops being a base layer. Platform
 * code in particular must stay at the level of a typedef or an intrinsic -
 * the day this header needs a real platform abstraction is the day it has grown
 * past its job.
 */
#ifndef PIBASE_PI_BASE_H
#define PIBASE_PI_BASE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Platform detection
 *
 * Only the macro matching the current platform is defined; the others stay
 * undefined, so both `#if PI_PLATFORM_X` and `#ifdef PI_PLATFORM_X` behave.
 * -------------------------------------------------------------------------- */
#if defined(_WIN32) || defined(_WIN64)
    #define PI_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define PI_PLATFORM_MACOS 1
#else
    #define PI_PLATFORM_LINUX 1
#endif

/* Interlocked intrinsics for the reference count below. <intrin.h> rather than
 * <windows.h>: this is a public header, and windows.h would drag its whole
 * macro namespace (min/max, near/far, ...) into every consumer. */
#if PI_PLATFORM_WINDOWS
    #include <intrin.h>
#endif

/* --------------------------------------------------------------------------
 * Visibility attributes
 *
 * The attribute is the family's business (it is the same on every member); the
 * *switch* is each member's, because only it knows which binary is being built.
 * A member composes its own marker on top of its own "building" macro:
 *
 *     #ifdef MYLIB_BUILDING
 *     #  define MYLIB_API PI_EXPORT
 *     #else
 *     #  define MYLIB_API PI_IMPORT
 *     #endif
 *
 * pibase itself exports nothing (everything below is `static inline`), so these
 * exist purely as the vocabulary members build on.
 * -------------------------------------------------------------------------- */
#if PI_PLATFORM_WINDOWS
    #define PI_EXPORT __declspec(dllexport)
    #define PI_IMPORT __declspec(dllimport)
    #define PI_LOCAL
#else
    #define PI_EXPORT __attribute__((visibility("default")))
    #define PI_IMPORT __attribute__((visibility("default")))
    #define PI_LOCAL  __attribute__((visibility("hidden")))
#endif

/* Calling convention: __stdcall on Windows, for maximum FFI compatibility.
 * Must be defined before any vtable that uses it. */
#ifndef PI_CALL
    #if PI_PLATFORM_WINDOWS
        #define PI_CALL __stdcall
    #else
        #define PI_CALL
    #endif
#endif

/* --------------------------------------------------------------------------
 * Result codes
 *
 * The shared status vocabulary: every member returns PiResult from its own API,
 * so the list has to be one list. Generic codes live here; a code that only
 * makes sense to one member may still live here (a shared vocabulary is allowed
 * to contain words only some members use - that is how HRESULT works), but a
 * member is also free to define its own in its own range.
 * -------------------------------------------------------------------------- */
typedef int32_t PiResult;

#define PI_OK                  ((PiResult)0)
#define PI_FAIL                ((PiResult) - 1)
#define PI_E_NOINTERFACE       ((PiResult) - 2)
#define PI_E_INVALIDARG        ((PiResult) - 3)
#define PI_E_OUTOFMEMORY       ((PiResult) - 4)
#define PI_E_NOTIMPL           ((PiResult) - 5)
#define PI_E_UNEXPECTED        ((PiResult) - 6)
#define PI_E_NOTFOUND          ((PiResult) - 7)
#define PI_E_MISSINGCAPABILITY ((PiResult) - 8) /* required host capability absent */
#define PI_E_VERSIONMISMATCH   ((PiResult) - 9) /* api_version incompatible */

#define PI_SUCCEEDED(r) ((PiResult)(r) >= 0)
#define PI_FAILED(r)    ((PiResult)(r) < 0)

/* Partition rule, mirroring the `>= 0x80000000` convention used for message
 * codes, event types and interface numbering: values <= -100 belong to apps and
 * libraries, so the framework can add codes above them (-10, -11, ...) without
 * ever colliding with something defined outside it.
 *
 * The sign is not a style choice. PI_SUCCEEDED / PI_FAILED decide by sign, so a
 * custom *error* code must be negative. Conversely, an interface that needs to
 * express "accepted, result will follow later" is describing SUCCESS, and must
 * introduce a positive result code for it - expressing it as a negative one
 * makes PI_FAILED() lie about the question it claims to answer, and it lies
 * quietly: code that writes its failure branch as `r < 0` silently classifies
 * "accepted" as "failed". */
#define PI_APP_RESULT_BASE ((PiResult) - 100)

/* --------------------------------------------------------------------------
 * 128-bit GUID (the RFC 4122 / COM binary layout)
 *
 * The layout is version-agnostic on purpose: UUID v1, v3, v4 and v5 all share
 * these 16 bytes, with the version in the high nibble of data3 and the variant
 * in the top bits of data4[0]. Nothing here reads either - this layer stores
 * and compares identifiers, it does not generate, parse or classify them, so it
 * does not care which version a caller chose.
 * -------------------------------------------------------------------------- */
typedef struct PiGuid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} PiGuid;

/* Macro to define a GUID inline; the parameter names follow the RFC 4122 field
 * names (time_low, time_mid, time_hi_and_version, then the 8 trailing bytes). */
#define PI_GUID(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)                 \
    {                                                                      \
        (uint32_t)(l), (uint16_t)(w1), (uint16_t)(w2),                     \
        {                                                                  \
            (uint8_t)(b1), (uint8_t)(b2), (uint8_t)(b3), (uint8_t)(b4),    \
                (uint8_t)(b5), (uint8_t)(b6), (uint8_t)(b7), (uint8_t)(b8) \
        }                                                                  \
    }

/* Byte-for-byte equality over all 128 bits. NULL-safe (two NULLs are equal,
 * one NULL is not equal to anything). */
static inline int pi_guid_equal(PiGuid const* a, PiGuid const* b)
{
    if (!a || !b)
    {
        return 0;
    }
    return a->data1 == b->data1 && a->data2 == b->data2 && a->data3 == b->data3 && memcmp(a->data4, b->data4, 8) == 0;
}

/* --------------------------------------------------------------------------
 * Opaque native window handle
 *
 * A typedef, not an abstraction: it is whatever the platform calls a window.
 * Anything that *manages* windows belongs to a member library, not here.
 * -------------------------------------------------------------------------- */
#if PI_PLATFORM_WINDOWS
typedef void* PiNativeWindow;
#elif PI_PLATFORM_MACOS
typedef void* PiNativeWindow;
#elif PI_PLATFORM_LINUX
typedef unsigned long PiNativeWindow;
#endif

#define PI_INVALID_WINDOW     ((PiNativeWindow)0)
#define PI_IS_VALID_WINDOW(h) ((h) != PI_INVALID_WINDOW)

/* --------------------------------------------------------------------------
 * IPiUnknown - the family's COM-style root interface
 *
 * Every interface in the family extends this one, which is why it cannot belong
 * to a single member: the first member of *every* vtable is IPiUnknownVtbl, and
 * a member defining its own root would force every other member's interfaces to
 * embed that member's name at the base of their layout.
 *
 * The vtable is a plain C struct of function pointers to guarantee ABI stability
 * across compilers and languages (Rust, C#, Java FFI). Objects that implement it
 * contain:
 *
 *     struct { const IPiUnknownVtbl* lpVtbl; ... };
 *
 * which is the same layout C++ compilers generate for single-inheritance
 * vtables, expressed in C so it is stable everywhere.
 * -------------------------------------------------------------------------- */
typedef struct IPiUnknownVtbl {
    PiResult(PI_CALL* pi_query_interface)(void* this_ptr, PiGuid const* iid, void** out);

    uint32_t(PI_CALL* pi_add_ref)(void* this_ptr);

    uint32_t(PI_CALL* pi_release)(void* this_ptr);
} IPiUnknownVtbl;

/* Complete struct, not a forward declaration: PiRefCountedBase embeds it by
 * value. */
typedef struct IPiUnknown {
    IPiUnknownVtbl const* lpVtbl;
} IPiUnknown;

/* Inline helpers - NULL-safe wrappers. */
static inline PiResult pi_iunknown_query_interface(IPiUnknown* self, PiGuid const* iid, void** out)
{
    if (!self || !self->lpVtbl || !self->lpVtbl->pi_query_interface)
    {
        return PI_E_INVALIDARG;
    }
    return self->lpVtbl->pi_query_interface((void*)self, iid, out);
}

static inline uint32_t pi_iunknown_add_ref(IPiUnknown* self)
{
    if (!self || !self->lpVtbl || !self->lpVtbl->pi_add_ref)
    {
        return 0;
    }
    return self->lpVtbl->pi_add_ref((void*)self);
}

static inline uint32_t pi_iunknown_release(IPiUnknown* self)
{
    if (!self || !self->lpVtbl || !self->lpVtbl->pi_release)
    {
        return 0;
    }
    return self->lpVtbl->pi_release((void*)self);
}

/* The root interface's own identifier. It lives here because IPiUnknown lives
 * here: a member unable to name the root interface would have to define this
 * GUID itself, and two members defining the same GUID independently is exactly
 * the duplication this layer exists to remove. It is the one IID that is family
 * vocabulary; every other identifier a member owns is numbered by that member.
 *
 * A header constant, not an exported data symbol, like everything else here. If
 * a compiler objects to an unused file-scope `static const` in a header, define
 * it once in a .c of your own and declare `extern const` - the value is the
 * contract, not where the storage lives. */
static PiGuid const PI_IID_UNKNOWN = PI_GUID(0x00000000, 0x0000, 0x0000,
                                             0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

/* --------------------------------------------------------------------------
 * Reference-counted base implementation
 *
 * Embed PiRefCountedBase as the FIRST member of your object struct, assign the
 * vtable, and call pi_refcounted_init() (or the _with_destroy variant) from
 * your constructor. The vtable's pi_add_ref / pi_release slots must point at
 * pi_refcounted_add_ref / pi_refcounted_release.
 *
 * Lifetime: when the count reaches zero the optional `destroy` callback runs
 * with the object pointer. For plain C objects allocated with a host allocator,
 * point it at a function that frees the object; for C++ classes, at a static
 * thunk calling `delete (MyClass*)ptr;` so destructors run. NULL destroy leaves
 * the object alone (static / singleton-style objects).
 *
 *     typedef struct { PiRefCountedBase base; ... } MyThing;
 *
 * The count is manipulated with the platform's interlocked intrinsic rather than
 * a lock, so a single object may be shared across threads. The intrinsic is used
 * directly instead of through <windows.h>, because this is a public header and
 * windows.h would drag its macro namespace into every consumer.
 * -------------------------------------------------------------------------- */
typedef void (*PiDestroyProc)(void* self);

typedef struct PiRefCountedBase {
    IPiUnknown unk;
    uint32_t volatile ref_count;
    PiDestroyProc destroy; /* called (once) when refcount hits 0 */
} PiRefCountedBase;

/* Initialize the ref-counted base with refcount 1 and no destroy callback. */
static inline void pi_refcounted_init(PiRefCountedBase* base, IPiUnknownVtbl const* vtbl)
{
    if (!base)
    {
        return;
    }
    base->unk.lpVtbl = vtbl;
    base->ref_count  = 1;
    base->destroy    = NULL;
}

/* Same, with a destroy callback for when the refcount reaches zero. */
static inline void pi_refcounted_init_with_destroy(PiRefCountedBase*     base,
                                                   IPiUnknownVtbl const* vtbl,
                                                   PiDestroyProc         destroy)
{
    if (!base)
    {
        return;
    }
    base->unk.lpVtbl = vtbl;
    base->ref_count  = 1;
    base->destroy    = destroy;
}

/* Default pi_add_ref / pi_release implementations for PiRefCountedBase. */
static inline uint32_t PI_CALL pi_refcounted_add_ref(void* this_ptr)
{
    PiRefCountedBase* base = (PiRefCountedBase*)this_ptr;
    if (!base)
    {
        return 0;
    }
#if PI_PLATFORM_WINDOWS
    return (uint32_t)_InterlockedIncrement((volatile long*)&base->ref_count);
#else
    return (uint32_t)__sync_add_and_fetch((volatile uint32_t*)&base->ref_count, 1);
#endif
}

static inline uint32_t PI_CALL pi_refcounted_release(void* this_ptr)
{
    PiRefCountedBase* base = (PiRefCountedBase*)this_ptr;
    uint32_t          ref;
    if (!base)
    {
        return 0;
    }
#if PI_PLATFORM_WINDOWS
    ref = (uint32_t)_InterlockedDecrement((volatile long*)&base->ref_count);
#else
    ref = (uint32_t)__sync_sub_and_fetch((volatile uint32_t*)&base->ref_count, 1);
#endif
    if (ref == 0)
    {
        /* Invoke the owner's destroy callback instead of free(): objects may be
         * allocated with any allocator (C++ new, host allocator, static
         * storage). destroy may be NULL. */
        PiDestroyProc destroy = base->destroy;
        if (destroy)
        {
            destroy(this_ptr);
        }
    }
    return ref;
}

#ifdef __cplusplus
}
#endif

#endif /* PIBASE_PI_BASE_H */
