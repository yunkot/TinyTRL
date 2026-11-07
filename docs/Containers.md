# Containers

TinyTRL provides three container types, all defined in `include/TinyTRL_Containers.h`:

* **`Array`** — a general-purpose, dynamic resizeable array.
* **`FlatMap`** — an associative key/value container backed by a sorted array.
* **`FlatSet`** — a set of unique values backed by a sorted array.

All three are exception-free, use a *pollute bit* error model (see [Error Handling](Error_Handling.md)), and are templated over a custom allocator.

---

## 1. `Array<Element, Alloc = Allocator>`

A dynamic, contiguous, random-access sequence. The base type is `Containers`, which supplies the shared `Length`, `Size`, `Location`, `Pair`, and capacity constants.

### Storage & error state

* Backed by a single heap block (`Element* _data`) sized to `capacity()`.
* The `_capacity` word packs the real capacity in the low bits (`LengthMask`) and an error flag in the top bit (`PolluteBit`, see TinyTRL_Containers.h).
* `operator bool` returns `!(_capacity & PolluteBit)` — i.e. `true` means *not* polluted.
* Capacity is at most `MaxLength` (`INT64_MAX` on 64-bit, `INT32_MAX` on 32-bit; see `__PLATFORM_X64` in TinyTRL_TypeDef.h).

### Element access (value vs reference)

* `operator [] (Length)` returns **`Element&`** (non-const) / `Element const&` (const) — direct, unchecked, by **integer index**. No bounds check.
* `first()` / `last()` return **`Element&`** to the first/last element.
* `data()`, `begin()`, `end()` expose the raw pointer to enable ranged-for.

So `Array` is the only one of the three whose indexed access yields a **mutable reference**: you can write `array[3] = x` directly. `FlatMap`/`FlatSet` are different — see below.

### Growth pattern

Capacity grows in exponential-ish steps so that the amortised cost of one `add`/`insert` is O(1). The next capacity is computed by `computeCapacity`, which delegates to `math::computeNextCapacity`:

* Under 16 elements it rounds up to the next power of two above `capacity + 1`, and if that step is less than one third of the target it expands by a further +50 % to avoid thrashing.
* For larger sizes it grows by `capacity += capacity / 2` (i.e. 1.5× per reallocation).

So the reallocation count for N appends is O(log N), and total element moves is O(N).

### Mutation & invalidation

* `add(element)` / `add(Element&&)` append and return the new index, or `NotFound` (-1) on overflow/allocation failure.
* `addp(...)` is like `add` but pollutes the array instead of returning `NotFound` on failure.
* `insert(index, element)` inserts at a saturating index (`math::saturate(index, 0, length)`), shifting existing tail elements — this **invalidates any previously obtained references/pointers** to elements at or after `index`; it *may* also reallocate, invalidating **all** stored references.
* `insertp(index, element)` is like `insert` but pollutes the array (instead of returning `false`) on overflow/allocation failure.
* `length(count, value = Element())` resizes to `count` elements — copy-constructing the new tail with `value` when growing, destroying the excess when shrinking; returns `false` on allocation failure.
* `populate(count, value)` appends `count` copies of `value` in one call (bulk append).
* `swap(first, second)` swaps two elements by index with no bounds checks (self-swap is safe).
* `erase(index)` / `erase(start, count)` remove by shifting the tail left, so every element whose index is `>=` the erased range moves to a lower index — all references to those elements are invalidated.
* `clear()` destroys elements but keeps capacity (memory is *not* released); `purge()` releases; `shrink()` shrinks capacity to match `length()` (may reallocate).
* `quickSort(...)` sorts a range with in-place QuickSort using a pivot chosen from the middle and the `utility::swap` placement-new swap.
* `binarySearch(...)` (two overloads) requires the range to already be sorted; the first overload compares `element` against array contents, the second uses a unary comparison against contents.

### Performance characteristics

* Append (`add`) is O(1) amortised (O(log N) reallocations for N appends in total).
* `insert` / `erase` / `clear` are O(N) in the number of shifted/destroyed elements.
* `operator []`, `data()`, `begin/end`, `first/last`, `length()`, `empty()`, `pollute/unpollute` are O(1) and unchecked (except where stated).
* `quickSort` is O(N log N) expected; `binarySearch` is O(log N) comparisons.
* No per-element padding overhead: the container is 1 data pointer + 2 size words (capacity — with the pollute bit packed in its top bit — and length) + an allocator (the default `Allocator` is an empty struct), so the layout is comparable to `std::vector`.

