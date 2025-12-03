#include "ErrorResponseBuilder.hpp"
#include <sstream>

// ====================================================================================================
// Public Methods - Error Response Builders
// ====================================================================================================

std::string ErrorResponseBuilder::build403Forbidden(const std::vector<std::string>& blocked_terms) {
    std::string html = buildErrorHTML(
        403,
        "403 Forbidden - Content Blocked",
        "Request Blocked",
        "Your request was blocked because it contained forbidden content.",
        blocked_terms,
        "#d32f2f"  // Red color theme
    );

    return buildResponse("HTTP/1.1 403 Forbidden", html);
}

std::string ErrorResponseBuilder::build503ServiceUnavailable(const std::vector<std::string>& blocked_terms) {
    std::string html = buildErrorHTML(
        503,
        "503 Service Unavailable - Content Blocked",
        "Response Blocked",
        "The server's response was blocked because it contained forbidden content.",
        blocked_terms,
        "#f57c00"  // Orange color theme
    );

    return buildResponse("HTTP/1.1 503 Service Unavailable", html);
}

std::string ErrorResponseBuilder::build502BadGateway(const std::string& reason) {
    std::string message = "The proxy server could not connect to the destination server.";
    if (!reason.empty()) {
        message += " Reason: " + htmlEscape(reason);
    }

    std::string html = buildErrorHTML(
        502,
        "502 Bad Gateway",
        "Connection Failed",
        message,
        {},  // No blocked terms
        "#9c27b0"  // Purple color theme
    );

    return buildResponse("HTTP/1.1 502 Bad Gateway", html);
}

std::string ErrorResponseBuilder::build400BadRequest(const std::string& reason) {
    std::string message = "Your request could not be understood by the proxy server.";
    if (!reason.empty()) {
        message += " Reason: " + htmlEscape(reason);
    }

    std::string html = buildErrorHTML(
        400,
        "400 Bad Request",
        "Invalid Request",
        message,
        {},  // No blocked terms
        "#e91e63"  // Pink color theme
    );

    return buildResponse("HTTP/1.1 400 Bad Request", html);
}

// ====================================================================================================
// Private Helper Methods
// ====================================================================================================

