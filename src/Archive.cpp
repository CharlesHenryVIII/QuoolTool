#include "Archive.h"

#include "libarchive/libarchive/archive.h"
#include "libarchive/libarchive/archive_entry.h"

void ArchiveErrorCheck(archive* a, int e)
{
    if (e != ARCHIVE_OK)
    {
        const char* error_rr_string = archive_error_string(a);
        DebugPrint("Archive Failure: %s", error_rr_string);
        FAIL;
    }
}

void AddEntryToZip(archive* a, const std::filesystem::path& full_path, const std::filesystem::path& relative_path, bool is_dir, std::vector<u8>& file_buffer, std::atomic<u64>& progress)
{
    struct stat st;
    if (stat(full_path.string().c_str(), &st) != 0)
    {
        perror("Problem getting information");
        int r = errno;
        switch (r)
        {
        case ENOENT:
            DebugPrint("File %s not found.\n", full_path.string().c_str());
            break;
        case EINVAL:
            DebugPrint("Invalid parameter to _stat.\n");
            break;
        default:
            //Should never be reached.
            DebugPrint("Unexpected error in _stat.\n");
        }
        FAIL;
        return;
    }
    if (is_dir)
    {
        std::vector<ScannedFile> out;
        SysScanDirectoryForFileNames(full_path, out, ScanDirectoryFlags_IncludeDirs);
        for (i32 i = 0; i < (i32)out.size(); i++)
        {
            AddEntryToZip(a, full_path / out[i].name, relative_path / out[i].name, out[i].dir, file_buffer, progress);
        }
    }
    else
    {
        archive_entry* entry = archive_entry_new();
        archive_entry_set_pathname(entry, relative_path.string().c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_copy_stat(entry, &st);
        int error = archive_write_header(a, entry);
        ArchiveErrorCheck(a, error);

        {
            std::ifstream file(full_path, std::ios::binary | std::ios::ate);
            if (!file)
            {
                DebugPrint("Error opening file: %s", full_path.string().c_str());
                FAIL;
                return;
            }
            const size_t file_size = (size_t)file.tellg();
            if (file_size > file_buffer.size())
                file_buffer.resize(file_size * 2);
            file.seekg(0, std::ios::beg);
            file.read((char*)file_buffer.data(), file_size);

            error = (int)archive_write_data(a, file_buffer.data(), file_size);
            if (error < 0)
                ArchiveErrorCheck(a, error);
            ++progress;
        }
        archive_entry_free(entry);
    }
}

void CreateZip(const Path& zip_path, const Path& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<Path> files_to_add_to_root, Atomic<u64>& progress/*, ArrayView<std::wstring> ext_to_exclude*/)
{
    archive* a = archive_write_new();
    archive_write_set_format_zip(a);
    int error = archive_write_zip_set_compression_deflate(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_set_options(a, "compression-level=9");
    ArchiveErrorCheck(a, error);
    CreateParentDirectories(zip_path);
    error = archive_write_open_filename(a, zip_path.string().c_str());
    ArchiveErrorCheck(a, error);

    std::vector<u8> file_buffer;
    //file_buffer.reserve(64*1000*1000);
    for (i32 i = 0; i < files_to_backup.size(); i++)
    {
        Path full;
        if (!source_folder.empty())
            full = source_folder / files_to_backup[i].name;
        else
            full = files_to_backup[i].name;
        AddEntryToZip(a, full, files_to_backup[i].name, files_to_backup[i].dir, file_buffer, progress);
    }
    for (i32 i = 0; i < files_to_add_to_root.size(); i++)
    {
        AddEntryToZip(a, files_to_add_to_root[i], files_to_add_to_root[i].filename(), false, file_buffer, progress);
    }

    error = archive_write_close(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_free(a);
    ArchiveErrorCheck(a, error);
}

bool UnzipArchive(const std::string& zip_path, const std::string& output_dir, std::vector<std::string>& filenames)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

    int error = ARCHIVE_OK;
    archive_read_open_filename(a, zip_path.c_str(), 10240);
    ArchiveErrorCheck(a, error);

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME |
        ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL |
        ARCHIVE_EXTRACT_FFLAGS);

    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const char* relative_path = archive_entry_pathname(entry);
        std::string full_path;
        if (output_dir.size())
            full_path = output_dir + "/" + relative_path;
        else
            full_path = relative_path;
        archive_entry_set_pathname(entry, full_path.c_str());
        filenames.push_back(full_path);

        archive_write_header(ext, entry);

        const void* buff;
        size_t size;
        la_int64_t offset;

        while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
        {
            archive_write_data_block(ext, buff, size, offset);
        }

        archive_write_finish_entry(ext);
    }

    archive_write_close(ext);
    archive_write_free(ext);

    archive_read_close(a);
    archive_read_free(a);

    return true;
}