#pragma once
#include "common.h"
#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

class File
{
public:
	File()
	{
		File::g_files.push_back(*this);
	}

	virtual bool validate(const std::filesystem::path& root) = 0;
	virtual bool perform(const std::filesystem::path& root) = 0;

	static const std::vector<std::reference_wrapper<File>>& getFiles()
	{
		return g_files;
	}

private:
	static std::vector<std::reference_wrapper<File>> g_files;
};

class AddedFile : public File
{
public:
	AddedFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& buffer)
		: m_path(path)
		, m_data(buffer)
	{
	}

	bool validate(const std::filesystem::path& root)
	{
		return !std::filesystem::exists(root / m_path);
	}

	bool perform(const std::filesystem::path& root)
	{
		std::cout << "Creating: " << m_path.make_preferred().string() << std::endl;

		auto path = root / m_path;
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary);

		if (!file.is_open())
			return false;

		unzip(m_data, file);

		return true;
	}

private:
	std::filesystem::path m_path;
	const std::vector<std::uint8_t>& m_data;
};

class RemovedFile : public File
{
public:
	RemovedFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& buffer)
		: m_path(path)
		, m_checkSum(*reinterpret_cast<const std::uint32_t*>(buffer.data()))
	{
	}

	bool validate(const std::filesystem::path& root)
	{
		auto path = root / m_path;

		if (std::filesystem::is_regular_file(path))
		{
			std::ifstream file(path, std::ios::binary);
			return crc32(file) == m_checkSum;
		}
		
		return false;
	}

	bool perform(const std::filesystem::path& root)
	{
		std::cout << "Deleting: " << m_path.make_preferred().string() << std::endl;
		return std::filesystem::remove(root / m_path);
	}

private:
	std::filesystem::path m_path;
	std::uint32_t m_checkSum;
};

class ModifiedFile : public File
{
public:
	ModifiedFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& crc, const std::vector<std::uint8_t>& buffer)
		: m_path(path)
		, m_checkSum(*reinterpret_cast<const std::uint32_t*>(crc.data()))
		, m_patch(buffer)
	{
	}

	bool validate(const std::filesystem::path& root)
	{
		auto path = root / m_path;

		if (std::filesystem::is_regular_file(path))
		{
			std::ifstream file(path, std::ios::binary);
			return crc32(file) == m_checkSum;
		}

		return false;
	}

	bool perform(const std::filesystem::path& root)
	{
		std::cout << "Updating: " << m_path.make_preferred().string() << std::endl;

		auto path = root / m_path;
		auto tempPath = root / (m_path.string() + ".tmp");

		{
			std::ifstream inFile(path, std::ios::binary);

			if (!inFile.is_open())
				return false;

			std::ofstream tempFile(tempPath, std::ios::binary);

			if (!tempFile.is_open())
				return false;

			patch(m_patch, inFile, tempFile);
		}

		std::filesystem::rename(tempPath, path);

		return true;
	}

private:
	std::filesystem::path m_path;
	std::uint32_t m_checkSum;
	const std::vector<std::uint8_t>& m_patch;
};
