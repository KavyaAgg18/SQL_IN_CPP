#include "value.h"
#include <cstdio>
#include <stdexcept>
#include <string>

Value parseValue(const std::string &raw, int datatypeId)
{
    // "NULL" sentinel passes through as string in all cases.
    if (raw == "NULL")
        return std::string("NULL");

    switch (datatypeId)
    {
    case 0: // INT
        try { return std::stoi(raw); }
        catch (...) { return raw; } // malformed: keep as string rather than crash
    case 1: // FLOAT — stored as std::string, not as float, to preserve the exact
        // on-disk text ("49.50" round-trips; float->string loses trailing zeros).
        // The float alternative in Value is reserved for future arithmetic use.
        // Validity was already checked at insert() time, so we trust the raw string here.
        return raw;
    case 2: // BOOL / BOOLEAN
        // Accept all four forms that insert() validation allows.
        if (raw == "TRUE"  || raw == "1") return true;
        if (raw == "FALSE" || raw == "0") return false;
        return raw; // malformed bool: fall back to string
    case 3: // STRING
    case 4: // DATE — string pass-through; a real Date type is future work
    default:
        return raw;
    }
}

std::string valueToString(const Value &v)
{
    return std::visit([](const auto &val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>)
        {
            return std::to_string(val);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            // %g: compact form, strips trailing zeros (3.14f->"3.14", 3.0f->"3").
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(val));
            return std::string(buf);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return val ? "TRUE" : "FALSE";
        }
        else // std::string — covers STRING, DATE, "NULL", and malformed fall-backs
        {
            return val;
        }
    }, v);
}

bool valuesEqual(const Value &a, const Value &b)
{
    // std::variant operator== returns true iff both hold the same alternative
    // and the contained values compare equal — exactly the same-type-only
    // semantics we want for Phase 3 hash joins.
    return a == b;
}
