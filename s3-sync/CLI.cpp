#include "CLI.h"
#include "AWSManager.h"

#include <string>
#include <string_view>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <utility>

CLI::CLI(int argc, char** argv)
	: argc{ argc }, argv{ argv }
{
#ifdef _WIN32
	const char* appData = std::getenv("LOCALAPPDATA");
	if (appData)
		configPath = std::filesystem::path(appData) / "s3-sync" / "config.cfg";
	else
		throw std::runtime_error("LOCALAPPDATA not set");
#elif __linux__
	const char* xdgState = std::getenv("XDG_STATE_HOME");
	if (xdgState)
		configPath = std::filesystem::path(xdgState) / "s3-sync" / "config.cfg";
	else {
		const char* home = std::getenv("HOME");
		if (!home)
			throw std::runtime_error("Neither XDG_STATE_HOME nor HOME is set");

		configPath = std::filesystem::path(home) / ".local" / "state" / "s3-sync" / "config.cfg";

#else
#error "Unsupported OS"
#endif


	Setup();
}

std::expected<void, Error::ErrorCode> CLI::Configure()
{
	std::string AccessKey{};
	std::cout << "AWS Access Key ID: ";
	std::cin >> AccessKey;

	std::string SecretKey{};
	std::cout << "AWS Secret Access Key: ";
	std::cin >> SecretKey;

	std::string region{};
	std::cout << "Region: ";
	std::cin >> region;

	std::filesystem::create_directories(configPath.parent_path());

	std::ofstream out(configPath, std::ios::binary);
	if (out.is_open()) {
		out << "valid\n";
		out << AccessKey << '\n';
		out << SecretKey << '\n';
		out << region;
		out.close();

		AccessKey.clear();
		SecretKey.clear();
		std::cout << "Successfully written config file to: " << configPath << "\n";
	}
	else
		return std::unexpected(Error::ErrorCode::FailedToOpenFile);

	return{};
}

std::expected<std::vector<std::string>, Error::ErrorCode> CLI::ReadConfigFile()
{
	namespace fs = std::filesystem;
	if (!fs::exists(configPath)) {
		return std::unexpected(Error::ErrorCode::NoConfigFile);
	}

	std::ifstream file(configPath);
	if (!file.is_open())
		return std::unexpected(Error::ErrorCode::FailedToOpenFile);

	std::vector<std::string> lines{};
	std::string line{};
	while (std::getline(file, line))
		lines.push_back(line);

	return lines;
}

void CLI::Setup()
{
	if (argc < 2) {
		InvalidArguments();
		return;
	}
	if (std::string_view(argv[1]) == "configure") {
		auto result{ Configure() };
		if (!result.has_value())
			std::cerr << "An error has occured: " << Error::ErrorParser(result.error());

		return;
	}
	auto credentials{ ReadConfigFile() };
	if (!credentials.has_value()) {
		std::cerr << "An error has occured: " << Error::ErrorParser(credentials.error());

		return;
	}

	AWSManager manager(credentials.value()[1], credentials.value()[2], credentials.value()[3]);

	credentials.value().clear();

	if (std::string_view(argv[1]) == "put") {
		int requiredArgCount{ 4 };
		if (!CheckArgCount(requiredArgCount))
			return;

		auto result{ manager.put(argv[2], argv[3]) };
		if (!result.has_value()) {
			std::cerr << Error::ErrorParser(result.error());
			return;
		}

		std::cout << "Successfully uploaded " << result.value() << " objects!\n";

		return;
	}

	if (std::string_view(argv[1]) == "get") {
		int requiredArgCount{ 4 };
		if (!CheckArgCount(requiredArgCount))
			return;

		auto result{ manager.get(argv[2], argv[3]) };
		if (!result.has_value()) {
			std::cerr << Error::ErrorParser(result.error());
			return;
		}

		std::cout << "Successfully downloaded " << result.value() << " objects!\n";

		return;
	}

	if (std::string_view(argv[1]) == "list") {
		std::expected<void, Error::ErrorCode> result;

		if (std::string_view(argv[2]) == "-b")
			result = manager.ListBuckets();
		
		else if (std::string_view(argv[2]) == "-o" && argc == 4) {
			result = manager.ListObjects(argv[3]);
		}
		else {
			InvalidArguments();
			return;
		}

		if (result.has_value())
			return;
		else {
			std::cerr << Error::ErrorParser(result.error());
			return;
		}
	}

	if (std::string_view(argv[1]) == "delete") {
		int requiredArgCount{ 3 };
		if (!CheckArgCount(requiredArgCount))
			return;
		
		std::string confirmation{};
		std::cout << "Are you sure you want to delete every object in the specified bucket? Type \"yes\" (without the quotation marks) to confirm: ";
		std::cin >> confirmation;

		if (confirmation != "yes")
			return;

		auto result{ manager.DeleteAllObjects(argv[2]) };
		if (!result.has_value()) {
			std::cerr << Error::ErrorParser(result.error());
			return;
		}

		std::cout << "Successfully deleted " << result.value().first << " out of " << result.value().second << " objects!\n";
		return;
	}

	if (std::string_view(argv[1]) == "help") {
		HelpMenu();
		return;
	}

	InvalidArguments();
	return;
}

void CLI::InvalidArguments()
{
	std::cerr << "You have used invalid arguments!\n";
	std::cout
		<< "Proper usage:\n\n"
		<< "To configure:\n s3-sync configure\n"
		<< "To upload:\n s3-sync put <SOURCE/FOLDER> <DESTINATION_BUCKET>\n"
		<< "To download:\n s3-sync get <SOURCE_BUCKET> <DESTINATION_FOLDER>\n"
		<< "To list buckets:\n s3-sync list -b\n"
		<< "To list objects:\n s3-sync list -o <SOURCE_BUCKET>\n"
		<< "To wipe a bucket:\n s3-sync delete <DESTINATION_BUCKET>\n"
		<< "To view this menu again:\n s3-sync help\n";
}

void CLI::HelpMenu()
{
	std::cerr << "Documentation:\n";
	std::cout
		<< "s3-sync usage:\n\n"
		<< "To configure:\n s3-sync configure\n"
		<< "To upload:\n s3-sync put <SOURCE/FOLDER> <DESTINATION_BUCKET>\n"
		<< "To download:\n s3-sync get <SOURCE_BUCKET> <DESTINATION_FOLDER>\n"
		<< "To list buckets:\n s3-sync list -b\n"
		<< "To list objects:\n s3-sync list -o <SOURCE_BUCKET>\n"
		<< "To wipe a bucket:\n s3-sync delete <DESTINATION_BUCKET>\n"
		<< "To view this menu again:\n s3-sync help\n";
}

std::expected<std::vector<std::string>, Error::ErrorCode> CLI::CheckConfigVector()
{
	auto parsedConfig{ ReadConfigFile() };
	if (!parsedConfig.has_value()) {
		std::cerr << "No config file found: " << Error::ErrorParser(parsedConfig.error()) << "\n";
		std::cout << "To set up, please use: s3-sync configure\n";
		return std::unexpected(parsedConfig.error());
	}
	if (parsedConfig.value().size() != 4) {
		std::cerr << Error::ErrorParser(Error::ErrorCode::CorruptConfigFile) << "\n";
		std::cout << "To reconfigure, please use: s3-sync configure\n";
		return std::unexpected(Error::ErrorCode::CorruptConfigFile);
	}
	if (parsedConfig.value()[0] != "valid") {
		std::cerr << Error::ErrorParser(Error::ErrorCode::CorruptConfigFile) << "\n";
		std::cout << "To reconfigure, please use: s3-sync configure\n";
		return std::unexpected(Error::ErrorCode::CorruptConfigFile);
	}

	return parsedConfig.value();
}

bool CLI::CheckArgCount(int argc)
{
	if (this->argc != argc) {
		InvalidArguments();
		return false;
	}

	return true;
}
