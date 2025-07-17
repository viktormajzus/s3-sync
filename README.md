#s3-sync

A basic tool to sync up with an S3 bucket.

##Build Instructions

###Clone vcpkg into any desired folder (in my case I just put it into Users)
```
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

###Install AWS SDK with vcpkg
```
.\vcpkg install aws-sdk-cpp[s3]:x64-windows
```

###Clone this repository anywhere
```
git clone https://github.com/viktormajzus/s3-sync
cd s3-sync
```

###Edit CMakePresets.json to use your path to vcpkg.cmake
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

###Build it and enjoy!

###Recommended:
Add the folder to your Path environment variable:
Windows Key -> Edit the system environment variables -> Environment variables -> Select Path -> Press Edit -> New -> C:\PATH\TO\source\repos\s3-sync\out\build\x64-release\s3-sync folder (not the executable!!)
