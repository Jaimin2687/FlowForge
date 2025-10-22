#include "../src/IAction.h"
#include "../src/PathUtils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <zlib.h>
#include <algorithm>
#include <locale>

using namespace std;
namespace fs = std::filesystem;

// ZIP file structures (simplified implementation)
#pragma pack(push, 1)
struct LocalFileHeader {
    uint32_t signature = 0x04034b50;
    uint16_t version = 20;
    uint16_t flags = 0;
    uint16_t compression = 8; // DEFLATE
    uint16_t mod_time = 0;
    uint16_t mod_date = 0;
    uint32_t crc32 = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint16_t filename_length = 0;
    uint16_t extra_length = 0;
};

struct CentralDirectoryHeader {
    uint32_t signature = 0x02014b50;
    uint16_t version_made = 20;
    uint16_t version_needed = 20;
    uint16_t flags = 0;
    uint16_t compression = 8;
    uint16_t mod_time = 0;
    uint16_t mod_date = 0;
    uint32_t crc32 = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint16_t filename_length = 0;
    uint16_t extra_length = 0;
    uint16_t comment_length = 0;
    uint16_t disk_start = 0;
    uint16_t internal_attr = 0;
    uint32_t external_attr = 0;
    uint32_t local_header_offset = 0;
};

struct EndOfCentralDirectory {
    uint32_t signature = 0x06054b50;
    uint16_t disk_number = 0;
    uint16_t central_dir_disk = 0;
    uint16_t num_entries_disk = 0;
    uint16_t num_entries_total = 0;
    uint32_t central_dir_size = 0;
    uint32_t central_dir_offset = 0;
    uint16_t comment_length = 0;
};
#pragma pack(pop)

class CompressAction : public IAction {
private:
    // Convert DOS time/date format
    uint16_t dos_time(time_t t) {
        tm* tm = localtime(&t);
        return (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2);
    }

    uint16_t dos_date(time_t t) {
        tm* tm = localtime(&t);
        return ((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday;
    }

    // Compress data using zlib
    vector<uint8_t> compressData(const vector<uint8_t>& data) {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            throw runtime_error("Failed to initialize zlib deflate");
        }

        zs.next_in = (Bytef*)data.data();
        zs.avail_in = data.size();

        vector<uint8_t> compressed;
        compressed.reserve(data.size());

        int ret;
        uint8_t buffer[8192];
        do {
            zs.next_out = buffer;
            zs.avail_out = sizeof(buffer);

            ret = deflate(&zs, Z_FINISH);

            if (compressed.size() + (sizeof(buffer) - zs.avail_out) > compressed.capacity()) {
                compressed.reserve(compressed.capacity() * 2);
            }

            compressed.insert(compressed.end(), buffer, buffer + (sizeof(buffer) - zs.avail_out));
        } while (ret == Z_OK);

        deflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            throw runtime_error("Compression failed");
        }

        return compressed;
    }

