#include "Error.h"

std::string_view Error::ErrorParser(Error::ErrorCode code) {
    switch (code) {
    case Error::ErrorCode::NoUserConfiguration:
        return "No user configuration found.";
        break;
    case Error::ErrorCode::DownloadFailed:
        return "Failed to download object.";
        break;
    case Error::ErrorCode::RetrieveFailed:
        return "Failed to retrieve data from S3.";
        break;
    case Error::ErrorCode::NoBuckets:
        return "No buckets found.";
        break;
    case Error::ErrorCode::NoFilePath:
        return "Invalid file path.";
        break;
    case Error::ErrorCode::OpenFileFailed:
        return "Failed to open file.";
        break;
    case Error::ErrorCode::UploadFailed:
        return "Failed to upload object.";
        break;
    case Error::ErrorCode::NoObjects:
        return "No objects found in bucket.";
        break;
    case Error::ErrorCode::FileSystemError:
        return "Filesystem error occurred.";
        break;
    case Error::ErrorCode::FailedToOpenFile:
        return "Failed to open file (filesystem).";
        break;
    case Error::ErrorCode::NoConfigFile:
        return "No configuration file found.";
        break;
    case Error::ErrorCode::CorruptConfigFile:
        return "Corrupt configuration file.";
        break;
    default:
        return "Unknown error.";
        break;
    }
}