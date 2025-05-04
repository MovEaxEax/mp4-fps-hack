#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <climits>
#include <cmath>
#include <windows.h>



unsigned long long lmd(unsigned long long a, double b)
{
    if (b <= 0.0) return 0;
    long double result = static_cast<long double>(a) * static_cast<long double>(b);
    if (result >= static_cast<long double>(ULLONG_MAX)) return ULLONG_MAX;
    return static_cast<unsigned long long>(std::llround(result));
}

unsigned long long ldd(unsigned long long a, double b)
{
    if (b <= 0.0) return 0;
    long double result = static_cast<long double>(a) / static_cast<long double>(b);
    if (result >= static_cast<long double>(ULLONG_MAX)) return ULLONG_MAX;
    return static_cast<unsigned long long>(std::llround(result));
}

std::string d2sp1(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

std::string d2sp2(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::vector<char> read_mp4(const std::string& file)
{
    std::ifstream in(file, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(in)), {});
    in.close();
    return data;
}

void write_mp4(const std::string& file, std::vector<char>& data)
{
    std::ofstream out(file, std::ios::binary);
    out.write(data.data(), data.size());
    out.close();
}

// MP4 is Big Endian for some reason...
unsigned long long read_be32(const char* ptr)
{
    return
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[0])) << 24) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[1])) << 16) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[2])) << 8) |
        static_cast<unsigned long long>(static_cast<unsigned char>(ptr[3]));
}

unsigned long long read_be64(const char* ptr)
{
    return
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[0])) << 56) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[1])) << 48) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[2])) << 40) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[3])) << 32) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[4])) << 24) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[5])) << 16) |
        (static_cast<unsigned long long>(static_cast<unsigned char>(ptr[6])) << 8) |
        static_cast<unsigned long long>(static_cast<unsigned char>(ptr[7]));
}

void write_be32(char* ptr, unsigned long long value)
{
    ptr[0] = static_cast<char>((value >> 24) & 0xFF);
    ptr[1] = static_cast<char>((value >> 16) & 0xFF);
    ptr[2] = static_cast<char>((value >> 8) & 0xFF);
    ptr[3] = static_cast<char>(value & 0xFF);
}

void write_be64(char* ptr, unsigned long long value)
{
    ptr[0] = static_cast<char>((value >> 56) & 0xFF);
    ptr[1] = static_cast<char>((value >> 48) & 0xFF);
    ptr[2] = static_cast<char>((value >> 40) & 0xFF);
    ptr[3] = static_cast<char>((value >> 32) & 0xFF);
    ptr[4] = static_cast<char>((value >> 24) & 0xFF);
    ptr[5] = static_cast<char>((value >> 16) & 0xFF);
    ptr[6] = static_cast<char>((value >> 8) & 0xFF);
    ptr[7] = static_cast<char>(value & 0xFF);
}

unsigned long long resolve_box_offset(const std::vector<char>& data, const char* box, unsigned long long start, unsigned long long end)
{
    unsigned long long offset = start;
    while (offset + 8 <= end)
    {
        unsigned long long size = read_be32(data.data() + offset);
        if (size < 8 || offset + size > end) break;
        if (std::memcmp(&data[offset + 4], box, 4) == 0)
        {
            return offset;
        }
        offset += size;
    }
    return 0;
}

unsigned long long resolve_atom_offset(const std::vector<char>& data, const char* atom, int channel = 0)
{
    for (unsigned long long i = 0; i < data.size(); i++)
    {
        if (std::memcmp(&data[i], atom, 4) == 0)
        {
            if (channel == 0) return i;
            else channel--;
        }
    }
    return -1;
}



unsigned long long read_mdhd_timescale(const std::vector<char>& data, int channel)
{
    unsigned long long mdhd_index = resolve_atom_offset(data, "mdhd", channel);
    if (mdhd_index == -1) return 0;

    unsigned char version = data[mdhd_index + 4];
    if (version == 0)
    {
        unsigned long long timescale_offset = mdhd_index + 16;
        return read_be32(data.data() + timescale_offset);
    }
    else if (version == 1)
    {
        unsigned long long timescale_offset = mdhd_index + 24;
        return read_be32(data.data() + timescale_offset);
    }

    return 0;
}

