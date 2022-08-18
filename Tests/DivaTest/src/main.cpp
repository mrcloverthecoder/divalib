#include <io_core.h>
#include <diva_db.h>

const char* SprDbFilename = "C:\\Eduardo\\Programas\\Steam\\steamapps\\common\\Hatsune Miku Project DIVA Mega Mix Plus\\mods\\Pop Culture\\rom\\2d\\mod_spr_db.bin";
const char* OutputFilename = "C:\\Development\\test.farc";
std::string data = "this is a very long and also huge string to prevent small string optimization";

int main(int argc, char** argv)
{
    IO::File f = IO::File::Open(SprDbFilename, IO::StreamMode::Read);
    Database::SpriteDatabase spr;
    spr.Parse(f);
    f.Close();

    int idx = 0;
    for (const Database::SpriteSetInfo& info : spr.Sets)
    {
        printf("Sprite set #%d:\n", idx++);
        printf("\tId: %u\n", info.Id);
        printf("\tName: %s\n", info.Name.c_str());
        printf("\tFile: %s\n", info.Filename.c_str());
        printf("\tIndex: %d\n", info.Index);
    }

    return 0;
}