std::string ErrorResponseBuilder::buildErrorHTML(int error_code,
                                                  const std::string& title,
                                                  const std::string& heading,
                                                  const std::string& message,
                                                  const std::vector<std::string>& blocked_terms,
                                                  const std::string& color) {
    std::ostringstream html;

    // Background color (lighter version of main color)
    std::string bg_color;
    if (color == "#d32f2f") bg_color = "#ffebee";      // Light red
    else if (color == "#f57c00") bg_color = "#fff3e0";  // Light orange
    else if (color == "#9c27b0") bg_color = "#f3e5f5";  // Light purple
    else if (color == "#e91e63") bg_color = "#fce4ec";  // Light pink
    else bg_color = "#f5f5f5";                          // Default gray

    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"UTF-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "  <title>" << htmlEscape(title) << "</title>\n"
         << "  <style>\n"
         << "    * { margin: 0; padding: 0; box-sizing: border-box; }\n"
         << "    body {\n"
         << "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, \n"
         << "                   'Helvetica Neue', Arial, sans-serif;\n"
         << "      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
         << "      min-height: 100vh;\n"
         << "      display: flex;\n"
         << "      align-items: center;\n"
         << "      justify-content: center;\n"
         << "      padding: 20px;\n"
         << "    }\n"
         << "    .container {\n"
         << "      background: white;\n"
         << "      border-radius: 12px;\n"
         << "      box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);\n"
         << "      max-width: 600px;\n"
         << "      width: 100%;\n"
         << "      overflow: hidden;\n"
         << "      animation: slideUp 0.5s ease-out;\n"
         << "    }\n"
         << "    @keyframes slideUp {\n"
         << "      from { opacity: 0; transform: translateY(30px); }\n"
         << "      to { opacity: 1; transform: translateY(0); }\n"
         << "    }\n"
         << "    .header {\n"
         << "      background: " << color << ";\n"
         << "      color: white;\n"
         << "      padding: 30px;\n"
         << "      text-align: center;\n"
         << "    }\n"
         << "    .error-code {\n"
         << "      font-size: 72px;\n"
         << "      font-weight: bold;\n"
         << "      line-height: 1;\n"
         << "      margin-bottom: 10px;\n"
         << "      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.2);\n"
         << "    }\n"
         << "    .error-title {\n"
         << "      font-size: 24px;\n"
         << "      font-weight: 600;\n"
         << "    }\n"
         << "    .content {\n"
         << "      padding: 30px;\n"
         << "    }\n"
         << "    .message {\n"
         << "      font-size: 16px;\n"
         << "      line-height: 1.6;\n"
         << "      color: #333;\n"
         << "      margin-bottom: 20px;\n"
         << "    }\n"
         << "    .blocked-terms {\n"
         << "      background: " << bg_color << ";\n"
         << "      border-left: 4px solid " << color << ";\n"
         << "      border-radius: 4px;\n"
         << "      padding: 15px 20px;\n"
         << "      margin: 20px 0;\n"
         << "    }\n"
         << "    .blocked-terms h3 {\n"
         << "      color: " << color << ";\n"
         << "      font-size: 14px;\n"
         << "      font-weight: 600;\n"
         << "      text-transform: uppercase;\n"
         << "      letter-spacing: 0.5px;\n"
         << "      margin-bottom: 10px;\n"
         << "    }\n"
         << "    .blocked-terms ul {\n"
         << "      list-style: none;\n"
         << "    }\n"
         << "    .blocked-terms li {\n"
         << "      padding: 8px 0;\n"
         << "      font-family: 'Courier New', monospace;\n"
         << "      font-weight: bold;\n"
         << "      color: " << color << ";\n"
         << "      border-bottom: 1px solid rgba(0, 0, 0, 0.05);\n"
         << "    }\n"
         << "    .blocked-terms li:last-child {\n"
         << "      border-bottom: none;\n"
         << "    }\n"
         << "    .footer {\n"
         << "      margin-top: 30px;\n"
         << "      padding-top: 20px;\n"
         << "      border-top: 1px solid #eee;\n"
         << "      color: #666;\n"
         << "      font-size: 14px;\n"
         << "      text-align: center;\n"
         << "    }\n"
         << "    .icon {\n"
         << "      font-size: 48px;\n"
         << "      margin-bottom: 10px;\n"
         << "    }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <div class=\"container\">\n"
         << "    <div class=\"header\">\n"
         << "      <div class=\"icon\">🚫</div>\n"
         << "      <div class=\"error-code\">" << error_code << "</div>\n"
         << "      <div class=\"error-title\">" << htmlEscape(heading) << "</div>\n"
         << "    </div>\n"
         << "    <div class=\"content\">\n"
         << "      <div class=\"message\">\n"
         << "        " << htmlEscape(message) << "\n"
         << "      </div>\n";

    // Add blocked terms section if there are any
    if (!blocked_terms.empty()) {
        html << "      <div class=\"blocked-terms\">\n"
             << "        <h3>Forbidden Terms Detected</h3>\n"
             << "        <ul>\n";

        for (const auto& term : blocked_terms) {
            html << "          <li>" << htmlEscape(term) << "</li>\n";
        }

        html << "        </ul>\n"
             << "      </div>\n";
    }

    html << "      <div class=\"footer\">\n"
         << "        If you believe this is an error, please contact your network administrator.\n"
         << "      </div>\n"
         << "    </div>\n"
         << "  </div>\n"
         << "</body>\n"
         << "</html>";

    return html.str();
}

std::string ErrorResponseBuilder::buildResponse(const std::string& status_line,
                                                 const std::string& body) {
    std::ostringstream response;

    response << status_line << "\r\n"
             << "Content-Type: text/html; charset=UTF-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-cache, no-store, must-revalidate\r\n"
             << "Pragma: no-cache\r\n"
             << "Expires: 0\r\n"
             << "\r\n"
             << body;

    return response.str();
}

std::string ErrorResponseBuilder::htmlEscape(const std::string& text) {
    std::ostringstream escaped;

    for (char c : text) {
        switch (c) {
            case '<':
                escaped << "&lt;";
                break;
            case '>':
                escaped << "&gt;";
                break;
            case '&':
                escaped << "&amp;";
                break;
            case '"':
                escaped << "&quot;";
                break;
            case '\'':
                escaped << "&#39;";
                break;
            default:
                escaped << c;
                break;
        }
    }

    return escaped.str();
}