unsigned long long read_stts_total_frames(const std::vector<char>& data, int channel)
{
    unsigned long long moov = resolve_box_offset(data, "moov", 0, data.size());
    if (moov == 0) return false;
    unsigned long long moov_end = moov + read_be32(data.data() + moov);

    unsigned long long offset = moov + 8;
    while (offset + 8 < moov_end)
    {
        unsigned long long trak = resolve_box_offset(data, "trak", offset, moov_end);
        if (trak == 0 || trak >= moov_end) break;
        unsigned long long trak_end = trak + read_be32(data.data() + trak);

        unsigned long long mdia = resolve_box_offset(data, "mdia", trak + 8, trak_end);
        if (mdia == 0) { offset = trak_end; continue; }

        unsigned long long minf = resolve_box_offset(data, "minf", mdia + 8, trak_end);
        if (minf == 0) { offset = trak_end; continue; }

        unsigned long long stbl = resolve_box_offset(data, "stbl", minf + 8, trak_end);
        if (stbl == 0) { offset = trak_end; continue; }

        unsigned long long stts = resolve_box_offset(data, "stts", stbl + 8, trak_end);
        if (stts == 0) { offset = trak_end; continue; }

        if (channel == 0)
        {
            unsigned long long entry_count_offset = stts + 12;
            unsigned long long entry_count = read_be32(data.data() + entry_count_offset);
            unsigned long long pos = entry_count_offset + 4;

            unsigned long long total_frames = 0;
            for (unsigned long long i = 0; i < entry_count; i++)
            {
                if (pos + 8 > data.size()) break;
                unsigned long long sample_count = read_be32(data.data() + pos); pos += 4;
                pos += 4;
                total_frames += sample_count;
            }

            return total_frames;
        }

        channel--;
        offset = trak_end;
    }

    return false;
}



unsigned long long read_mdhd_duration(const std::vector<char>& data, int channel)
{
    unsigned long long mdhd_audio_index = resolve_atom_offset(data, "mdhd", channel);
    if (mdhd_audio_index == -1) return false;

    unsigned char version = data[mdhd_audio_index + 4];
    unsigned long long duration = -1;
    if (version == 0)
    {
        unsigned long long duration_offset = mdhd_audio_index + 20;
        return read_be32(data.data() + duration_offset);
    }
    if (version == 1)
    {
        unsigned long long duration_offset = mdhd_audio_index + 28;
        return read_be64(data.data() + duration_offset);
    }

    return 0;
}

bool patch_mdhd_duration(std::vector<char>& data, unsigned long long duration, int channel)
{
    unsigned long long mdhd_audio_index = resolve_atom_offset(data, "mdhd", channel);
    if (mdhd_audio_index == -1) return false;

    unsigned char version = data[mdhd_audio_index + 4];
    if (version == 0)
    {
        unsigned long long duration_offset = mdhd_audio_index + 20;
        write_be32(data.data() + duration_offset, duration);
    }
    if (version == 1)
    {
        unsigned long long duration_offset = mdhd_audio_index + 28;
        write_be64(data.data() + duration_offset, duration);
    }

    return true;
}

unsigned long long read_elst_duration(const std::vector<char>& data, int channel)
{
    int elst_audio_index = resolve_atom_offset(data, "elst", channel);
    if (elst_audio_index == -1) return 0;

    unsigned char version = data[elst_audio_index + 4];
    unsigned long long duration = 0;

    int entry_count_offset = elst_audio_index + 8;
    unsigned long long entry_count = read_be32(data.data() + entry_count_offset);
    if (entry_count < 1) return 0;

    int duration_offset = (version == 1) ? elst_audio_index + 16 : elst_audio_index + 12;
    return (version == 1)
        ? read_be64(data.data() + duration_offset)
        : read_be32(data.data() + duration_offset);
}

