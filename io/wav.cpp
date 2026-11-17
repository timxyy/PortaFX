#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <fstream>

std::vector<uint8_t> read_file(const std::string& path) {
    // Open file as binary from the end
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("File could not be opened: " + path); // Catch io error

    // Since file was opened from end, this gets the size of the file
    std::streamsize size = file.tellg();
    // Put the tracker to the beginning
    file.seekg(0);

    std::vector<uint8_t> bytes(size); // Store the entire file into memory as chunks (bytes)
    file.read(reinterpret_cast<char*>(bytes.data()), size); // Reread the file as char*
    if (!file) throw std::runtime_error("File could not be read: " + path);
    return bytes;
}

uint16_t read_U16(const std::vector<uint8_t>& bytes, size_t pos) {
    return (uint16_t)bytes[pos + 1] << 8 | (uint16_t)bytes[pos];
}

uint32_t read_U32(const std::vector<uint8_t>& bytes, size_t pos) {
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        out |= (uint32_t)bytes[pos + i] << (i * 8);
    }
    return out;
}