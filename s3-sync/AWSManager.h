#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/s3/S3Client.h"

#include <optional>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

class AWSManager {
private:
	Aws::SDKOptions options;
	std::optional<Aws::Client::ClientConfiguration> config;
	std::optional<Aws::Auth::AWSCredentials> credentials;
	std::unique_ptr<Aws::S3::S3Client> client;


public:
	AWSManager(std::string_view accessKey, std::string_view secretKey, std::string_view region);
	~AWSManager();
	
	std::optional<std::vector<std::string>> GetBucketNames();
	void ListBuckets();
	void put(std::string_view srcFilePath, std::string_view dstBucket);

	Aws::S3::S3Client& GetClient();

private:
	// Helper func
	std::vector<std::string> GetFilePaths(std::string_view rootPath);
	std::string NormalizePathForS3(const std::filesystem::path& path);
};