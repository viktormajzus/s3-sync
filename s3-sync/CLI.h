#pragma once
#include "Error.h"
#include <expected>
#include <string>
#include <vector>
#include <filesystem>

class CLI {
private:
	int argc;
	char** argv;
	std::filesystem::path configPath;

public:
	CLI(int argc, char** argv);

private:
	std::expected<void, Error::ErrorCode> Configure();
	std::expected < std::vector<std::string>, Error::ErrorCode> ReadConfigFile();
	void Setup();
	void InvalidArguments();
	std::expected<std::vector<std::string>, Error::ErrorCode> CheckConfigVector();
	bool CheckArgCount(int argc);
};