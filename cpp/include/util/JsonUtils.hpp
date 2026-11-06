#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace plagiguard::util {

/**
 * @brief JsonUtils - Minimal, fast JSON builder for API responses.
 */
class JsonUtils {
public:
    static std::string escape(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) <= 0x1f) {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }
};

} // namespace plagiguard::util