    // Calculate CRC32
    uint32_t calculate_crc32(const vector<uint8_t>& data) {
        uint32_t crc = 0xFFFFFFFF;
        for (uint8_t byte : data) {
            crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }

    // CRC32 lookup table
    static const uint32_t crc32_table[256];

    // Add file to ZIP
    void addFileToZip(ofstream& zip_file, const fs::path& file_path, const string& entry_name,
                     vector<CentralDirectoryHeader>& central_headers, vector<string>& filenames, uint32_t& local_header_offset) {
        // Read file data
        ifstream file(file_path, ios::binary);
        if (!file) {
            throw runtime_error("Cannot open file: " + file_path.string());
        }

        vector<uint8_t> file_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        // Compress data
        vector<uint8_t> compressed_data = compressData(file_data);

        // Get file times
        auto file_time = fs::last_write_time(file_path);
        time_t t = chrono::system_clock::to_time_t(
            chrono::time_point_cast<chrono::system_clock::duration>(
                file_time - fs::file_time_type::clock::now() + chrono::system_clock::now()
            )
        );

        // Create local file header
        LocalFileHeader local_header;
        local_header.mod_time = dos_time(t);
        local_header.mod_date = dos_date(t);
        local_header.crc32 = calculate_crc32(file_data);
        local_header.compressed_size = compressed_data.size();
        local_header.uncompressed_size = file_data.size();
        local_header.filename_length = entry_name.size();

        // Write local file header
        zip_file.write(reinterpret_cast<char*>(&local_header), sizeof(local_header));
        zip_file.write(entry_name.c_str(), entry_name.size());

        // Write compressed data
        zip_file.write(reinterpret_cast<char*>(compressed_data.data()), compressed_data.size());

        // Create central directory header
        CentralDirectoryHeader central_header;
        central_header.mod_time = local_header.mod_time;
        central_header.mod_date = local_header.mod_date;
        central_header.crc32 = local_header.crc32;
        central_header.compressed_size = local_header.compressed_size;
        central_header.uncompressed_size = local_header.uncompressed_size;
        central_header.filename_length = local_header.filename_length;
        central_header.local_header_offset = local_header_offset;

        central_headers.push_back(central_header);
        filenames.push_back(entry_name);

        // Update offset for next file
        local_header_offset += sizeof(LocalFileHeader) + entry_name.size() + compressed_data.size();
    }

    // Create ZIP file from directory or single file
    bool createZipFile(const fs::path& source_path, const fs::path& zip_path) {
        ofstream zip_file(zip_path, ios::binary);
        if (!zip_file) {
            cerr << "CompressAction: Cannot create ZIP file: " << zip_path.string() << endl;
            return false;
        }

        vector<CentralDirectoryHeader> central_headers;
        vector<string> filenames;
        uint32_t local_header_offset = 0;

        try {
            if (fs::is_directory(source_path)) {
                // Add all files in directory recursively
                for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
                    if (entry.is_regular_file()) {
                        string relative_path = fs::relative(entry.path(), source_path.parent_path()).string();
                        // Normalize path separators to forward slashes for ZIP
                        replace(relative_path.begin(), relative_path.end(), '\\', '/');
                        addFileToZip(zip_file, entry.path(), relative_path, central_headers, filenames, local_header_offset);
                    }
                }
            } else if (fs::is_regular_file(source_path)) {
                // Add single file
                string filename = source_path.filename().string();
                addFileToZip(zip_file, source_path, filename, central_headers, filenames, local_header_offset);
            } else {
                throw runtime_error("Source is neither a file nor a directory");
            }

            // Write central directory
            uint32_t central_dir_offset = local_header_offset;
            size_t filename_index = 0;
            for (const auto& header : central_headers) {
                zip_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
                // Write filename after header
                if (filename_index < filenames.size()) {
                    zip_file.write(filenames[filename_index].c_str(), filenames[filename_index].size());
                }
                filename_index++;
            }

            // Write end of central directory
            EndOfCentralDirectory eocd;
            eocd.num_entries_disk = central_headers.size();
            eocd.num_entries_total = central_headers.size();
            eocd.central_dir_size = central_headers.size() * sizeof(CentralDirectoryHeader);
            eocd.central_dir_offset = central_dir_offset;

            zip_file.write(reinterpret_cast<char*>(&eocd), sizeof(eocd));

            zip_file.close();
            return true;

        } catch (const exception& e) {
            zip_file.close();
            fs::remove(zip_path); // Clean up partial file
            throw;
        }
    }

public:
    void execute(const string& params) override {
        try {
            string expanded = PathUtils::expandAndNormalizePath(params);
            cout << "CompressAction: Compressing " << expanded << endl;

            // Create backup directory if it doesn't exist
            fs::path backups_dir = fs::current_path().parent_path() / "data" / "backups";
            fs::create_directories(backups_dir);

            // Get source path
            fs::path src = fs::u8path(expanded);
            if (!fs::exists(src)) {
                cerr << "CompressAction: Source does not exist: " << expanded << endl;
                return;
            }

            // Create a timestamped zip filename
            auto t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);
        std::ostringstream ss;
        ss.imbue(std::locale("C"));
        ss << std::put_time(&tm, "%Y%m%d%H%M%S");
        string timestamp = ss.str();

            string base_name = src.filename().string();
            string zip_name = base_name + "_" + timestamp + ".zip";
            fs::path zip_path = backups_dir / zip_name;

            // Create the ZIP file
            if (createZipFile(src, zip_path)) {
                cout << "CompressAction: Successfully created ZIP file: " << zip_path.string() << endl;
            } else {
                cerr << "CompressAction: Failed to create ZIP file" << endl;
            }
        } catch (const exception& e) {
            cerr << "CompressAction error: " << e.what() << endl;
        }
    }
};

// CRC32 lookup table implementation
const uint32_t CompressAction::crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

extern "C" IAction* create_action() {
    return new CompressAction();
}
