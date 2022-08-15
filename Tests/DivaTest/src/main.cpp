#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string_view>
#include <io_core.h>
#include <io_archive.h>

const char* OutputFilename = "C:\\Development\\test.farc";
std::string data = "this is a very long and also huge string to prevent small string optimization";

int main(int argc, char** argv)
{
    if (argc < 2)
        return -1;
    
    std::string path = std::string(argv[1]);
    std::vector<std::string> files;

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;

        std::string path = entry.path().string();
        files.push_back(path);
    }

    Archive::FArcPacker packer;
    
    for (const std::string& path : files)
    {
        packer.AddFile({
            IO::Path::GetFilename(path),
            data.size(),
            data.data()
        });
    }

    packer.Flush(OutputFilename, false);
    
    return 0;
}