#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <fstream>

std::vector<uint8_t> read_file(const std::string& path);
uint16_t read_U16(const std::vector<uint8_t>& bytes, size_t pos);
uint32_t read_U32(const std::vector<uint8_t>& bytes, size_t pos);