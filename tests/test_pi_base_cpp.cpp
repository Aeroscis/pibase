/*
 * C++ includability check.
 *
 * pibase is a C header, and the family's members and consumers are not all C:
 * the point of a C ABI is that C++ (and Rust, and C#) can use it. So the header
 * must compile as C++ with no warnings and no surprises - which is a property
 * that silently rots, hence a test.
 *
 * The static_asserts restate the layout contract at compile time, in the
 * language most consumers actually write.
 */
#include "pibase/pi_base.h"

#include <cstddef>
#include <cstdint>

static_assert(sizeof(PiGuid) == 16, "PiGuid is the frozen 16-byte layout");
static_assert(sizeof(PiResult) == 4, "PiResult is a 32-bit status");
static_assert(offsetof(PiGuid, data4) == 8, "PiGuid field offsets are frozen");
static_assert(offsetof(PiRefCountedBase, unk) == 0,
              "PiRefCountedBase embeds IPiUnknown first, by contract");
static_assert(PI_SUCCEEDED(PI_OK), "PI_OK is a success");
static_assert(!PI_SUCCEEDED(PI_FAIL), "PI_FAIL is not a success");
static_assert(PI_APP_RESULT_BASE <= -100, "the app partition starts at -100");

/* A C++ object can be built on the same refcounted base the C side uses. */
namespace {

class Thing {
public:
    Thing() { pi_refcounted_init_with_destroy(&base_, &vtbl_, &DestroyThunk); }
    ~Thing() = default;

    IPiUnknown* unknown() { return &base_.unk; }

    static int destroyed_count;

    /* Public because the test compares the object's vtable pointer against it. */
    static const IPiUnknownVtbl vtbl_;

private:
    /* NOTE: PiDestroyProc is a plain cdecl `void (*)(void*)` - it has no
     * PI_CALL, so this thunk must not have one either. */
    static void DestroyThunk(void* self) { (void)self; ++destroyed_count; }

    static PiResult PI_CALL QueryInterface(void*, const PiGuid*, void**) { return PI_E_NOINTERFACE; }
    static uint32_t PI_CALL AddRef(void* self) { return pi_refcounted_add_ref(self); }
    static uint32_t PI_CALL Release(void* self) { return pi_refcounted_release(self); }

    PiRefCountedBase base_;
};

int Thing::destroyed_count = 0;

const IPiUnknownVtbl Thing::vtbl_ = { &Thing::QueryInterface, &Thing::AddRef, &Thing::Release };

}  // namespace

int main()
{
    int failures = 0;

    Thing thing;
    if (thing.unknown()->lpVtbl != &Thing::vtbl_) ++failures;   /* C++ can reach it */
    if (pi_iunknown_add_ref(thing.unknown()) != 2) ++failures;
    if (pi_iunknown_release(thing.unknown()) != 1) ++failures;
    if (pi_iunknown_release(thing.unknown()) != 0) ++failures;
    if (Thing::destroyed_count != 1) ++failures;

    /* The one family-root identifier is reachable from C++ too. */
    if (!pi_guid_equal(&PI_IID_UNKNOWN, &PI_IID_UNKNOWN)) ++failures;

    return failures == 0 ? 0 : 1;
}
