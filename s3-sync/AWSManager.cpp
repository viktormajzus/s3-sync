#include "AWSManager.h"

#include <fstream>
#include <iosfwd>
#include <iostream>
#include <chrono>

#include <future>
#include <vector>
#include <algorithm>
#include <semaphore>

#include <unordered_map>

#include <aws/s3/model/ListBucketsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/DateTime.h>
#include <aws/s3/model/DeleteObjectRequest.h>

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

std::expected<std::vector<std::string>, Error::ErrorCode> AWSManager::GetBucketNames()
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
		return std::unexpected(Error::ErrorCode::RetrieveFailed);
}

std::expected<void, Error::ErrorCode> AWSManager::ListBuckets()
{
	auto buckets{ this->GetBucketNames() };
	if (buckets)
	{
		std::cout << "Current Buckets:\n";
		for (const auto& bucket : buckets.value())
			std::cout << " - " << bucket << "\n";
		return{};
	}
	
	return std::unexpected(Error::ErrorCode::NoBuckets);
}

std::expected<std::vector<std::string>, Error::ErrorCode> AWSManager::GetFilePaths(std::string_view rootPath)
{
	namespace fs = std::filesystem;
	std::vector<std::string> files;

	fs::path root = rootPath;

	if (!fs::exists(root) || !fs::is_directory(root))
		return std::unexpected(Error::ErrorCode::NoFilePath);

	for (const auto& entry : fs::recursive_directory_iterator(root)) {
		if (fs::is_regular_file(entry)) {
			files.emplace_back(entry.path().string());
		}
	}

	return files;
}

std::expected<int, Error::ErrorCode> AWSManager::put(std::string_view srcFilePath, std::string_view dstBucket)
{
	namespace fs = std::filesystem;
	constexpr int maxThreads{ 8 };

	auto files{ GetRelativeFilePaths(srcFilePath) };
	auto tempFilePath{ NormalizePathForS3(srcFilePath) };

	std::counting_semaphore<maxThreads> sem(maxThreads);
	std::mutex coutMutex{};
	std::vector<std::future<void>> futures;

	auto objects{ GetObjects(dstBucket) };
	if (!objects) {
		if (objects.error() != Error::ErrorCode::NoObjects)
			return std::unexpected(objects.error());
	}

	std::unordered_map<std::string, std::time_t> remoteTimestamps{};

	if (objects.has_value()) {
		for (const auto& object : objects.value())
			remoteTimestamps[object.GetKey()] = object.GetLastModified().Seconds();
	}

	int fileCount{ 0 };

	for (const auto& file : files) {
		sem.acquire();
		futures.push_back(std::async(std::launch::async, [&, file]() {
			fs::path path = srcFilePath;
			path /= file;
			std::string key = file;
			/*
			size_t pos = key.find(tempFilePath);
			if (pos != std::string::npos)
				key.erase(pos, tempFilePath.length());
			if (!key.empty() && key.at(0) == '/')
				key.erase(0, 1);
				*/

			std::time_t localTime = std::chrono::system_clock::to_time_t(
				std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					fs::last_write_time(path) - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
				)
			);

			if (remoteTimestamps.contains(key) && localTime <= remoteTimestamps[key]) {
				{
					std::lock_guard<std::mutex> lock(coutMutex);
				}
				sem.release();
				return;
			}

			Aws::S3::Model::PutObjectRequest request;
			request.SetBucket(dstBucket);
			request.SetKey(key);

			std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
				key.c_str(), path.c_str(), std::ios_base::in | std::ios_base::binary
			);

			if (!inputData || !inputData->good()) {
				std::lock_guard<std::mutex> lock(coutMutex);
				std::cerr << "[!] Failed to open file: " << file << "\n";
				sem.release();
				return; // OpenFileFailed
			}

			request.SetBody(inputData);

			auto outcome = GetClient().PutObject(request);
			if (!outcome.IsSuccess()) {
				std::lock_guard<std::mutex> lock(coutMutex);
				std::cerr << "[!] Failed to upload: " << key << "\n";
				// UploadFailed
			}
			else {
				std::lock_guard<std::mutex> lock(coutMutex);
				++fileCount;
			}

			sem.release();
		}));
	}

	for (auto& f : futures)
		f.wait();

	return fileCount;
}

Aws::S3::S3Client& AWSManager::GetClient()
{
	return *client;
}

std::expected<std::vector<Aws::S3::Model::Object>, Error::ErrorCode> AWSManager::GetObjects(std::string_view bucketName)
{
	Aws::S3::Model::ListObjectsV2Request request{};
	request.WithBucket(bucketName);

	Aws::String continuationToken{};
	std::vector<Aws::S3::Model::Object> objects{};

	do
	{
		if (!continuationToken.empty())
			request.SetContinuationToken(continuationToken);

		auto outcome{ GetClient().ListObjectsV2(request) };
		if (outcome.IsSuccess()) {
			std::vector<Aws::S3::Model::Object> tempObjects{ outcome.GetResult().GetContents() };

			objects.insert(objects.end(), tempObjects.begin(), tempObjects.end());
			continuationToken = outcome.GetResult().GetNextContinuationToken();
		}
		else
			return std::unexpected(Error::ErrorCode::RetrieveFailed);

	} while (!continuationToken.empty());

	return objects;
}

