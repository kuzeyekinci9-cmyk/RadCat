#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <concepts>

struct DebugClass {

    static DebugClass& getInstance() {
        static DebugClass instance;
        return instance;
    }

    // Stringify any type
    template<typename T>
    std::string stringify(const T& v) {
        if constexpr (requires { std::ostringstream{} << v; }) {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        } else if constexpr (requires { std::to_string(v); }) {
            return std::to_string(v);
        } else {
            return "[unprintable type]";
        }
    }

    constexpr static int debugLevel = 3; // 0=none, 1=errors, 2=warnings, 3=info

    // ANSI color codes
    enum class Color { DEFAULT, RED, YELLOW, GREEN, CYAN, WHITE };

    inline static std::string colorCode(Color c) {
        switch (c) {
            case Color::RED:    return "\033[31m"; // Error
            case Color::YELLOW: return "\033[33m"; // Warning
            case Color::GREEN:  return "\033[32m"; // Timestamp
            case Color::CYAN:   return "\033[36m"; // For later use
            case Color::WHITE:  return "\033[37m"; // Log
            default:            return "\033[0m";
        }
    }

    // Get current time as HH:MM:SS
    inline static std::string currentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_since_epoch = now.time_since_epoch();
        auto hours = std::chrono::duration_cast<std::chrono::hours>(time_since_epoch) % 24;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time_since_epoch) % 60;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch) % 60;
        
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours.count() << ":"
            << std::setfill('0') << std::setw(2) << minutes.count() << ":"
            << std::setfill('0') << std::setw(2) << seconds.count();
        return oss.str();
    }

    // Printing
    inline static void print(const std::string& prefix, const std::string& msg, Color c, bool isError=false, int LinesBeforeMessage=1) {
        for (int i = 0; i < LinesBeforeMessage; ++i) std::cout << std::endl; // Empty Lines
        std::ostream& out = isError ? std::cerr : std::cout;
        out << colorCode(Color::GREEN) << "[" << currentTime() << "] " << colorCode(Color::DEFAULT); // Timestamp
        out << colorCode(c) << prefix << " " << msg << colorCode(Color::DEFAULT) << std::endl; // Prefix and message
    }

    // Logging functions
    inline void Log(const std::string& msg, int LinesBeforeMessage=1)    { if constexpr (debugLevel >= 3) print("[DEBUG]:", msg, Color::WHITE, false, LinesBeforeMessage); }
    inline void Warn(const std::string& msg, int LinesBeforeMessage=1)   { if constexpr (debugLevel >= 2) print("[WARNING]:", msg, Color::YELLOW, false, LinesBeforeMessage); }
    inline void Error(const std::string& msg, int LinesBeforeMessage=1)  { if constexpr (debugLevel >= 1) print("[ERROR]:", msg, Color::RED, true, LinesBeforeMessage); }

    //Templated function for any type
    template<typename... Args> 
    inline void Log(const Args&... args) {
        if constexpr (debugLevel < 3) return;
        std::ostringstream oss;
        ((oss << stringify(args)), ...);
        Log(oss.str());
    }

    template<typename... Args> 
    inline void Warn(const Args&... args) {
        if constexpr (debugLevel < 2) return;
        std::ostringstream oss;
        ((oss << stringify(args)), ...);
        Warn(oss.str());
    }

    template<typename... Args> 
    inline void Error(const Args&... args) {
        if constexpr (debugLevel < 1) return;
        std::ostringstream oss;
        ((oss << stringify(args)), ...);
        Error(oss.str());
    }

};

inline DebugClass& Debug = DebugClass::getInstance();