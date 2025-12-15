#include "Utilities.hpp"

namespace Utilities {

    DataValidationResult validateReceivedData(const unsigned char* data, size_t dataSize, const std::string& context) {
        DataValidationResult result;
        result.isValid = true;
        result.hasSuspiciousPattern = false;
        result.checksum = 0;
        result.message = "";

        // CODING ERROR CHECKS
        if (data == nullptr || dataSize == 0) {
            result.isValid = false;
            result.message = context + " validation failed: null or empty data";
            return result;
        }

        // Calculate XOR checksum
        for (size_t i = 0; i < dataSize; i++) {
            result.checksum ^= data[i];
        }

        // Build hex string representation
        std::ostringstream hexStream;
        for (size_t i = 0; i < dataSize; i++) {
            if (i > 0) hexStream << " ";
            hexStream << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i];
        }
        result.hexData = hexStream.str();

        // CHECK FOR ALL ZEROS OR ALL ONES
        bool allZeros = true;
        bool allOnes = true;
        for (size_t i = 0; i < dataSize; i++) {
            if (data[i] != 0x00) allZeros = false;
            if (data[i] != 0xFF) allOnes = false;
        }

        if (allZeros || allOnes) {
            result.hasSuspiciousPattern = true;
            result.message = context + " has suspicious pattern (all " + (allZeros ? "zeros" : "ones") + ")";
        }

        // CHECK FOR PARTIAL CORRUPTION TODO:

        return result;
    }

    bool validateAndLogData(const unsigned char* data, size_t dataSize, const std::string& context) {
        auto validation = validateReceivedData(data, dataSize, context);
        
        if (!validation.isValid) {
            Debug.Error("Data validation failed: " + validation.message);
            return false;
        }
        
        if (validation.hasSuspiciousPattern) Debug.Warn("Suspicious data: " + validation.message);
        
        // std::ostringstream checksumStream;
        // checksumStream << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)validation.checksum;
        // Debug.Log(context + " received: " + validation.hexData + " (checksum: " + checksumStream.str() + ")");
        return true;
    }

    void sleepMs(int milliseconds) {std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));}

}