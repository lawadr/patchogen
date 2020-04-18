#include "common.h"
#include <fstream>

int main(int argc, char* argv[])
{
	std::ifstream oldFile(argv[1], std::ios::binary);

	if (!oldFile.is_open())
		return EXIT_FAILURE;

	std::ifstream newFile(argv[2], std::ios::binary);

	if (!newFile.is_open())
		return EXIT_FAILURE;

	std::ofstream outFile(argv[3], std::ios::binary);

	if (!outFile.is_open())
		return EXIT_FAILURE;

	diff(oldFile, newFile, outFile);

	return EXIT_SUCCESS;
}
