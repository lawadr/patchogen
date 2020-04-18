#include "common.h"
#include <fstream>

int main(int argc, char* argv[])
{
	std::ifstream inFile(argv[1], std::ios::binary);

	if (!inFile.is_open())
		return EXIT_FAILURE;

	std::ofstream outFile(argv[2], std::ios::binary);

	if (!outFile.is_open())
		return EXIT_FAILURE;

	zip(inFile, outFile);

	return EXIT_SUCCESS;
}
