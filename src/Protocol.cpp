#include <cstring>

#include "Protocol.hpp"


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

        size_t needed = offset + 4 + path_len + 8; // perms(2) + path_len(2) + path + file_size(8)
        if (buffer.size() < needed) return false;

        out.path = std::string(&buffer[offset + 4], path_len);
        out.file_size = parse_uint64(&buffer[offset + 4 + path_len]);

        out_next_offset = needed;
        return true;
    }

    uint16_t parse_uint16(const char* data) {
        return (static_cast<uint8_t>(data[0]) << 8) |
               static_cast<uint8_t>(data[1]);
    }

    uint64_t parse_uint64(const char* data) {
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result = (result << 8) | static_cast<uint8_t>(data[i]);
        }
        return result;
    }

    void write_uint16(char* dest, uint16_t value) {
        dest[0] = static_cast<char>((value >> 8) & 0xFF);
        dest[1] = static_cast<char>(value & 0xFF);
    }

    void write_uint64(char* dest, uint64_t value) {
        for (int i = 7; i >= 0; --i) {
            dest[i] = static_cast<char>(value & 0xFF);
            value >>= 8;
        }
    }

} // namespace Protocol