std::expected<void, Error::ErrorCode> AWSManager::ListObjects(std::string_view bucketName)
{
	auto objects{ GetObjects(bucketName) };
	if (!objects.has_value())
		return std::unexpected(Error::ErrorCode::NoObjects);

	std::cout << bucketName << " Objects:\n";
	for (const auto& object : objects.value()) {
		std::cout << " - " << object.GetKey() << "\n";
		std::cout << "   " << "Last modified: " << object.GetLastModified().ToGmtString(Aws::Utils::DateFormat::ISO_8601) << "\n";
	}

	return{};
}

std::expected<int, Error::ErrorCode> AWSManager::get(std::string_view srcBucket, std::string_view dstPath)
{
	namespace fs = std::filesystem;
	constexpr int maxThreads{ 8 };

	fs::path p{ dstPath };
	auto objects{ GetObjects(srcBucket) };
	if (!objects.has_value())
		return std::unexpected(Error::ErrorCode::NoObjects);

	std::counting_semaphore<maxThreads> sem(maxThreads);
	std::mutex coutMutex{};
	std::vector<std::future<void>> futures{};

	int fileCount{ 0 };

	for (const auto& object : objects.value()) {
		sem.acquire();

		futures.push_back(std::async(std::launch::async, [&, object]() {
			fs::path tempPath = p / object.GetKey();

			bool shouldDownload{ true };

			if (fs::exists(tempPath) && fs::is_regular_file(tempPath)) {
				auto ftime{ fs::last_write_time(tempPath) };
				auto sctp{ std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
				) };
				std::time_t fileUnixTime{ std::chrono::system_clock::to_time_t(sctp) };
				std::time_t objectUnixTime{ object.GetLastModified().Seconds() };

				if (fileUnixTime > objectUnixTime)
					shouldDownload = false;
			}

			if (shouldDownload) {
				auto result{ DownloadObjectToPath(
					srcBucket, p, object.GetKey()
				) };

				if (!result) {
					std::lock_guard<std::mutex> lock(coutMutex);
					std::cout << "[!] Failed to download " << srcBucket << "/" << object.GetKey() << "\n";
					// DownloadFailed
				}
				else 
				{
					std::lock_guard<std::mutex> lock(coutMutex);
					++fileCount;
				}
			}

			sem.release();
		}));
	}

	for (auto& f : futures)
		f.wait();

	return fileCount;
}

std::expected<std::pair<int, int>, Error::ErrorCode> AWSManager::DeleteAllObjects(std::string_view bucketName)
{
	auto objects{ GetObjects(bucketName) };
	if (!objects.has_value())
		return std::unexpected(Error::ErrorCode::RetrieveFailed);
	if (objects.value().empty())
		return std::unexpected(Error::ErrorCode::NoObjects);

	int deletedObjects{ 0 };

	for (const auto& object : objects.value()) {
		Aws::S3::Model::DeleteObjectRequest request{};
		request.WithKey(object.GetKey()).WithBucket(bucketName);

		auto outcome{ GetClient().DeleteObject(request) };
		if (outcome.IsSuccess()) {
			++deletedObjects;
		}
	}

	return std::make_pair(deletedObjects, objects.value().size());
}

std::vector<std::string> AWSManager::GetRelativeFilePaths(std::string_view rootPath)
{
	std::vector<std::string> temp{};

	namespace fs = std::filesystem;
	for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
		if (entry.is_regular_file())
			temp.push_back(NormalizePathForS3(std::filesystem::relative(entry.path(), rootPath)));
	}

	return temp;
}

std::string AWSManager::NormalizePathForS3(const std::filesystem::path& path)
{
	return path.generic_string();
}

std::expected<void, Error::ErrorCode> AWSManager::DownloadObjectToPath(std::string_view srcBucket, const std::filesystem::path& dstPath, std::string_view objectKey)
{
	std::filesystem::path tempPath{ dstPath };
	tempPath /= objectKey;

	Aws::S3::Model::GetObjectRequest request{};
	request.SetBucket(srcBucket);
	request.SetKey(objectKey);

	auto outcome{ GetClient().GetObject(request) };
	if (!outcome.IsSuccess())
		return std::unexpected(Error::ErrorCode::RetrieveFailed);

	std::filesystem::create_directories(tempPath.parent_path());

	auto& s3Stream{ outcome.GetResult().GetBody() };
	std::ofstream outputFile(tempPath, std::ios::binary);
	if (!outputFile.is_open())
		return std::unexpected(Error::ErrorCode::FileSystemError);

	constexpr size_t bufferSize{ 8192 };
	char buffer[bufferSize];

	while (s3Stream.good()) {
		s3Stream.read(buffer, bufferSize);
		std::streamsize bytesRead = s3Stream.gcount();
		outputFile.write(buffer, bytesRead);
	}

	return{};
}
