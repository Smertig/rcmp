#include <rcmp/memory.hpp>
#include <rcmp/codegen.hpp>

#include <array>
#include <optional>
#include <vector>
#include <cassert>
#include <algorithm>

static_assert(RCMP_GET_ARCH() == RCMP_ARCH_X86 || RCMP_GET_ARCH() == RCMP_ARCH_X86_64);

namespace {

std::string hex_dump(rcmp::address_t address, std::size_t count) {
    std::array<char, 0x100> buffer;
    std::string result;

    snprintf(buffer.data(), buffer.size(), "[%0" PRIXPTR "]", address.as_number());
    result += buffer.data();

    for (std::size_t i = 0; i < count; i++) {
        snprintf(buffer.data(), buffer.size(), " %02X", static_cast<unsigned>(address.as_ptr<const std::uint8_t>()[i]));
        result += buffer.data();
    }

    return result;
}

using jmp_diff_t = std::int32_t;

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
constexpr std::size_t g_jmp_size = 1 + sizeof(jmp_diff_t);

void make_jmp(rcmp::address_t from, rcmp::address_t to) {
    const jmp_diff_t delta = to - (from + g_jmp_size);
    static_assert(sizeof(delta) == 4);

    std::array<std::byte, g_jmp_size> code;
    code[0] = std::byte{ 0xE9 };
    std::memcpy(&code[1], &delta, sizeof(delta));

    rcmp::set_opcode(from, code);
}
#else
constexpr std::size_t g_jmp_size = 8 + sizeof(std::uintptr_t);

// Reference: https://github.com/stevemk14ebr/PolyHook/blob/master/PolyHook/PolyHook.hpp#L1184
void make_jmp(rcmp::address_t from, rcmp::address_t to) {
    std::uintptr_t to_value = to.as_number();

    std::array<std::byte, g_jmp_size> code;
    code[0] = std::byte{ 0x50 };
    code[1] = std::byte{ 0x48 };
    code[2] = std::byte{ 0xB8 };
    std::memcpy(&code[3], &to_value, sizeof(to_value));
    code[11] = std::byte{ 0x48 };
    code[12] = std::byte{ 0x87 };
    code[13] = std::byte{ 0x04 };
    code[14] = std::byte{ 0x24 };
    code[15] = std::byte{ 0xC3 };

    rcmp::set_opcode(from, code);
}
#endif

class opcode {
    uint8_t m_len = 0;
    uint8_t m_bytes[2]{};

public:
    constexpr opcode(std::uint8_t b) : m_len(1), m_bytes{ b, 0 } {}
    constexpr opcode(std::uint8_t b1, std::uint8_t b2) : m_len(2), m_bytes{ b1, b2 } {}

    friend bool operator==(const opcode& lhs, const opcode& rhs) {
        if (lhs.m_len != rhs.m_len) {
            return false;
        }

        return std::memcmp(lhs.m_bytes, rhs.m_bytes, lhs.m_len) == 0;
    }

    friend bool operator!=(const opcode& lhs, const opcode& rhs) { return !(lhs == rhs); }

    std::size_t len() const {
        return m_len;
    }

    std::uint8_t first() const {
        return m_bytes[0];
    }

    std::uint8_t second() const {
        assert(m_len == 2);
        return m_bytes[1];
    }

    const std::uint8_t* data() const {
        return m_bytes;
    }
};

// TODO: rename
class jmp_translator {
    struct jmp_opcode_pair {
        std::optional<opcode> short_jmp;
        opcode long_jmp;
    };

    std::vector<jmp_opcode_pair> m_jmps;

    // http://ref.x86asm.net/coder32.html
    explicit jmp_translator() {
        m_jmps.push_back({ opcode(0xEB), opcode(0xE9) }); // jmp
        m_jmps.push_back({ std::nullopt, opcode(0xE8) }); // call

        // j*
        for (std::uint8_t i = 0; i < 0x10; i++) {
            m_jmps.push_back({ opcode(0x70 + i), opcode(0x0F, 0x80 + i) });
        }
    }

public:
    static const jmp_translator& instance() {
        static const jmp_translator instance;
        return instance;
    }

    std::optional<opcode> short_to_long(const opcode& op) const {
        auto it = std::find_if(std::begin(m_jmps), std::end(m_jmps), [&op](const jmp_opcode_pair& pair) {
            return pair.short_jmp == op;
        });

        return it != std::end(m_jmps) ? std::make_optional(it->long_jmp) : std::nullopt;
    }

    bool is_long(const opcode& op) const {
        return std::find_if(std::begin(m_jmps), std::end(m_jmps), [&op](const jmp_opcode_pair& pair) {
            return pair.long_jmp == op;
        }) != std::end(m_jmps);
    }

