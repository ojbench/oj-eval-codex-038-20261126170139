# Vector implementation

The live ACMOJ API identifies 2609 as a C++ header submission, with two
50-point performance subtasks (Move Test and Realloc Test). Its specification
links to the official STLite-ACM-2026 vector interface. Submit `vector.hpp`;
the judge provides `exceptions.hpp` and its test driver. The generic executable
submission instructions in README.md do not describe this header-based task.

Storage is contiguous and doubles when full. Push is amortized O(1), indexed
access is O(1), and insertion/erasure is O(n). Objects are constructed only in
occupied slots. Trivial elements use bulk memory operations; other elements
use move construction and assignment. Pop and clear retain capacity, avoiding
repeated allocations near a capacity boundary. Copying owns independent storage,
while moving a vector transfers its buffer in O(1). Insertion and push preserve
aliased arguments before relocating storage.

The supplied exception definitions are reproduced unchanged from the official
course repository. No standard container is used in the implementation.

Validation before submission:

- All seven public tests from ACMClassCourse-2025/STLite-ACM-2026 passed.
- 30,000 randomized operations compared against std::vector passed.
- Aliased insertion, aliased growth, copy/move ownership, iterator checking, and
  object lifetime tests passed with address and undefined-behavior sanitizers.

Build the local checks with `cmake -S . -B build`, `cmake --build build`, and
`ctest --test-dir build --output-on-failure`.

The submission client was repaired by implementing its missing `submit_code`
method. Submission IDs are also logged by the client outside the repository.
