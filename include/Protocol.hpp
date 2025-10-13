#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <arpa/inet.h>


namespace Protocol {
    
    enum class CommandID : uint8_t {
        IDENTIFY  = 0,
        GET_FILE  = 1,
        PUT_FILE  = 2,
        ENUMERATE = 3
    };

    struct ProxyHeader {
        in_addr dest_addr;   // Destination IP Address, 4 Bytes (IPv4)
        uint16_t dest_port;  // Destination Port, 2 Bytes
    };

    constexpr size_t COMMAND_HEADER_SIZE = 4;

    struct CommandHeader {
        CommandID command_id;
        uint8_t reserved[3]{};

        static bool parse(const std::vector<char>& buffer, CommandHeader& out);
    };

    struct FileHeader {
        uint16_t permissions;
        std::string path;
        uint64_t file_size;

        static bool parse(const std::vector<char>& buffer, size_t offset, FileHeader& out, size_t& out_next_offset);
    };

    enum class ReplyStatus : uint8_t {
        ACK   =   0,
        NACK  =   1,
        INVALID = 254, //Use for when the user tries to do something invalid, like GET_FILE on a non-existent file
        ERROR = 255
    };

    void sendReply(int socket_fd, ReplyStatus status);

    // Integer Parsing / Writing
    uint16_t parse_uint16(const char* data);
    uint64_t parse_uint64(const char* data);

    void write_uint16(char* dest, uint16_t value);
    void write_uint64(char* dest, uint64_t value);

} // namespace Protocol

#endif // PROTOCOL_HPP