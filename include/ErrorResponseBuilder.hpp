#ifndef ERROR_RESPONSE_BUILDER_HPP
#define ERROR_RESPONSE_BUILDER_HPP

#include <string>
#include <vector>

/**
 * ErrorResponseBuilder - Generates HTTP error responses with styled HTML pages
 * 
 * Responsibilities:
 * - Build complete HTTP error responses
 * - Generate styled HTML error pages
 * - Handle different error types (403, 503, 502, etc.)
 */
class ErrorResponseBuilder {
public:
    /**
     * Build 403 Forbidden response
     * Used when client request contains forbidden content
     * 
     * @param blocked_terms List of forbidden words that were matched
     * @return Complete HTTP response (headers + body)
     */
    static std::string build403Forbidden(const std::vector<std::string>& blocked_terms);

    /**
     * Build 503 Service Unavailable response
     * Used when server response contains forbidden content
     * 
     * @param blocked_terms List of forbidden words that were matched
     * @return Complete HTTP response (headers + body)
     */
    static std::string build503ServiceUnavailable(const std::vector<std::string>& blocked_terms);

    /**
     * Build 502 Bad Gateway response
     * Used when proxy cannot connect to destination server
     * 
     * @param reason Optional reason for failure
     * @return Complete HTTP response (headers + body)
     */
    static std::string build502BadGateway(const std::string& reason = "");

    /**
     * Build 400 Bad Request response
     * Used when client request is malformed
     * 
     * @param reason Optional reason for rejection
     * @return Complete HTTP response (headers + body)
     */
    static std::string build400BadRequest(const std::string& reason = "");

private:
    /**
     * Generate styled HTML error page
     * 
     * @param error_code HTTP status code (403, 502, etc.)
     * @param title Page title
     * @param heading Main heading text
     * @param message Primary error message
     * @param blocked_terms List of blocked terms (empty if not applicable)
     * @param color Theme color for the error page
     * @return Complete HTML page
     */
    static std::string buildErrorHTML(int error_code,
                                       const std::string& title,
                                       const std::string& heading,
                                       const std::string& message,
                                       const std::vector<std::string>& blocked_terms,
                                       const std::string& color);

    /**
     * Build complete HTTP response from status line and HTML body
     * 
     * @param status_line HTTP status line (e.g., "HTTP/1.1 403 Forbidden")
     * @param body HTML body content
     * @return Complete HTTP response
     */
    static std::string buildResponse(const std::string& status_line,
                                      const std::string& body);

    /**
     * HTML escape a string to prevent XSS
     * Converts: < > & " ' to their HTML entities
     * 
     * @param text Text to escape
     * @return HTML-safe text
     */
    static std::string htmlEscape(const std::string& text);
};

#endif // ERROR_RESPONSE_BUILDER_HPP