bool patch_elst_duration(std::vector<char>& data, unsigned long long duration, int channel)
{
    int elst_audio_index = resolve_atom_offset(data, "elst", channel);
    if (elst_audio_index == -1) return false;

    unsigned char version = data[elst_audio_index + 4];

    int entry_count_offset = elst_audio_index + 8;
    unsigned long long entry_count = read_be32(data.data() + entry_count_offset);
    if (entry_count < 1) return false;

    int duration_offset = (version == 1) ? elst_audio_index + 16 : elst_audio_index + 12;

    if (version == 1)
        write_be64(data.data() + duration_offset, duration);
    else
        write_be32(data.data() + duration_offset, duration);

    return true;
}

unsigned long long read_tkhd_duration(const std::vector<char>& data, unsigned long long channel = 0)
{
    unsigned long long match = 0;

    for (unsigned long long i = 0; i + 8 < data.size(); i++)
    {
        if (std::memcmp(&data[i + 4], "trak", 4) == 0)
        {
            unsigned long long trak_size = read_be32(data.data() + i);
            unsigned long long trak_end = i + trak_size;

            unsigned long long mdia = resolve_box_offset(data, "mdia", i + 8, trak_end);
            if (mdia == 0) continue;

            unsigned long long hdlr = resolve_box_offset(data, "hdlr", mdia + 8, trak_end);
            if (hdlr == 0) continue;

            std::string handler(&data[hdlr + 16], 4);
            if (handler != "soun") continue;

            if (channel == 0)
            {
                unsigned long long j = i + 8;
                while (j + 8 < trak_end)
                {
                    if (std::memcmp(&data[j + 4], "tkhd", 4) == 0)
                    {
                        unsigned char version = data[j + 8];
                        size_t offset = (version == 1) ? j + 36 : j + 28;
                        return read_be32(data.data() + offset);
                    }

                    unsigned long long inner_size = read_be32(data.data() + j);
                    if (inner_size < 8) break;
                    j += inner_size;
                }
            }

            channel--;
        }
    }

    return 0;
}

bool patch_tkhd_duration(std::vector<char>& data, unsigned long long duration, unsigned long long channel = 0)
{
    unsigned long long match = 0;

    for (unsigned long long i = 0; i + 8 < data.size(); i++)
    {
        if (std::memcmp(&data[i + 4], "trak", 4) == 0)
        {
            unsigned long long trak_size = read_be32(data.data() + i);
            unsigned long long trak_end = i + trak_size;

            unsigned long long mdia = resolve_box_offset(data, "mdia", i + 8, trak_end);
            if (mdia == 0) continue;

            unsigned long long hdlr = resolve_box_offset(data, "hdlr", mdia + 8, trak_end);
            if (hdlr == 0) continue;

            std::string handler(&data[hdlr + 16], 4);
            if (handler != "soun") continue;

            if (channel == 0)
            {
                unsigned long long j = i + 8;
                while (j + 8 < trak_end)
                {
                    if (std::memcmp(&data[j + 4], "tkhd", 4) == 0)
                    {
                        unsigned char version = data[j + 8];
                        unsigned long long offset = (version == 1) ? j + 36 : j + 28;
                        write_be32(data.data() + offset, duration);
                        return true;
                    }

                    unsigned long long inner_size = read_be32(data.data() + j);
                    if (inner_size < 8) break;
                    j += inner_size;
                }
            }

            channel--;
        }
    }

    return false;
}



