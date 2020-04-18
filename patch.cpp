#include "config.h"
#include "patch.h"

std::vector<std::reference_wrapper<File>> File::g_files;

int main(int argc, char* argv[])
{
	struct ExitGuard
	{
		~ExitGuard()
		{
			std::cout << "Press enter to exit...";
			std::cin.ignore();
		}
	} guard;

	if (!g_productName.empty() || !g_newVersion.empty())
	{
		std::cout << g_productName << " ";

		if (!g_newVersion.empty())
			std::cout << g_newVersion << " ";

		std::cout << "patch" << std::endl;
	}

	if (argc > 2)
	{
		std::cerr << "Invalid arguments" << std::endl;
		return EXIT_FAILURE;
	}

	auto root = std::filesystem::current_path();

	if (argc > 1)
	{
		root = std::filesystem::absolute(argv[1]);
	}
	
	if (!std::filesystem::is_directory(root))
	{
		std::cerr << "Invalid directory: " << root.string() << std::endl;
		return EXIT_FAILURE;
	}

	for (File& file : File::getFiles())
	{
		if (!file.validate(root))
		{
			if (g_oldVersion.empty())
			{
				std::cerr << "Failed to find required version" << std::endl;
			}
			else
			{
				std::cerr << "Failed to find version " << g_oldVersion << std::endl;
			}

			return EXIT_FAILURE;
		}
	}

	std::cout << "Installing to: " << root.string() << std::endl;
	std::cout << "Press enter to continue...";
	std::cin.ignore();

	for (File& file : File::getFiles())
	{
		if (!file.perform(root))
		{
			std::cout << "Installation failed" << std::endl;
			return EXIT_FAILURE;
		}
	}

	std::cout << "Installation successful" << std::endl;
	return EXIT_SUCCESS;
}
