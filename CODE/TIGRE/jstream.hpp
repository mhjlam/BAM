// jstream.hpp - JSON-based bidirectional save/restore stream.
// Works like SaveStream but targets nlohmann::json instead of FILE*.
// All types that had FILE* sync calls get a named-key equivalent here.
//
// Usage:
//   JsonStream js{jsonNode, true};   // saving
//   JsonStream js{jsonNode, false};  // restoring
//   js.sync(myField, "myField");
//   js.syncEnum(myEnum, "myEnum");
//   js.syncBytes(myArray, sizeof(myArray), "myArray");
//   js.sentinel("optional-tag");     // no-op; retained for call-site compatibility

#ifndef jstream_hpp
#define jstream_hpp

#include "json.hpp"
#include "rect.hpp"
#include "types.hpp"   // int8, uint8, int16, uint16, int32, uint32, ticks_t, grip
#include <cstring>
#include <string>

static inline std::string b64_encode(const uint8_t* d, size_t n) {
    static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    r.reserve((n + 2) / 3 * 4);
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)d[i] << 16;
        if (i+1 < n) v |= (uint32_t)d[i+1] << 8;
        if (i+2 < n) v |= d[i+2];
        r += T[(v>>18)&63]; r += T[(v>>12)&63];
        r += (i+1 < n) ? T[(v>>6)&63] : '=';
        r += (i+2 < n) ? T[v&63]      : '=';
    }
    return r;
}

static inline uint8_t b64val(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a' + 26);
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0' + 52);
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0;
}

static inline void b64_decode(const std::string& s, uint8_t* d, size_t n) {
    size_t out = 0;
    for (size_t i = 0; i + 3 < s.size() && out < n; i += 4) {
        uint32_t v = ((uint32_t)b64val(s[i])   << 18) | ((uint32_t)b64val(s[i+1]) << 12)
                   | ((uint32_t)b64val(s[i+2]) <<  6) |  (uint32_t)b64val(s[i+3]);
        if (out < n)              d[out++] = (uint8_t)((v >> 16) & 0xFF);
        if (out < n && s[i+2] != '=') d[out++] = (uint8_t)((v >>  8) & 0xFF);
        if (out < n && s[i+3] != '=') d[out++] = (uint8_t)(v & 0xFF);
    }
    if (out < n) memset(d + out, 0, n - out);
}

struct JsonStream {
    nlohmann::json& j;
    bool saving;

    // ---- 1-byte ----
    void sync(bool         &v, const char* key) {
        if (saving) j[key] = v;
        else        v = j.value(key, false);
    }
    void sync(int8_t       &v, const char* key) {
        if (saving) j[key] = (int)v;
        else        v = (int8_t)j.value(key, 0);
    }
    void sync(uint8_t      &v, const char* key) {
        if (saving) j[key] = (unsigned)v;
        else        v = (uint8_t)j.value(key, 0u);
    }

    // ---- 2-byte ----
    void sync(int16_t      &v, const char* key) {
        if (saving) j[key] = (int)v;
        else        v = (int16_t)j.value(key, 0);
    }
    void sync(uint16_t     &v, const char* key) {
        if (saving) j[key] = (unsigned)v;
        else        v = (uint16_t)j.value(key, 0u);
    }

    // ---- 4-byte (int, unsigned int — covers int32, uint32, coord, ticks_t, uint) ----
    void sync(int          &v, const char* key) {
        if (saving) j[key] = v;
        else        v = j.value(key, 0);
    }
    void sync(unsigned int &v, const char* key) {
        if (saving) j[key] = v;
        else        v = j.value(key, 0u);
    }

    // ---- 8-byte ----
    void sync(int64_t      &v, const char* key) {
        if (saving) j[key] = v;
        else        v = j.value(key, (int64_t)0);
    }
    void sync(uint64_t     &v, const char* key) {
        if (saving) j[key] = v;
        else        v = j.value(key, (uint64_t)0);
    }

