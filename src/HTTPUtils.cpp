#include "HTTPUtils.hpp"
#include "StringUtils.hpp"

using namespace utils;

namespace http_utils {
std::string removeHeader(const std::string_view request,
                                             const std::string_view header_name) {
    // Create lowercase versions for case-insensitive search
    std::string lower_request = toLower(request);
    std::string lower_name = toLower(header_name);
    
    // Find header line
    std::string search_key = lower_name + ":";
    size_t pos = lower_request.find(search_key);
    
    if (pos == std::string::npos) {
        // Header not found, return original request
        return std::string{request};
    }

    // Find the start of the header line (beginning of line)
    size_t line_start = pos;
    if (pos > 0) {
        // Check if header is at the start of headers or after \r\n
        if (pos >= 2 && request[pos-2] == '\r' && request[pos-1] == '\n') {
            line_start = pos;
        } else if (pos >= 1 && request[pos-1] == '\n') {
            line_start = pos;
        } else {
            // Search backwards for the start of line
            line_start = request.rfind("\n", pos);
            if (line_start != std::string::npos) {
                line_start++;  // Move past the \n
            } else {
                line_start = 0;  // Header is on first line
            }
        }
    }

    // Find end of header line (including \r\n)
    size_t line_end = request.find("\r\n", pos);
    if (line_end == std::string::npos) {
        // Malformed request, return original
        return std::string{request};
    }
    line_end += 2;  // Include the \r\n

    // Remove the header line
    std::string result;
    result.reserve(line_start + (request.length() - line_end));
    result.append(request.substr(0, line_start));
    result.append(request.substr(line_end));

    return result;
}

std::string insertHeader(const std::string_view request,
                                             const std::string_view header_name,
                                             const std::string_view header_value) {
    // First, remove the header if it already exists
    std::string result = removeHeader(request, header_name);
    
    // Find the end of headers (\r\n\r\n)
    size_t headers_end = result.find("\r\n\r\n");
    
    if (headers_end == std::string::npos) {
        // Malformed request, return original
        return std::string{request};
    }
    
    // Insert the new header after the last header line (before the empty line)
    // headers_end points to the first \r of the last header's \r\n
    // We want to insert after that \r\n, so we add 2
    std::string new_header = std::string{header_name} + ": " + std::string{header_value} + "\r\n";
    result.insert(headers_end + 2, new_header);
    
    return result;
}
} // namespace http_utils