#ifndef EVENTENUMS_H
#define EVENTENUMS_H
//NOTE: This approach has a limit of 256 enum values.
#ifndef SWIG
    #include <string>
    #include <vector>

    #define NAMED_ENUM(enumName, ...) \
    enum enumName { \
        __VA_ARGS__ \
    }; \
    static const char* GetName(enumName e) { \
        static std::vector<std::string> names; \
        static bool init = false; \
        if (!init) { \
            init = true; \
            std::string s = #__VA_ARGS__; \
            size_t start = 0; \
            while (start < s.size()) { \
                size_t end = s.find(',', start); \
                if (end == std::string::npos) end = s.size(); \
                std::string item = s.substr(start, end - start); \
                size_t lineStart = 0; \
                while (lineStart < item.size()) { \
                    size_t lineEnd = item.find('\n', lineStart); \
                    if (lineEnd == std::string::npos) lineEnd = item.size(); \
                    std::string line = item.substr(lineStart, lineEnd - lineStart); \
                    size_t p1 = line.find_first_not_of(" \t\r"); \
                    size_t p2 = line.find_last_not_of(" \t\r"); \
                    if (p1 != std::string::npos && p2 != std::string::npos) { \
                        std::string token = line.substr(p1, p2 - p1 + 1); \
                        if (token.substr(0, 2) != "//") { \
                            size_t eq = token.find('='); \
                            if (eq != std::string::npos) token = token.substr(0, eq); \
                            size_t tp = token.find_last_not_of(" \t\r"); \
                            if (tp != std::string::npos) token = token.substr(0, tp + 1); \
                            names.push_back(token); \
                            break; \
                        } \
                    } \
                    lineStart = lineEnd + 1; \
                } \
                start = end + 1; \
            } \
        } \
        if ((size_t)e < names.size()) return names[(size_t)e].c_str(); \
        return ""; \
    };
#else //#ifdef SWIG
    %define NAMED_ENUM(enumName, ...)
        enum enumName {
            __VA_ARGS__
        };
    %enddef //NAMED_ENUM
#endif //#ifdef SWIG

#endif //EVENTENUMS_H