bool patch_chunk_offsets(std::vector<char>& data, unsigned long long insert_offset, unsigned long long size_inc)
{
    unsigned long long moov = resolve_box_offset(data, "moov", 0, data.size());
    if (moov == 0) return false;
    unsigned long long moov_end = moov + read_be32(data.data() + moov);

    unsigned long long offset = moov + 8;
    while (offset + 8 < moov_end)
    {
        unsigned long long trak = resolve_box_offset(data, "trak", offset, moov_end);
        if (trak == 0 || trak >= moov_end) break;
        unsigned long long trak_end = trak + read_be32(data.data() + trak);

        unsigned long long mdia = resolve_box_offset(data, "mdia", trak + 8, trak_end);
        if (mdia == 0) { offset = trak_end; continue; }

        unsigned long long hdlr = resolve_box_offset(data, "hdlr", mdia + 8, trak_end);
        if (hdlr == 0) { offset = trak_end; continue; }

        std::string handler(&data[hdlr + 16], 4);
        if (handler != "soun" && handler != "vide") { offset = trak_end; continue; }

        unsigned long long minf = resolve_box_offset(data, "minf", mdia + 8, trak_end);
        if (minf == 0) { offset = trak_end; continue; }

        unsigned long long stbl = resolve_box_offset(data, "stbl", minf + 8, trak_end);
        if (stbl == 0) { offset = trak_end; continue; }

        unsigned long long stbl_end = stbl + read_be32(data.data() + stbl);
        unsigned long long stco = resolve_box_offset(data, "stco", stbl + 8, stbl_end);
        unsigned long long co64 = resolve_box_offset(data, "co64", stbl + 8, stbl_end);

        if (stco)
        {
            unsigned long long count_offset = stco + 12;
            unsigned long long entry_count = read_be32(data.data() + count_offset);
            unsigned long long entry_offset = count_offset + 4;

            for (unsigned long long i = 0; i < entry_count; i++)
            {
                unsigned long long val_offset = entry_offset + i * 4;
                unsigned long long chunk_offset = read_be32(data.data() + val_offset);
                if (chunk_offset >= insert_offset)
                {
                    write_be32(data.data() + val_offset, chunk_offset + size_inc);
                }
            }
        }

        if (co64)
        {
            unsigned long long count_offset = co64 + 12;
            unsigned long long entry_count = read_be32(data.data() + count_offset);
            unsigned long long entry_offset = count_offset + 4;

            for (unsigned long long i = 0; i < entry_count; i++)
            {
                unsigned long long val_offset = entry_offset + i * 8;
                unsigned long long chunk_offset = read_be64(data.data() + val_offset);
                if (chunk_offset >= insert_offset)
                {
                    write_be64(data.data() + val_offset, chunk_offset + size_inc);
                }
            }
        }

        offset = trak_end;
    }

    return true;
}

bool patch_stts(std::vector<char>& data, double speed, bool multiply, int channel)
{
    unsigned long long moov = resolve_box_offset(data, "moov", 0, data.size());
    if (moov == 0) return false;
    unsigned long long moov_end = moov + read_be32(data.data() + moov);

    unsigned long long offset = moov + 8;
    while (offset + 8 < moov_end)
    {
        unsigned long long trak = resolve_box_offset(data, "trak", offset, moov_end);
        if (trak == 0 || trak >= moov_end) break;
        unsigned long long trak_end = trak + read_be32(data.data() + trak);

        unsigned long long mdia = resolve_box_offset(data, "mdia", trak + 8, trak_end);
        if (mdia == 0) { offset = trak_end; continue; }

        unsigned long long minf = resolve_box_offset(data, "minf", mdia + 8, trak_end);
        if (minf == 0) { offset = trak_end; continue; }

        unsigned long long stbl = resolve_box_offset(data, "stbl", minf + 8, trak_end);
        if (stbl == 0) { offset = trak_end; continue; }

        unsigned long long stts = resolve_box_offset(data, "stts", stbl + 8, trak_end);
        if (stts == 0) { offset = trak_end; continue; }

        if (channel == 0)
        {
            unsigned long long entry_count_offset = stts + 12;
            unsigned long long entry_count = read_be32(data.data() + entry_count_offset);
            unsigned long long first_entry = entry_count_offset + 4;

            if (entry_count == 1)
            {
                unsigned long long sample_count = read_be32(data.data() + first_entry);
                unsigned long long sample_delta = read_be32(data.data() + first_entry + 4);
                if (sample_count <= 1 || sample_delta < 2) return false;

                write_be32(data.data() + first_entry, sample_count - 1);

                std::vector<char> new_entry(8);
                write_be32(new_entry.data(), 1);
                write_be32(new_entry.data() + 4, ldd(sample_delta, speed));
                data.insert(data.begin() + first_entry + 8, new_entry.begin(), new_entry.end());

                write_be32(data.data() + entry_count_offset, 2);
                if (multiply) write_be32(data.data() + first_entry + 4, lmd(sample_delta, speed));

                for (auto box : { stts, stbl, minf, mdia, trak, moov })
                {
                    unsigned long long size = read_be32(data.data() + box);
                    write_be32(data.data() + box, size + 8);
                }

                patch_chunk_offsets(data, first_entry + 8, 8);
            }
            else if (entry_count >= 2)
            {
                unsigned long long sample_delta = read_be32(data.data() + first_entry + 4);
                if (multiply) write_be32(data.data() + first_entry + 4, lmd(sample_delta, speed));
            }

            return true;
        }

        channel--;
        offset = trak_end;
    }

    return false;
}

