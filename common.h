#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

std::uint32_t crc32(std::istream& stream);

void zip(std::istream& inStream, std::ostream& outStream);
void unzip(std::istream& inStream, std::ostream& outStream);
void unzip(const std::vector<std::uint8_t>& inBuffer, std::ostream& outStream);

void diff(std::istream& oldStream, std::istream& newStream, std::ostream& outStream);
void patch(const std::vector<std::uint8_t>& patchBuffer, std::istream& inStream, std::ostream& outStream);
