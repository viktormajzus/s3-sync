#include "AWSManager.h"
#include <fstream>
#include <iostream>

#include <aws/s3/model/ListBucketsRequest.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/s3/model/PutObjectRequest.h>

AWSManager::AWSManager(std::string_view accessKey, std::string_view secretKey, std::string_view region)
{
	options = Aws::SDKOptions{};
	Aws::InitAPI(options);

	credentials.emplace(accessKey.data(), secretKey.data());
	config.emplace();
	config->region = region;
	client = std::make_unique<Aws::S3::S3Client>(
		*credentials,
		*config,
		Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::RequestDependent,
		true
	);
}

AWSManager::~AWSManager()
{
	client.reset();
	Aws::ShutdownAPI(this->options);
}

std::optional<std::vector<std::string>> AWSManager::GetBucketNames()
{
	std::vector<std::string> temp{};

	auto outcome{ GetClient().ListBuckets()};

	if (outcome.IsSuccess()) {
		temp.reserve(outcome.GetResult().GetBuckets().size());

		const auto& buckets = outcome.GetResult().GetBuckets();
		for (const auto& bucket : buckets)
			temp.push_back(bucket.GetName());

		return temp;
	}
	else
		return std::nullopt;
}

void AWSManager::ListBuckets()
{
	auto buckets{ this->GetBucketNames() };
	if (buckets)
	{
		std::cout << "Current Buckets:\n";
		for (const auto& bucket : buckets.value())
			std::cout << " - " << bucket << "\n";
	}
}

void AWSManager::put(std::string_view srcFilePath, std::string_view dstBucket)
{
	auto files{ this->GetFilePaths(srcFilePath) };

	auto tempFilePath{ NormalizePathForS3(srcFilePath) };

	for (const auto& file : files)
	{
		std::string key{ file };
		size_t pos = file.find(tempFilePath);

		if (pos != std::string::npos)
			key.erase(pos, tempFilePath.length());
		if (key.at(0) == '/')
			key.erase(0, 1);

		Aws::S3::Model::PutObjectRequest request{};
		request.SetBucket(dstBucket);
		request.SetKey(key);

		std::shared_ptr<Aws::IOStream> inputData =
			Aws::MakeShared<Aws::FStream>(key.c_str(),
				file.c_str(),
				std::ios_base::in | std::ios_base::binary);

		if (!inputData) {
			std::cerr << "Failed to open file: " << file << "\n";
			continue;
		}

		request.SetBody(inputData);

		auto outcome{ GetClient().PutObject(request)};
		if (!outcome.IsSuccess())
			std::cerr << "Failed to upload file: " << file << "\n";
	}
}

Aws::S3::S3Client& AWSManager::GetClient()
{
	return *client;
}

std::vector<std::string> AWSManager::GetFilePaths(std::string_view rootPath)
{
	std::vector<std::string> temp{};

	namespace fs = std::filesystem;
	for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
		if (entry.is_regular_file())
			temp.push_back(NormalizePathForS3(entry.path()));
	}

	return temp;
}

std::string AWSManager::NormalizePathForS3(const std::filesystem::path& path)
{
	return path.generic_string();
}
