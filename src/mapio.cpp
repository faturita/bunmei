#include <fstream>
#include <filesystem>

#include "map.h"
#include "mapio.h"

extern Map map;

void saveMap(const std::string &path)
{
    std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path());

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        printf("Error opening file for writing.\n");
        return;
    }

    for (int lat = map.minlat; lat < map.maxlat; lat++) {
        for (int lon = map.minlon; lon < map.maxlon; lon++) {
            auto &cell = map(lat, lon);
            out.write(reinterpret_cast<const char*>(&lat), sizeof(lat));
            out.write(reinterpret_cast<const char*>(&lon), sizeof(lon));
            out.write(reinterpret_cast<const char*>(&cell.code), sizeof(cell.code));
            out.write(reinterpret_cast<const char*>(&cell.bioma), sizeof(cell.bioma));
            out.write(reinterpret_cast<const char*>(&cell.resource), sizeof(cell.resource));
            // Save resource production rates using mapcell accessors.
            size_t sz = static_cast<size_t>(cell.getResourceProductionRateSize());
            out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (size_t i = 0; i < sz; ++i) {
                int rate = cell.getResourceProductionRate(static_cast<int>(i));
                out.write(reinterpret_cast<const char*>(&rate), sizeof(rate));
            }
            // Save owner if present
            out.write(reinterpret_cast<const char*>(&cell.c_id_owner), sizeof(cell.c_id_owner));
            out.write(reinterpret_cast<const char*>(&cell.f_id_owner), sizeof(cell.f_id_owner));
            out.write(reinterpret_cast<const char*>(&cell.owners), sizeof(cell.owners));
            // Save improvements bitmap (road/irrigation/mine/etc).
            out.write(reinterpret_cast<const char*>(&cell.improvements), sizeof(cell.improvements));
            // Save per-faction fog of war. vector<bool> is bit-packed, not a flat buffer, so
            // each entry is written as one byte rather than memcpy'd as a block.
            size_t vsz = cell.visible.size();
            out.write(reinterpret_cast<const char*>(&vsz), sizeof(vsz));
            for (size_t i = 0; i < vsz; ++i) {
                char v = cell.visible[i] ? 1 : 0;
                out.write(&v, sizeof(v));
            }
        }
    }
    out.close();
    printf("Map saved to %s\n", path.c_str());
}

void loadMap(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        printf("Error opening file for reading.\n");
        return;
    }

    for (int lat = map.minlat; lat < map.maxlat; lat++) {
        for (int lon = map.minlon; lon < map.maxlon; lon++) {
            int file_lat, file_lon;
            in.read(reinterpret_cast<char*>(&file_lat), sizeof(file_lat));
            in.read(reinterpret_cast<char*>(&file_lon), sizeof(file_lon));
            if (!in) break;

            auto &cell = map.set(file_lat, file_lon);

            int code;
            int bioma;
            int resource;
            in.read(reinterpret_cast<char*>(&code), sizeof(code));
            in.read(reinterpret_cast<char*>(&bioma), sizeof(bioma));
            in.read(reinterpret_cast<char*>(&resource), sizeof(resource));

            cell = mapcell(code);
            cell.bioma = bioma;
            cell.resource = resource;

            // Load resource production rates using mapcell accessors.
            size_t sz = 0;
            in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
            for (size_t i = 0; i < sz; ++i) {
                int rate = 0;
                in.read(reinterpret_cast<char*>(&rate), sizeof(rate));
                cell.addResourceProductionRate(rate);
            }
            // Load owner if present
            in.read(reinterpret_cast<char*>(&cell.c_id_owner), sizeof(cell.c_id_owner));
            in.read(reinterpret_cast<char*>(&cell.f_id_owner), sizeof(cell.f_id_owner));
            in.read(reinterpret_cast<char*>(&cell.owners), sizeof(cell.owners));
            // Load improvements bitmap.
            in.read(reinterpret_cast<char*>(&cell.improvements), sizeof(cell.improvements));
            // Load per-faction fog of war (see saveMap's per-byte encoding above).
            size_t vsz = 0;
            in.read(reinterpret_cast<char*>(&vsz), sizeof(vsz));
            cell.visible.resize(vsz);
            for (size_t i = 0; i < vsz; ++i) {
                char v = 0;
                in.read(&v, sizeof(v));
                cell.visible[i] = (v != 0);
            }
        }
    }
    in.close();
    printf("Map loaded from %s\n", path.c_str());
}
