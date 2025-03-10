#pragma once

#include <string>

class skCrypt {
private:
    std::string encrypted;
public:
    explicit skCrypt(const char* input) {
        encrypted = input; // This is just a placeholder (actual encryption logic should be implemented)
    }
    std::string decrypt() const {
        return encrypted; // Return original string (replace this with actual decryption logic)
    }
};