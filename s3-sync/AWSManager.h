#pragma once
#include "Error.h"

#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/s3/S3Client.h"

#include <expected>

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
	
	std::expected<std::vector<std::string>, Error::ErrorCode> GetBucketNames();
	std::expected<void, Error::ErrorCode> ListBuckets();
	std::expected<int, Error::ErrorCode> put(std::string_view srcFilePath, std::string_view dstBucket);

	Aws::S3::S3Client& GetClient();

	std::expected<std::vector<Aws::S3::Model::Object>, Error::ErrorCode> GetObjects(std::string_view bucketName);
	std::expected<void, Error::ErrorCode> ListObjects(std::string_view bucketName);
	std::expected<int, Error::ErrorCode> get(std::string_view srcBucket, std::string_view dstPath);

private:
	// Helper func
	std::expected<std::vector<std::string>, Error::ErrorCode> GetFilePaths(std::string_view rootPath);
	std::string NormalizePathForS3(const std::filesystem::path& path);
	std::expected<void, Error::ErrorCode> DownloadObjectToPath(std::string_view srcBucket, const std::filesystem::path& dstPath, std::string_view objectKey);
	std::vector<std::string> GetRelativeFilePaths(std::string_view rootPath);
};