# s3-sync

A basic tool to sync up with an S3 bucket.

## Build Instructions

### Clone vcpkg into any desired folder (in my case I just put it into Users)
```
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

### Install AWS SDK with vcpkg
```
.\vcpkg install aws-sdk-cpp[s3]:x64-windows
```

### Clone this repository anywhere
```
git clone https://github.com/viktormajzus/s3-sync
cd s3-sync
```

### Edit CMakePresets.json to use your path to vcpkg.cmake
```
{
  "name": "windows-base",
  "hidden": true,
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/out/build/${presetName}",
  "installDir": "${sourceDir}/out/install/${presetName}",
  "toolchainFile": "C:/PATH/TO/vcpkg/scripts/buildsystems/vcpkg.cmake",
  "cacheVariables": {
    "CMAKE_C_COMPILER": "cl.exe",
    "CMAKE_CXX_COMPILER": "cl.exe"
  },
  "condition": {
    "type": "equals",
    "lhs": "${hostSystemName}",
    "rhs": "Windows"
  }
},
```

### Build it and enjoy!

### Recommended:
Add the folder to your Path environment variable:
Windows Key -> Edit the system environment variables -> Environment variables -> Select Path -> Press Edit -> New -> C:\PATH\TO\source\repos\s3-sync\out\build\x64-release\s3-sync folder (not the executable!!)


## Usage Instructions

### Configure your client
`s3-sync configure`

Will open a configuration prompt, where you can type your Access Key, Secret Key, and Region.

**WARNING** This data is stored in an unencrypted format in `%LOCALAPPDATA%/s3-sync/config.cfg`

### Upload Files
`s3-sync put <SOURCE/FOLDER> <DESTINATION_BUCKET>`

Will upload every single file within this folder into the destination bucket.

**WARNING** Overwrites files based on their date modified

### Download Files
`s3-sync get <SOURCE_BUCKET> <DESTINATION_FOLDER>`

Downloads files from the source bucket to the destination folder.

**WARNING** Overwrites files based on their date modified

### List buckets
`s3-sync list -b`

Lists every bucket that the user (with the access key) has access to see

### List objects
`s3-sync list -o <SOURCE_BUCKET>`

Lists every object within a specified bucket

### Wipe a bucket
`s3-sync delete <DESTINATION_BUCKET>`

Deletes every object in a specified bucket.

**WARNING** Dangerous command, try not to use it, unless you know what you're doing

### Help menu
`s3-sync help`

Opens a help menu
