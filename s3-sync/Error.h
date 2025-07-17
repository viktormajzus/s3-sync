#pragma once
#include <string_view>

namespace Error {
	enum class ErrorCode {
		NoUserConfiguration,
		DownloadFailed,
		RetrieveFailed,
		NoBuckets,
		NoFilePath,
		OpenFileFailed,
		UploadFailed,
		NoObjects,
		FileSystemError,
		FailedToOpenFile,
		NoConfigFile,
        CorruptConfigFile
	};

	std::string_view ErrorParser(Error::ErrorCode code);
}