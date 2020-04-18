#include <fstream>
#include <iomanip>

int main(int argc, char* argv[])
{
	const int rowLength = 32;

	std::ifstream inFile(argv[1], std::ios::binary);
	if (!inFile.is_open()) return EXIT_FAILURE;

	std::ofstream outFile(argv[2]);
	if (!outFile.is_open()) return EXIT_FAILURE;

	outFile << std::hex;
	outFile << "#include <vector>\n\n";
	outFile << "const std::vector<std::uint8_t> " << argv[3] << " = {";

	std::uint8_t value = 0;
	int rowCount = 0;

	while (inFile.get(reinterpret_cast<char&>(value)))
	{
		if (rowCount++ == 0)
		{
			outFile << "\n\t";
		}

		outFile << "0x" << std::setw(2) << std::setfill('0') << static_cast<std::uint32_t>(value) << ",";

		rowCount %= rowLength;
	}

	outFile << "\n};\n";

	return EXIT_SUCCESS;
}
