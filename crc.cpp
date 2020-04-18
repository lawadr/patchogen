#include "common.h"
#include <iostream>
#include <fstream>
#include <iomanip>

int main(int argc, char* argv[])
{
	std::ifstream inFile(argv[1], std::ios::binary);

	if (!inFile.is_open())
		return EXIT_FAILURE;

	auto crc = crc32(inFile);

	if (argc > 2)
	{
		std::ofstream outFile(argv[2], std::ios::binary);

		if (!outFile.is_open())
			return EXIT_FAILURE;

		outFile.write(reinterpret_cast<char*>(&crc), sizeof(crc));
	}
	else
	{
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << crc;
	}

	return EXIT_SUCCESS;
}
