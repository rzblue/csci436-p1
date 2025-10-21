#include <bit>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>

#include "Protocol.hpp"
#include <iostream>

namespace Protocol {

    bool CommandHeader::parse(const std::vector<char>& buffer, CommandHeader& out) {
        if (buffer.size() < COMMAND_HEADER_SIZE) return false;
        out.command_id = static_cast<CommandID>(static_cast<uint8_t>(buffer[0]));
        std::memcpy(out.reserved, &buffer[1], 3);
        return true;
    }

    bool FileHeader::parse(const std::vector<char>& buffer, size_t offset, FileHeader& out, size_t& out_next_offset) {
        if (buffer.size() < offset + 4) return false;

        out.permissions = parse_uint16(&buffer[offset]);
        uint16_t path_len = parse_uint16(&buffer[offset + 2]);

        size_t needed = offset + 2 + 2 + path_len + 8; // perms(2) + path_len(2) + path(path_len) + file_size(8)
        if (buffer.size() < needed) return false;

        out.path = std::string(&buffer[offset + 4], path_len);
        out.file_size = parse_uint64(&buffer[offset + 4 + path_len]);

        out_next_offset = needed;
        return true;
    }

    // Refactored by GPT4
    void sendReply(int socket_fd, ReplyStatus status) {
        uint8_t value = static_cast<uint8_t>(status);
        send(socket_fd, &value, sizeof(value), 0);  // 1-byte reply
    }

    /** reverses the byte order (polyfill for std::byteswap from C++23) */
    template<typename T> requires std::integral<T>
    constexpr T byteswap(T value) noexcept {        
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            return static_cast<T>((value >> 8) | (value << 8));
        } else if constexpr (sizeof(T) == 4) {
            value = ((value >> 8) & 0x00FF00FF) | ((value << 8) & 0xFF00FF00);
            return (value >> 16) | (value << 16);
        } else if constexpr (sizeof(T) == 8) {
            value = ((value >> 8) & 0x00FF00FF00FF00FF) | ((value << 8) & 0xFF00FF00FF00FF00);
            value = ((value >> 16) & 0x0000FFFF0000FFFF) | ((value << 16) & 0xFFFF0000FFFF0000);
            return (value >> 32) | (value << 32);
        }
    }

    /** Convert an integral value between native and little endian */
    template<typename T> requires std::integral<T>
    constexpr T le_convert(T value) {
        if constexpr (std::endian::native == std::endian::little) {
            return value; // Do nothing on little endian
        } else {
            return byteswap(value); // Swap the bytes on big endian
        }
    }

    // Parse a 2-byte little-endian uint from buffer
    uint16_t parse_uint16(const char* data) {
        uint16_t raw = *reinterpret_cast<const uint16_t*>(data);
        return le_convert(raw);
    }

    // Parse an 8-byte little-endian uint from buffer
    uint64_t parse_uint64(const char* data) {
        uint64_t raw = *reinterpret_cast<const uint64_t*>(data);
        return le_convert(raw);
    }

    // Write a 2-byte little-endian uint to buffer
    void write_uint16(char* dest, uint16_t value) {
        value = le_convert(value);
        *reinterpret_cast<uint16_t*>(dest) = value;
    }

    // Write an 8-byte little-endian uint to buffer
    void write_uint64(char* dest, uint64_t value) {
        value = le_convert(value);
        *reinterpret_cast<uint64_t*>(dest) = value;
 }

} // namespace Protocol