    // ---- enum (always stored as int32 regardless of underlying type size) ----
    template<typename E>
    void syncEnum(E &e, const char* key) {
        if (saving) j[key] = (int32_t)(int)e;
        else        e = (E)(int32_t)j.value(key, (int32_t)0);
    }

    // ---- grip: only slot index is persisted; generation is runtime-only ----
    void syncGrip(grip &g, const char* key) {
        if (saving) j[key] = (uint32_t)g.index;
        else        g = grip{(uint16_t)j.value(key, 0u), 0};
    }

    // ---- fixed-length char array (stored as JSON string, NUL-terminated) ----
    void syncStr(char* s, size_t maxLen, const char* key) {
        if (saving) {
            j[key] = std::string(s, strnlen(s, maxLen));
        } else {
            std::string str = j.value(key, std::string{});
            size_t n = str.size() < maxLen - 1 ? str.size() : maxLen - 1;
            memcpy(s, str.c_str(), n);
            s[n] = '\0';
        }
    }

    // ---- raw bytes (stored as base64 string) ----
    // Works for POD structs, enums, and byte arrays.
    void syncBytes(void* p, size_t n, const char* key) {
        auto* bytes = static_cast<uint8_t*>(p);
        if (saving) {
            j[key] = b64_encode(bytes, n);
        } else {
            if (!j.contains(key)) { memset(bytes, 0, n); return; }
            b64_decode(j[key].get<std::string>(), bytes, n);
        }
    }

    // ---- Rectangle: saves only the four coord fields by name, avoiding the
    //      vtable pointer that syncBytes would otherwise capture and corrupt. ----
    void syncRect(Rectangle& r, const char* key) {
        auto& node = j[key];
        if (saving) {
            node["x1"] = r.x1;  node["y1"] = r.y1;
            node["x2"] = r.x2;  node["y2"] = r.y2;
        } else {
            r.Set(node.value("x1", r.x1), node.value("y1", r.y1),
                  node.value("x2", r.x2), node.value("y2", r.y2));
        }
    }

    // ---- typed arrays (1-D and 2-D): stored as JSON arrays of numbers ----
    // Works for int[], bool[], enum[], uint32[], int8[], etc.
    // 2-D arrays are stored as arrays-of-arrays.
    template<typename T, size_t N>
    void syncArray(T (&arr)[N], const char* key) {
        if (saving) {
            auto a = nlohmann::json::array();
            for (size_t i = 0; i < N; i++) a.push_back((int64_t)arr[i]);
            j[key] = std::move(a);
        } else {
            if (!j.contains(key)) { memset(arr, 0, sizeof(arr)); return; }
            auto& a = j[key];
            size_t n = a.size() < N ? a.size() : N;
            if (n < N) memset(arr, 0, sizeof(arr));
            for (size_t i = 0; i < n; i++) arr[i] = (T)a[i].get<int64_t>();
        }
    }

    template<typename T, size_t M, size_t N>
    void syncArray(T (&arr)[M][N], const char* key) {
        if (saving) {
            auto outer = nlohmann::json::array();
            for (size_t i = 0; i < M; i++) {
                auto inner = nlohmann::json::array();
                for (size_t k = 0; k < N; k++) inner.push_back((int64_t)arr[i][k]);
                outer.push_back(std::move(inner));
            }
            j[key] = std::move(outer);
        } else {
            if (!j.contains(key)) { memset(arr, 0, sizeof(arr)); return; }
            auto& outer = j[key];
            size_t m = outer.size() < M ? outer.size() : M;
            if (m < M) memset(arr, 0, sizeof(arr));
            for (size_t i = 0; i < m; i++) {
                auto& inner = outer[i];
                size_t n = inner.size() < N ? inner.size() : N;
                for (size_t k = 0; k < n; k++) arr[i][k] = (T)inner[k].get<int64_t>();
            }
        }
    }

    // ---- sentinel: no-op in JSON (retained for call-site compatibility) ----
    void sentinel(const char* = "") {}
};

#endif // jstream_hpp