### Example (from `examples/src/Arrays.cpp` and README)

```cpp
Array<int32_t> values = {10, 20, 30};
values.addp(5);                // may reallocate/shift
if (!values) { /* OOM or overflow */ }
for (int32_t v : values) printf("%d\n", v);
```

---

## 2. `FlatMap<Key, Value, Comparer = DefaultComparer<Key>, Alloc = Allocator>`

An ordered associative container. Internally it is an `Array<Pair<Key,Value>>` kept **sorted by key** (TinyTRL_Containers.h). All storage, error, and growth semantics are those of `Array`.

### Key invariants

* Elements are always kept in ascending key order via the user-supplied `Comparer`.
* The default comparer (`DefaultComparer<T>`) implements a three-way comparison with `DefaultCompare::perform`:

```cpp
return left < right ? -1 : (right < left ? 1 : 0);
```

  so it requires `Key` to be `operator<`-comparable and is only total-order if `Key` is. Custom `Comparer`s must return a three-way result (`-1`/`0`/`+1`) and impose a strict weak order over the keys for the binary-search-based API to remain correct.

* Each key is unique in the container; `add(key, value)` for an existing key **overwrites** the value in place (no new element is appended).

### Location semantics (value vs reference)

`FlatMap` exposes only **`Location`-based, const-indexed access** to stored pairs; it has no mutable `operator []` and no `at()` returning a `Value&` by *key*.

* `operator [] (Location const&) const` → `KeyValue const&` (TinyTRL_Containers.h). This is a const, **read-only** reference; mutation must go through the `add`/`insert` API.
* `at(Location)` → `Value const&` / `Value&` (TinyTRL_Containers.h). This is the one way to obtain a **mutable** reference to a stored value — but only via a *Location* you obtained in the *current, unmutated* state of the map (a recent `find`).
* `value(key)` → `Value const*` / `Value*` (TinyTRL_Containers.h). Convenient lookup-by-key that returns a pointer; `NULL` if the key is absent.
* `find(key)` → `Location` (or the invalid `Location(NotFound)`). `find(location, key)` (TinyTRL_Containers.h) returns `false` and sets `location` to the insertion point when the key is absent — this is the idiom the header uses to avoid a double search.

**Invalidation rules** (important in practice):

1. A `Location` is just an integer index packed into a `Containers::Length`. It is **not a pointer** — it is *stable across mutations* only as long as the index of the element you care about does not change.
2. `insert(Location, key, value)` (TinyTRL_Containers.h) is only guaranteed to be valid when the `Location` came from a *recent* `find`. Inserting at any other index may break the sorted invariant and is undefined behaviour.
3. `erase(key)` / `erase(Location)` shifts the tail. Every `Location` whose index is `>=` the erased index refers to a *different* element afterwards. All previously held `Locations` (and `Value&`/pointers from `at` / `value`) that were at or after the erased index are invalidated.
4. `add(key, value)` for a **new** key inserts at the `find`-reported position, so any `Location` that was at or after that position is invalidated. For an **existing** key only the value is overwritten; the index is stable.
5. Reallocation (`add`, `insert`, `capacity(n)` growth, `shrink()`, copy/move assignment) invalidates *every* stored `Value&` / pointer, even though the `Location` indices themselves remain numerically correct.

The `Location` class (TinyTRL_Containers.h) also:
* is **explicit** convertible to `bool` (`operator bool` is invalid iff `_index == NotFound`, `-1`);
* supports `==`, `!=`, `<`, `>` by index, so you can keep a set of locations and compare them;
* carries only the index — it has no back-pointer to the container, so **a `Location` is not safe to use after the owning container has been destroyed, moved from, or had its internal array re-allocated**. Treat it like a `std::vector::size_type` + a "is this still valid?" flag, but *without* automatic lifetime tying.

### Add-or-update semantics

`add(...)` overwrites the value if the key already exists; `insert(Location, ...)` performs a true insert at the supplied (recent-`find`) position and does **not** do a second search. `addp(...)` variants pollute the map on allocation failure instead of returning an invalid `Location`.