double read_fps_from_mp4(const std::vector<char>& data, int channel = 0)
{
    unsigned long long timescale = read_mdhd_timescale(data, channel);
    unsigned long long duration = read_mdhd_duration(data, channel);
    unsigned long long total_frames = read_stts_total_frames(data, channel);

    if (timescale == 0 || duration == 0) return 0.0;
    double duration_in_seconds = static_cast<double>(duration) / timescale;
    return total_frames / duration_in_seconds;
}



std::string get_working_directory()
{
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    return std::string(buffer);
}

void menu_version()
{
    std::cout << "MP4 FPS Hack - Version: 1.0" << std::endl;
}

void menu_help()
{
    std::cout << std::endl;
    std::cout << "- MP4 FPS Hack instructions -" << std::endl;
    std::cout << std::endl;
    std::cout << "description:" << std::endl;
    std::cout << std::endl;
    std::cout << "converts videos to mp4 and kind of overclocks them, to let them claim have a specific fps, but actually are played on different speed." << std::endl;
    std::cout << "tool aims for 30 fps stealth mode, but fps can be adjusted as needed (experimental)." << std::endl;
    std::cout << "can be backtested, in VLC player it should be slow and stutter in audio, in windows media player, it should be run in the correct fps settings and normal audio." << std::endl;
    std::cout << std::endl;
    std::cout << "usage:" << std::endl;
    std::cout << std::endl;
    std::cout << "mp4_fps_hack.exe <input_file.|mp4|avi|mov> <output_file.mp4> <mode|1|2|3 [1]> <video_channel|0|1|2 [0]> <audio_channel|0|1|2 [1]> <safe_codec|false|true [false]> <target_fps [30]>" << std::endl;
    std::cout << std::endl;
    std::cout << "arguments:" << std::endl;
    std::cout << std::endl;
    std::cout << "input_file: the absolute or relative file path to the input video that should be patched (Tested with .mp4, .avi and .mov, can be something else probably)" << std::endl;
    std::cout << "output_file: the absolute or relative file path to the output video that should saved" << std::endl;
    std::cout << "mode: integer representing the mode that should be used for the patch (1 = audio stts (recommended), 2 = audio stts and duration, 3 = audio/video stts and duration)" << std::endl;
    std::cout << "video channel: integer representing the index of the video channel (0 by default)" << std::endl;
    std::cout << "audio channel: integer representing the index of the audio channel (1 by default)" << std::endl;
    std::cout << "safe codec: uses codec settings, that are proven to work with this (video quality may suffer)" << std::endl;
    std::cout << "safe codec: uses codec settings, that are proven to work with this (video quality may suffer)" << std::endl;
    std::cout << std::endl;
    std::cout << "if the hack dont work for your file, try in probably this order:" << std::endl;
    std::cout << "- video channel = 1, audio channel = 0" << std::endl;
    std::cout << "- mode = 2" << std::endl;
    std::cout << "- mode = 2, video channel = 1, audio channel = 0" << std::endl;
    std::cout << "- mode = 3" << std::endl;
    std::cout << "- mode = 3, video channel = 1, audio channel = 0" << std::endl;
    std::cout << "- as addition: safe codec = true" << std::endl;
    std::cout << std::endl;
    std::cout << "quick go:" << std::endl;
    std::cout << "mp4_fps_hack.exe \"C:\\mypath\\input.mp4 C:\\mypath\\output.mp4\"" << std::endl;
    std::cout << std::endl;
    std::cout << "complex go:" << std::endl;
    std::cout << "mp4_fps_hack.exe \"C:\\mypath\\input.mp4 C:\\mypath\\output.mp4\" 1 0 1 false 30" << std::endl;
    std::cout << std::endl;
    std::cout << "if the application isn't work at all, make sure ffmpeg is installed and set in PATH variable, as it is the requirement to bring the video in the correct format" << std::endl;
    std::cout << std::endl;
}