    bool is_relocatable(std::size_t length, std::uint8_t first_byte) const {
        // loop*
        if (length == 2 && first_byte >= 0xE0 && first_byte < 0xE4) {
            return false;
        }

        return true;
    }
};

std::size_t relocate_opcode(rcmp::address_t from, rcmp::address_t to) {
    char dummy[30];
    if (to == nullptr) {
        to = &dummy;
    }

    const auto cmd_len    = rcmp::opcode_length(from);
    const auto bytes_from = from.as_ptr<const uint8_t>();
    const auto bytes_to   = to.as_ptr<uint8_t>();

    if (cmd_len == 0) {
        throw rcmp::error("unknown opcode: %s...", hex_dump(from, 4).c_str());
    }

    if (!jmp_translator::instance().is_relocatable(cmd_len, bytes_from[0])) {
        throw rcmp::error("unsupported opcode: %s", hex_dump(from, cmd_len).c_str());
    }

    std::optional<std::pair<opcode /* long jmp */, std::ptrdiff_t /* old_offset */>> jmp_info;

    if (cmd_len == 2) {
        if (auto maybe_long_opcode = jmp_translator::instance().short_to_long(opcode{ bytes_from[0] })) {
            // relative jmp [XX YY], where YY is offset
            std::int8_t jmp_offset = 0;
            std::memcpy(&jmp_offset, bytes_from + 1, sizeof(jmp_offset));

            jmp_info.emplace(*maybe_long_opcode, jmp_offset);
        }
    }
    else if (cmd_len == 1 + sizeof(jmp_diff_t)) {
        opcode cmd = opcode{ bytes_from[0] };
        if (jmp_translator::instance().is_long(cmd)) {
            // relative jmp [XX YY YY YY YY], where YY is offset
            jmp_diff_t jmp_offset = 0;
            std::memcpy(&jmp_offset, bytes_from + 1, sizeof(jmp_offset));

            jmp_info.emplace(cmd, jmp_offset);
        }
    }
    else if (cmd_len == 2 + sizeof(jmp_diff_t)) {
        opcode cmd = opcode{ bytes_from[0], bytes_from[1] };
        if (jmp_translator::instance().is_long(cmd)) {
            // relative jmp [XX XX YY YY YY YY], where YY is offset
            jmp_diff_t jmp_offset = 0;
            std::memcpy(&jmp_offset, bytes_from + 2, sizeof(jmp_offset));

            jmp_info.emplace(cmd, jmp_offset);
        }
    }

    // requires relocation
    if (jmp_info) {
        const auto [long_opcode, old_jmp_offset] = *jmp_info;

        const std::uint8_t* jmp_destination_address = (bytes_from + cmd_len) + old_jmp_offset;

        const std::ptrdiff_t new_jmp_offset_long = jmp_destination_address - (bytes_to + long_opcode.len() + sizeof(jmp_diff_t));
        const jmp_diff_t new_jmp_offset = static_cast<jmp_diff_t>(new_jmp_offset_long);

        // TODO: refactor
        if (new_jmp_offset_long != new_jmp_offset) {
            throw rcmp::error("unable to relocate jmp, too far");
        }

        std::memcpy(bytes_to, long_opcode.data(), long_opcode.len());
        std::memcpy(bytes_to + long_opcode.len(), &new_jmp_offset, sizeof(new_jmp_offset));

        return long_opcode.len() + sizeof(jmp_diff_t);
    }
    else {
        // default
        std::memcpy(bytes_to, bytes_from, cmd_len);
        return cmd_len;
    }
}

std::unique_ptr<std::byte[]> relocate_function(rcmp::address_t address, std::size_t bytes) {
    if (bytes == 0) {
        return nullptr;
    }

    const std::size_t relocated_size = [address, bytes]{
        rcmp::address_t from_it  = address;
        std::size_t     out_size = 0;

        while (from_it < address + bytes) {
            out_size += relocate_opcode(from_it, nullptr);
            from_it  += rcmp::opcode_length(from_it);
        }
        return out_size;
    }();

    // 1 + sizeof(..) == sizeof(jmp)
    auto result = rcmp::allocate_code(relocated_size + g_jmp_size);

    // copy beginning of func to result
    rcmp::address_t from_it = address;
    rcmp::address_t out_it  = result.get();

    while (from_it < address + bytes) {
        out_it  += relocate_opcode(from_it, out_it);
        from_it += rcmp::opcode_length(from_it);
    }

    rcmp::unprotect_memory(address, from_it - address);
    std::memset(rcmp::bit_cast<char*>(address), 0x90, from_it - address);

    // jump from end of result to original func
    make_jmp(out_it, from_it);

    return result;
}

} // unnamed namespace

// returns relocated original address
rcmp::address_t rcmp::make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function) {
    auto new_original = relocate_function(original_function, g_jmp_size);

    make_jmp(original_function, wrapper_function);

    // force memory leak
    return new_original.release();
}

#define RCMP_ENABLE_ONLY_LENGTH_DISASM

#define NMD_ASSEMBLY_IMPLEMENTATION

#if RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
    #pragma GCC diagnostic push
    // nothing
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmissing-braces"
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    #pragma warning(push)
    // nothing
#endif

#include <nmd_assembly.h>

#if RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
    #pragma GCC diagnostic pop
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG
    #pragma clang diagnostic pop
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    #pragma warning(pop)
#endif