### Performance characteristics

* `exists`, `find`, `value(key)`: **O(log N)** binary search, no per-lookup node allocation.
* `add(key, value)` for an existing key: O(log N) for the search + O(1) overwrite.
* `add(key, value)` for a new key: O(log N) search + O(N) shift (insertion) + possible O(N) reallocation (amortised O(1)).
* `erase(key)`: O(log N) search + O(N) shift.
* `at(Location)`, `operator [](Location)`: O(1), unchecked.
* Storage is a single contiguous `Pair<Key,Value>` block — cache-friendly iteration in key order, no per-node pointer chasing (the primary motivation for the "Flat" prefix).

### Example (from `examples/src/FlatMapsAndSets.cpp`)

```cpp
using UserPassword = Containers::Pair<String, String>;
FlatMap<String, String, utility::TextComparer> passwords = {
  UserPassword{"Dan", "user2000"},
  UserPassword{"Leo1", "leo2024"}};

passwords.addp("henry", "rockplayer54");

// find-or-insert idiom: one search instead of two
Containers::Location location;
if (!passwords.find(location, "Jude10"))
  passwords.insert(location, "Jude10", "jude_pass");

// read-only iteration
for (UserPassword const& up : passwords)
  printf("%s\n", up.key.data());
```

---

## 3. `FlatSet<Value, Comparer = DefaultComparer<Value>, Alloc = Allocator>`

A set of unique values, kept sorted in a single `Array<Value>` (TinyTRL_Containers.h). Same storage, error, and growth model as `Array`; same `Location` and invalidation contract as `FlatMap` (because the implementation delegates to the same `Array` primitives).

### Location semantics

* `operator [] (Location const&) const` → `Value const&` (TinyTRL_Containers.h). **Const** only; there is no mutable indexed accessor and no `at(Location)`.
* `value` is the element (no key/value distinction); "keys" are the values themselves.
* `find(value)` → `Location`; `find(Location&, value)` (TinyTRL_Containers.h) returns `false` and sets the location to the insertion point if the value is absent. There is also a generic-template overload `find(Location&, CustomValue, CustomCompare)` (TinyTRL_Containers.h) that lets you search a `FlatSet` with a *different* value type and comparer (useful for prefix or substring queries over a set of strings, for example) — this is a FlatSet-only feature; `FlatMap` has no equivalent.
* `add`, `update`, `insert`, `erase`, `addp`, `pollute/unpollute` follow the same patterns and invalidation rules as `FlatMap` (mutation shifts the tail; reallocation invalidates pointers, `Location` indices remain numerically valid).

`update(value)` is a convenience that adds the value if absent or **overwrites it in place** if present.

### Performance characteristics

Identical in form to `FlatMap`:

* `exists`, `find`: **O(log N)** binary search.
* `add`/`update` (new value): O(log N) + O(N) shift + possible reallocation.
* `add`/`update` (existing value): O(log N) + O(1) overwrite.
* `erase(value)`: O(log N) + O(N) shift.
* Iteration is contiguous and in value order.

---

## 4. `Containers::Location` — value vs reference, and invalidation

A `Location` is an **opaque integer handle** (a `Containers::Length`), not a pointer. It is the only reference-like type the flat containers expose to the outside. The practical consequences:

### Value vs reference

* **Reference to the stored element**: obtained via the *container*, not the *location*. In `FlatMap` the only mutable-reference accessor is `at(Location) -> Value&`; in `Array` it is `operator [](Length) -> Element&`. The `Location` itself is a value (copyable, comparable, assignable).
* **Value (copy) of the element**: you get it through `operator [](Location) const` (FlatMap/FlatSet) or `operator [](Length)` (Array), which returns `Element const&` and you can copy from it.
* There is **no `std::pair`-style `operator [](key)`** that auto-default-constructs a value, and no *mutable* `operator []` on the flat maps/sets; mutation is via `add`, `insert`, `update`, or `at(Location)`.

### Invalidation

Because `Location` does not own a pointer, it does not *itself* dangle — but the *element it refers to* may move or be destroyed in three ways:

| Event | What happens to `Location`s |
|---|---|
| Insert / `add` (new key/value) at index `i` | All `Location`s with index `>= i` now point at a *different* element; indices `< i` are unaffected. |
| `erase` at index `i` | All `Location`s with index `>= i` now point at a *different* element; erased element is gone. |
| Re-allocation (`capacity`, `shrink`, `clear`, copy/move assignment, move constructor) | All `Location`s *indices are still numerically valid* (they are just ints), but any `Value&`/pointer handed out by `at()`/`value()` is gone. |
| `pollute()` / failed allocation | The container is marked invalid; subsequent lookups return "not found" but `Location` handles that *were already* produced remain numerically usable until the next mutation. |

### Lifetime rules of thumb

1. Hold a `Location` only across a single logical "use" (e.g. `find → insert` pair). Do **not** keep `Locations` in a long-lived structure that outlives the surrounding mutation window.
2. If you must keep a handle across mutations, store it as an index *and* re-validate by re-searching (compare `at(loc).key` with your expected key, or `at(loc)` for a flat set) before using it.
3. Treat `at(Location)` / `value(key)`-returned pointers as **stable only until the next container mutation or reallocation** — do not persist them past an `add`, `insert`, `erase`, or a `capacity()` change.

---

## 5. Sorted-key behaviour and comparer contract

* `FlatMap`/`FlatSet` keep the underlying array in ascending order of the *key* (in `Set`, key == value) using the supplied `Comparer`. The binary-search primitives (`search` in both containers) assume a **three-way** comparator returning `-1`/`0`/`+1` under a strict weak ordering; a non-total ordering (e.g. equal-but-not-identical keys) will cause `add` to overwrite the first equal element it encounters, which is the documented behaviour of `add`'s "update" path.
* `DefaultCompare::perform` is a thin wrapper over `operator<`; it is the right default for `int`, `String` with its lexicographic ordering, and similar value types. For case-insensitive lookups the `utility::TextComparer` (as used in the `FlatMapsAndSets` example) supplies the three-way result.
* Custom comparers are copied by value into `FlatMap`/`FlatSet` (the `Comparer` member is copied in the copy/move constructors and assignment operators — they are `= default`-ed in the header). The `Alloc` member participates in copy/move semantics like any other member — it is copied on copy and moved on move, and it is the allocator used to deallocate the container's storage.
* Because `FlatMap`/`FlatSet` are sorted by *key*, two elements that compare equal under the `Comparer` will never coexist in the same container instance. If you need multi-valued or multi-key behaviour, maintain a `FlatMap<Key, FlatSet<Value>>` or use an `Array<Pair>` and sort it with `quickSort` instead.

---

## 6. Performance at a glance (summary)

| Operation | `Array` | `FlatMap` | `FlatSet` |
|---|---|---|---|
| Build/append (amortised) | O(1) | O(log N) search + O(N) shift | O(log N) search + O(N) shift |
| Random read by index | O(1) | O(1) via `Location` | O(1) via `Location` |
| Lookup by key/value | — | O(log N) | O(log N) |
| Update in place | O(1) (by index) | O(1) after `find` (`add` existing key) | O(1) after `find` (`update`) |
| Insert (new) | O(N) shift / O(1) amortised append | O(log N) + O(N) shift | O(log N) + O(N) shift |
| Erase | O(N) shift | O(log N) + O(N) shift | O(log N) + O(N) shift |
| Sort (explicit) | O(N log N) QuickSort | n/a (auto-maintained) | n/a (auto-maintained) |
| Binary search | O(log N) | O(log N) | O(log N) |
| Storage overhead | 1 data ptr + 1 cap word (with pollute bit) + 1 len + 1 allocator (≈ 3 words on both 32-bit and 64-bit; the default `Allocator` is an empty struct) | + 1 comparer (sizeof `Comparer`) | + 1 comparer |
| Cache behaviour | contiguous, value-major | contiguous, key-sorted | contiguous, value-sorted |

Notes:

* "O(N) shift" is a **move** of the elements (placement-new + destructor), not a shallow `memcpy` — see `elementInsert` and `erase`. For trivially copyable `Element`s this is still very fast because the elements are packed tightly.
* Reallocation frequency is O(log N) for N appends (amortised 1.5× growth for large arrays), so the *expected* cost of N appends is O(N), matching `std::vector`'s.
* The flat layout is the library's main competitive advantage over a node-based tree: pointer-chasing is eliminated, and iteration is cache-line-friendly.

