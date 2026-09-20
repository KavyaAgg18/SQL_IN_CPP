#ifndef VALUE_H
#define VALUE_H

#include <variant>
#include <string>
#include <vector>

// A single cell's value, typed according to the column's declared datatype.
// INT->int, FLOAT->float, BOOL->bool, STRING/DATE->string.
// DATE stays a string — a real Date type is future work, not part of this pass.
using Value = std::variant<int, float, bool, std::string>;

// One in-memory row: cells in schema-index order (matching Table::columns sorted order).
struct Row {
    std::vector<Value> cells;
};

// Converts a raw CSV field into a typed Value using the column's datatype ID
// (globals.h: 0=INT, 1=FLOAT, 2=BOOL/BOOLEAN, 3=STRING, 4=DATE).
// "NULL" is always passed through as std::string("NULL") — no separate null type yet.
// Malformed values (e.g. "abc" for an INT column) also fall back to std::string.
Value parseValue(const std::string &raw, int datatypeId);

// Converts a typed Value back to its string form for display/CSV output.
// bool decision: true->"TRUE", false->"FALSE". Inputs "1"/"0" are not
// preserved because bool loses the original text once parsed; this is safe in
// Phase 1 since parseRow is not yet in the live code path (no on-disk writes change).
// float uses %g format (strips trailing zeros: 3.14f->"3.14", 3.0f->"3").
std::string valueToString(const Value &v);

// Equality comparison for joins (used starting Phase 3).
// Compares by same variant alternative only — cross-type numeric equality
// (int 2 vs float 2.0) is intentionally not handled here; same-type matching
// is sufficient for hash-join correctness and avoids silent type coercion bugs.
bool valuesEqual(const Value &a, const Value &b);

// Hasher for Value so it can be used as an unordered_map key (Phase 3 buildIndex).
// Mixes the variant type index into the hash to prevent cross-type collisions
// (e.g. int(1) and bool(true) would otherwise both hash to ~1).
struct ValueHash {
    std::size_t operator()(const Value &v) const {
        std::size_t h = std::visit([](const auto &val) -> std::size_t {
            return std::hash<std::decay_t<decltype(val)>>{}(val);
        }, v);
        // Multiply type index by a large prime before XOR to spread bits.
        return h ^ (v.index() * 2654435761ULL);
    }
};

#endif // VALUE_H