int main(int argc, char* argv[])
{
    std::string directory_working = get_working_directory() + "\\";

    std::string file_input = "";
    std::string file_output = "";
    std::string file_tmp = directory_working + "tmp_file.mp4";
    int mode = 1;
    int channel_video = 0;
    int channel_audio = 1;
    bool safe_codec = false;
    double target_fps = 30;



    // ARC 1: The Configuration

    if (argc == 1)
    {
        menu_help();
        return 1;
    }

    if (argc > 1)
    {
        file_input = argv[1];
        if (file_input == "-version" || file_input == "--version" || file_input == "-v" || file_input == "--v")
        {
            menu_version();
            return 1;
        }
        if (file_input == "-help" || file_input == "--help" || file_input == "-?" || file_input == "--?")
        {
            menu_help();
            return 1;
        }
        if (file_input.find("\\") == std::string::npos)
        {
            file_input.insert(file_input.begin(), directory_working.begin(), directory_working.end());
        }
    }

    if (argc > 2)
    {
        file_output = argv[2];
        if (file_output.find("\\") == std::string::npos)
        {
            file_output.insert(file_output.begin(), directory_working.begin(), directory_working.end());
        }
    }
    else
    {
        std::cout << "[ERROR] Missing argument 2 - no output_file is specified" << std::endl;
        return -1;
    }

    if (argc > 3)
    {
        try
        {
            mode = std::stoi(argv[3]);
            if (mode < 1 || mode > 3)
            {
                std::cout << "[ERROR] Argument 3 mode (" << argv[3] << ") is has to be either 1, 2 or 3." << std::endl;
                return -2;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "[ERROR] Argument 3 mode (" << argv[3] << ") is invalid." << std::endl;
            return -3;
        }
    }

    if (argc > 4)
    {
        try
        {
            channel_video = std::stoi(argv[4]);
            if (channel_video < 0 || channel_video > 2)
            {
                std::cout << "[ERROR] Argument 4 video channel (" << argv[3] << ") is has to be between 0 and 2" << std::endl;
                return -4;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "[ERROR] Argument 4 video channel (" << argv[4] << ") is invalid." << std::endl;
            return -5;
        }
    }

    if (argc > 5)
    {
        try
        {
            channel_video = std::stoi(argv[5]);
            if (channel_audio < 0 || channel_audio > 2)
            {
                std::cout << "[ERROR] Argument 5 audio channel (" << argv[3] << ") is has to be between 0 and 2" << std::endl;
                return -6;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "[ERROR] Argument 5 audio channel (" << argv[5] << ") is invalid." << std::endl;
            return -7;
        }
    }

    if (argc > 6)
    {
        safe_codec = argv[6] == "true";
    }

    if (argc > 7)
    {
        try
        {
            target_fps = std::stod(argv[7]);
            if (target_fps < 0 || target_fps > 300)
            {
                std::cout << "[ERROR] Argument 7 fps (" << argv[3] << ") is has to be between 0 and 300" << std::endl;
                return -8;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "[ERROR] Argument 7 fps (" << argv[5] << ") is invalid." << std::endl;
            return -9;
        }
    }
    
    DeleteFileA(file_output.c_str());



    // ARC 2: FFMPEG the big hero

    if (system("where ffmpeg >nul 2>nul") == 0)
    {
        std::cout << "[ERROR] ffmpeg installation not found, make sure it's installed and set to PATH variable" << std::endl;
        return -10;
    }

    std::vector<char> tmp_data;
    if (file_input.find(".mp4") == std::string::npos)
    {
        std::string convert_mp4_command = "ffmpeg -y -i \"" + file_input + "\" \"" + file_tmp + "\"";
        system(convert_mp4_command.c_str());
        tmp_data = read_mp4(file_tmp);
        DeleteFileA(file_tmp.c_str());
    }
    else
    {
        tmp_data = read_mp4(file_input);
    }

    double fps = read_fps_from_mp4(tmp_data, channel_video);
    std::cout << "target fps: " << fps << std::endl;

    double speed = fps / target_fps;
    std::cout << "speed factor: " << d2sp1(speed) << std::endl;

    std::string scale_command = "";
    if (safe_codec)
    {
        scale_command = "ffmpeg -i \"" + file_input + "\" -vf \"setpts=" + d2sp1(speed) + "*PTS\" -r " + d2sp2(target_fps) + " -c:v libx264 -pix_fmt yuv420p -preset fast -crf 23 -level 4.2 -c:a aac -b:a 128k -ar 44100 -movflags +faststart \"" + file_output + "\"";
    }
    else
    {
        scale_command = "ffmpeg -i \"" + file_input + "\" -vf \"setpts=" + d2sp1(speed) + "*PTS\" -r " + d2sp2(target_fps) + " \"" + file_output + "\"";
    }
    system(scale_command.c_str());



    // ARC 3: Shady things going on in MP4

    auto output_data = read_mp4(file_output);

    if (mode >= 1)
    {
        std::cout << "patch stts audio delta..." << std::endl;
        patch_stts(output_data, speed, true, channel_audio);
    }
    if (mode >= 2)
    {
        std::cout << "patch audio duration parameter..." << std::endl;

        unsigned long long mdhd_duration = read_mdhd_duration(output_data, channel_audio);
        if (mdhd_duration > 0)
        {
            unsigned long long duration = lmd(mdhd_duration, speed);
            std::cout << "old mdhd duration: " << mdhd_duration << " | new mdhd duration: " << duration << std::endl;
            patch_mdhd_duration(output_data, duration, channel_audio);
        }
        else
        {
            std::cout << "mdhd duration not found!" << std::endl;
        }

        unsigned long long elst_duration = read_elst_duration(output_data, channel_audio);
        if (elst_duration > 0)
        {
            unsigned long long duration = lmd(elst_duration, speed);
            std::cout << "old elst duration: " << elst_duration << " | new elst duration: " << duration << std::endl;
            patch_elst_duration(output_data, duration, channel_audio);
        }
        else
        {
            std::cout << "elst duration not found!" << std::endl;
        }

        unsigned long long tkhd_duration = read_tkhd_duration(output_data, channel_audio);
        if (tkhd_duration > 0)
        {
            unsigned long long duration = lmd(tkhd_duration, speed);
            std::cout << "old tkhd duration: " << tkhd_duration << " | new tkhd duration: " << duration << std::endl;
            patch_tkhd_duration(output_data, duration, 1);
        }
        else
        {
            std::cout << "tkhd duration not found!" << std::endl;
            if (channel_audio > 0)
            {
                std::cout << "Maybe only 1 trak? Switching to channel flip fallback..." << std::endl;
                tkhd_duration = read_tkhd_duration(output_data, 0);
                if (tkhd_duration > 0)
                {
                    unsigned long long duration = lmd(tkhd_duration, speed);
                    std::cout << "old tkhd duration: " << tkhd_duration << " | new tkhd duration: " << duration << std::endl;
                    patch_tkhd_duration(output_data, duration, 1);
                }
                else
                {
                    std::cout << "tkhd duration not found!" << std::endl;
                }
            }
        }
    }
    if (mode == 3)
    {
        std::cout << "patch stts video delta..." << std::endl;
        patch_stts(output_data, speed, false, channel_video);
    }

    write_mp4(file_output, output_data);



    return 1;
}


