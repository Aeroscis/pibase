/*
 * pibase self-test
 *
 * pibase has no behaviour to speak of, so what is worth asserting is the two
 * things consumers silently rely on:
 *
 *   1. the binary layout (a GUID is exactly 16 bytes; PiResult is 32-bit) -
 *      this is the layer whose whole promise is "the contract is the header",
 *      so a drifted layout is the one failure mode that matters;
 *   2. the semantics of the shared vocabulary (GUID comparison, the sign rule
 *      behind PI_SUCCEEDED/PI_FAILED, reference counting, NULL tolerance).
 *
 * Exit code decides; the last line is a greppable evidence line.
 */
#include "pibase/pi_base.h"

#include <stdio.h>
#include <stddef.h>

/* CHECK() is deliberately applied to compile-time constants here - the result
 * code sign rule *is* a constant, and asserting it is the point of the test.
 * MSVC calls that "conditional expression is constant"; silence it. */
#if defined(_MSC_VER)
#  pragma warning(disable : 4127)
#endif

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                             \
    do {                                                                        \
        ++g_checks;                                                             \
        if (!(cond)) {                                                          \
            ++g_failures;                                                       \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
        }                                                                       \
    } while (0)

#define CHECK_EQ_INT(a, b)                                                      \
    do {                                                                        \
        ++g_checks;                                                             \
        long long va_ = (long long)(a), vb_ = (long long)(b);                   \
        if (va_ != vb_) {                                                       \
            ++g_failures;                                                       \
            printf("FAIL %s:%d: %s == %s (%lld != %lld)\n",                     \
                   __FILE__, __LINE__, #a, #b, va_, vb_);                       \
        }                                                                       \
    } while (0)

/* How many platform macros are defined: exactly one, always. */
static int defined_platform_count(void)
{
    int n = 0;
#ifdef PI_PLATFORM_WINDOWS
    ++n;
#endif
#ifdef PI_PLATFORM_MACOS
    ++n;
#endif
#ifdef PI_PLATFORM_LINUX
    ++n;
#endif
    return n;
}

/* --------------------------------------------------------------------------
 * A minimal object built the way the header tells consumers to build one.
 * -------------------------------------------------------------------------- */
typedef struct TestThing {
    PiRefCountedBase base;   /* must be first */
    int              magic;
} TestThing;

static int g_destroy_calls = 0;

static PiResult PI_CALL TestThing_QueryInterface(void* self_ptr, const PiGuid* iid, void** out)
{
    TestThing* me = (TestThing*)self_ptr;
    if (!out) return PI_E_INVALIDARG;
    *out = NULL;
    if (iid && pi_guid_equal(iid, &PI_IID_UNKNOWN)) {
        *out = &me->base.unk;
        pi_iunknown_add_ref(&me->base.unk);
        return PI_OK;
    }
    return PI_E_NOINTERFACE;
}

static uint32_t PI_CALL TestThing_AddRef(void* self_ptr)
{
    return pi_refcounted_add_ref(self_ptr);
}

static uint32_t PI_CALL TestThing_Release(void* self_ptr)
{
    return pi_refcounted_release(self_ptr);
}

static void TestThing_Destroy(void* self_ptr)
{
    ++g_destroy_calls;
    (void)self_ptr;
}

static const IPiUnknownVtbl s_test_vtbl = {
    &TestThing_QueryInterface,
    &TestThing_AddRef,
    &TestThing_Release
};

/* An identifier to compare against, in the RFC 4122 field order. */
static const PiGuid s_iid_a = PI_GUID(0x9F3C1D42, 0x7B08, 0x4E55,
                                      0xA1, 0x6C, 0x0D, 0xF2, 0x88, 0x37, 0x51, 0xBE);
static const PiGuid s_iid_a_again = PI_GUID(0x9F3C1D42, 0x7B08, 0x4E55,
                                            0xA1, 0x6C, 0x0D, 0xF2, 0x88, 0x37, 0x51, 0xBE);
static const PiGuid s_iid_b = PI_GUID(0x9F3C1D42, 0x7B08, 0x4E55,
                                      0xA1, 0x6C, 0x0D, 0xF2, 0x88, 0x37, 0x51, 0xBF);

static void TestLayout(void)
{
    /* The frozen part: this header IS the contract, so these numbers are it. */
    CHECK_EQ_INT(sizeof(PiGuid), 16);
    CHECK_EQ_INT(sizeof(PiResult), 4);
    CHECK_EQ_INT(sizeof(uint32_t), 4);
    CHECK_EQ_INT(offsetof(PiGuid, data1), 0);
    CHECK_EQ_INT(offsetof(PiGuid, data2), 4);
    CHECK_EQ_INT(offsetof(PiGuid, data3), 6);
    CHECK_EQ_INT(offsetof(PiGuid, data4), 8);
    /* PiRefCountedBase embeds IPiUnknown first; objects rely on that. */
    CHECK_EQ_INT(offsetof(PiRefCountedBase, unk), 0);
    CHECK_EQ_INT(sizeof(IPiUnknown), sizeof(void*));
    /* Exactly one platform macro is defined. */
    CHECK_EQ_INT(defined_platform_count(), 1);
}

static void TestResultCodes(void)
{
    /* Sign rule: successes are >= 0, errors are < 0. */
    CHECK(PI_SUCCEEDED(PI_OK));
    CHECK(PI_FAILED(PI_FAIL));
    CHECK(PI_FAILED(PI_E_NOINTERFACE));
    CHECK(PI_FAILED(PI_E_INVALIDARG));
    CHECK(PI_FAILED(PI_E_OUTOFMEMORY));
    CHECK(PI_FAILED(PI_E_NOTIMPL));
    CHECK(PI_FAILED(PI_E_UNEXPECTED));
    CHECK(PI_FAILED(PI_E_NOTFOUND));
    CHECK(PI_FAILED(PI_E_MISSINGCAPABILITY));
    CHECK(PI_FAILED(PI_E_VERSIONMISMATCH));
    CHECK(!PI_FAILED(PI_OK));

    /* The partition rule: an app error code is still an error... */
    CHECK(PI_FAILED(PI_APP_RESULT_BASE));
    CHECK(PI_APP_RESULT_BASE <= (PiResult)-100);
    CHECK(PI_FAILED(PI_APP_RESULT_BASE - 1));

    /* ...and an "accepted, result later" state is a success, so a positive
     * code. This asserts the trap the partition rule exists to warn about:
     * had it been negative, PI_FAILED would report the wrong answer. */
    {
        const PiResult accepted = (PiResult)1;
        CHECK(PI_SUCCEEDED(accepted));
        CHECK(!PI_FAILED(accepted));
    }
}

static void TestGuid(void)
{
    CHECK(pi_guid_equal(&s_iid_a, &s_iid_a_again));
    CHECK(!pi_guid_equal(&s_iid_a, &s_iid_b));
    CHECK(pi_guid_equal(&s_iid_a, &s_iid_a));
    /* NULL semantics: two NULLs are NOT equal (preserved from the origin). */
    CHECK(!pi_guid_equal(NULL, NULL));
    CHECK(!pi_guid_equal(&s_iid_a, NULL));
    CHECK(!pi_guid_equal(NULL, &s_iid_a));
    /* A GUID built by the macro is the identifier the family reserves. */
    CHECK(pi_guid_equal(&PI_IID_UNKNOWN, &PI_IID_UNKNOWN));
}

static void TestWindow(void)
{
    PiNativeWindow none = PI_INVALID_WINDOW;
    int dummy = 0;
    PiNativeWindow some = (PiNativeWindow)&dummy;

    CHECK(!PI_IS_VALID_WINDOW(none));
    CHECK(PI_IS_VALID_WINDOW(some));
}

static void TestRefCountAndUnknown(void)
{
    TestThing thing;

    g_destroy_calls = 0;
    thing.magic = 0x5A5A;
    pi_refcounted_init_with_destroy(&thing.base, &s_test_vtbl, &TestThing_Destroy);

    CHECK_EQ_INT(thing.base.ref_count, 1);
    CHECK(thing.base.unk.lpVtbl == &s_test_vtbl);
    CHECK_EQ_INT(thing.magic, 0x5A5A);

    /* add_ref / release through the helpers reach the vtable. */
    CHECK_EQ_INT(pi_iunknown_add_ref(&thing.base.unk), 2);
    CHECK_EQ_INT(pi_iunknown_release(&thing.base.unk), 1);
    CHECK_EQ_INT(g_destroy_calls, 0);          /* still alive at 1 */

    /* The last release runs destroy exactly once. */
    CHECK_EQ_INT(pi_iunknown_release(&thing.base.unk), 0);
    CHECK_EQ_INT(g_destroy_calls, 1);

    /* NULL tolerance everywhere. */
    CHECK_EQ_INT(pi_iunknown_add_ref(NULL), 0);
    CHECK_EQ_INT(pi_iunknown_release(NULL), 0);
    CHECK_EQ_INT(pi_iunknown_query_interface(NULL, &s_iid_a, NULL), PI_E_INVALIDARG);
    CHECK_EQ_INT(pi_refcounted_add_ref(NULL), 0);
    CHECK_EQ_INT(pi_refcounted_release(NULL), 0);

    /* A destroy-less object is left alone at zero. */
    {
        TestThing quiet;
        pi_refcounted_init(&quiet.base, &s_test_vtbl);
        CHECK(quiet.base.destroy == NULL);
        CHECK_EQ_INT(pi_refcounted_release(&quiet.base), 0);
    }
}

static void TestQueryInterface(void)
{
    TestThing thing;
    void* out = NULL;

    pi_refcounted_init(&thing.base, &s_test_vtbl);

    CHECK_EQ_INT(pi_iunknown_query_interface(&thing.base.unk, &PI_IID_UNKNOWN, &out), PI_OK);
    CHECK(out == (void*)&thing.base.unk);
    CHECK_EQ_INT(thing.base.ref_count, 2);      /* QI hands out an add-ref'd pointer */

    out = (void*)&thing;
    CHECK_EQ_INT(pi_iunknown_query_interface(&thing.base.unk, &s_iid_b, &out), PI_E_NOINTERFACE);
    CHECK(out == NULL);                         /* miss forces *out to NULL */

    CHECK_EQ_INT(pi_iunknown_release(&thing.base.unk), 1);
    CHECK_EQ_INT(pi_iunknown_release(&thing.base.unk), 0);
}

int main(void)
{
    TestLayout();
    TestResultCodes();
    TestGuid();
    TestWindow();
    TestRefCountAndUnknown();
    TestQueryInterface();

    printf("pibase: checks=%d failures=%d\n", g_checks, g_failures);
    if (g_failures == 0) printf("ALL CHECKS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
