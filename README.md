# pibase

**The PI family base layer.** One dependency-free C header holding the
vocabulary that every PI library shares — and deliberately nothing else.

```c
#include <pibase/pi_base.h>
```

| What | Names |
|---|---|
| Platform / ABI plumbing | `PI_PLATFORM_*`, `PI_EXPORT`, `PI_IMPORT`, `PI_LOCAL`, `PI_CALL` |
| The result-code list | `PiResult`, `PI_OK` / `PI_FAIL` / `PI_E_*` / `PI_SUCCEEDED` / `PI_FAILED` |
| The 128-bit identifier | `PiGuid`, `PI_GUID()`, `pi_guid_equal()` |
| The native window handle | `PiNativeWindow`, `PI_INVALID_WINDOW`, `PI_IS_VALID_WINDOW` |
| The COM-style root | `IPiUnknownVtbl`, `IPiUnknown`, `PI_IID_UNKNOWN`, `PiRefCountedBase`, `pi_refcounted_*()` |

## The naming rule

> The rule's authoritative statement is [`docs/naming.md`](docs/naming.md); what
> follows is the summary, and the same rule is restated next to the code in
> `include/pibase/pi_base.h`.

PI is a family of libraries. Each member has its own name, its own headers and
its own release cycle; the names above belong to none of them, because a result
code, a GUID, a window handle and an interface root mean the same thing in every
member and every member needs them in the signatures of its own API.

So names are scoped by prefix, and this header is where that is stated:

```
family root vocabulary    bare pi_ / Pi / PI_         <- this library
a member's own names      pi_<member>_ / Pi<Member> / PI_<MEMBER>_
```

`PiResult` is family-level and stays bare. A member library prefixes everything
it adds itself. A reader can tell from the identifier alone whether a name is
shared across the family or owned by one member — which is the entire point of
having this layer rather than letting each library invent its own `PiResult`.

Conventions within each tier follow the ecosystem norm: **snake_case** for C
functions, **PascalCase** for types, **SCREAMING_SNAKE_CASE** for macros. (Bare
PascalCase C *functions* are a platform-SDK habit, not a library one.)

## What belongs here

The test is one question:

> Does this name appear in the signatures of **more than one** member's API?

If it does not, it belongs to a member. Concretely, a name is admitted only if:

1. it appears in a cross-library API signature;
2. its implementation, if any, is a **pure function** — no state, no allocation,
   no I/O, no threads;
3. its correct implementation is **obvious** and involves no policy choice;
4. leaving it out would force every member to define the same thing
   independently at the ABI boundary.

Everything else is someone else's business. Containers, strings, allocators,
threads, I/O and parsers all fail test 1: they are features a library may offer,
not vocabulary the family shares. So does anything needing a policy — a UUID
*generator* fails test 3 (which version? which randomness source?), even though
`PiGuid` itself obviously passes.

**A quantified red line:** platform-specific code in this library must stay at
the level of a typedef or a compiler intrinsic. Today that is the two
interlocked operations behind the reference count. The day this header needs a
real platform abstraction is the day it has grown past its job.

## Header-only, on purpose

Everything here is a type, a macro, or a `static inline` function. Depending on
pibase therefore adds **no binary, no link step and no binary ABI that could
ever need freezing**. This is the one layer that must never break, so it is the
one layer with no ABI to break: the contract is the header, and a consumer
recompiles to adopt it.

That also makes it the natural boundary for non-C consumers. The header is valid
C *and* valid C++ (both are tested), and a Rust or C# binding can consume it with
no library to link. Language-specific sugar, when it is wanted, belongs in a
separate opt-in wrapper — never as a reason to make this header depend on
anything.

## Using it

CMake:

```cmake
find_package(pibase REQUIRED)
target_link_libraries(my_target PRIVATE pibase::pibase)   # headers + features only
```

Conan:

```ini
[requires]
pibase/0.1.0
```

## Building and testing

```bash
cmake --preset default          # Windows (Visual Studio); default-unix elsewhere
cmake --build --preset default
ctest --preset default
```

The self-test asserts the two things consumers silently rely on: the **binary
layout** (a GUID is exactly 16 bytes, `PiResult` is 32-bit, `IPiUnknown` sits
first in `PiRefCountedBase`) and the **semantics** of the shared vocabulary
(GUID comparison including its NULL rule, the sign rule behind
`PI_SUCCEEDED`/`PI_FAILED`, reference counting down to the destroy callback,
NULL tolerance on every helper). It runs as both C and C++.

## Versioning

`0.x.y` while the family is still settling. A `y` release only fixes; an `x`
release may add or change names, and the changelog says which. There is no
binary compatibility question to answer — the version is the header's contract
version, and consumers recompile.

## License

MIT — see [LICENSE](LICENSE).
