#pragma once
#include <string>
#include <string_view>

namespace http_utils {
/**
 * Remove a specific header from an HTTP request
 *
 * @param request HTTP request string
 * @param header_name Name of header to remove (case-insensitive)
 * @return Modified request with header removed
 */
std::string removeHeader(std::string_view headers,
                         std::string_view header_name);

/**
 * Insert or update a header in an HTTP request
 * If the header already exists, it will be replaced.
 * The header is inserted before the final \r\n\r\n separator.
 *
 * @param request HTTP request string
 * @param header_name Name of header to insert (case-insensitive for replacement
 * check)
 * @param header_value Value of the header
 * @return Modified request with header inserted/updated
 */
std::string insertHeader(std::string_view request, std::string_view header_name,
                         std::string_view header_value);
} // namespace http_utils