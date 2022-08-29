#include <core_io.h>
#include <diva_auth2d.h>
#include <diva_archive.h>

const char* AetFilename = "C:\\Development\\aet_gam_pv637.bin";
const char FileData[1672 * 1024] = { 0xCC };

int main(int argc, char** argv)
{
    IO::Reader reader;
    reader.FromFile(AetFilename);
    Aet::AetSet set = { };
    set.Parse(reader);

    Archive::FArcPacker packer = { };
    Archive::FArcPacker::FArcFile file = { std::string("file.bin"), 1672 * 1024, &FileData[0] };
    packer.AddFile(file);

    packer.Flush("C:\\Development\\new_test.farc", false);

    return 0;
}