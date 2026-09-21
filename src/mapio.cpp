#include <fstream>
#include <filesystem>

#include "map.h"
#include "mapio.h"
#include "engine.h"    // assignProductionRates: a loaded map gets its rates rebuilt, never read from the file

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
            // peek(), NOT map(lat,lon): operator() adds the viewing faction's map offset
            // (shifted with 'f'/'g'), so a map saved while the view was scrolled was written
            // out rotated by that offset while the lat/lon LABELS stayed unshifted -- and
            // loadMap below reads those labels straight into map.set(), which has no offset.
            // Reloading then put the terrain back rotated under units and cities, which are
            // saved at their true coordinates. Save and load must both be offset-free.
            auto &cell = map.peek(lat, lon);
            out.write(reinterpret_cast<const char*>(&lat), sizeof(lat));
            out.write(reinterpret_cast<const char*>(&lon), sizeof(lon));
            out.write(reinterpret_cast<const char*>(&cell.code), sizeof(cell.code));
            out.write(reinterpret_cast<const char*>(&cell.bioma), sizeof(cell.bioma));
            out.write(reinterpret_cast<const char*>(&cell.resource), sizeof(cell.resource));
            // Production rates are NOT saved. Every tile keeps its own (mapcell), but they
            // are REBUILT from the code/bioma/resource written above plus the productionrates
            // tables, by assignProductionRates() -- on a fresh world and again at the end of
            // loadMap(). Saving them stored the same fact twice, and stored it wrong: this
            // wrote getResourceProductionRate(i), which has the improvement factor already
            // applied, where loadMap read the BASE. Any load that did not re-assign
            // afterwards therefore took an inflated base and applied the bonus again, and
            // saving compounded it (an irrigated river tile 2 -> 4 -> 8 -> 16).
            //
            // The count stays in the format, always 0, because the block is self-describing
            // (count, then that many ints) and the map file carries no version header: a
            // pre-change file still says 6 and its stale rates are read and discarded below,
            // so old saves keep loading. Drop the field when the format gains a version.
            const size_t sz = 0;
            out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
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

            // Read and DISCARD whatever rates the file carries (see saveMap): a file written
            // before this change holds improvement-inflated values, and assignProductionRates()
            // at the end of this function rebuilds every tile's rates anyway. Files written
            // now store a count of 0 and nothing follows it.
            size_t sz = 0;
            in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
            for (size_t i = 0; i < sz; ++i) {
                int staleRate = 0;
                in.read(reinterpret_cast<char*>(&staleRate), sizeof(staleRate));
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

    // The rates are not in the file (see above), so give every tile its own from the tables
    // now that code/bioma/resource are in place. Here rather than at the call sites so no
    // loader can forget: initMap() used to be the only thing that re-assigned, which is the
    // single reason the stored inflated values never corrupted a live game.
    assignProductionRates(map);

    printf("Map loaded from %s\n", path.c_str());
}
