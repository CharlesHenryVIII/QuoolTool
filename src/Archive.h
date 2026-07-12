#include "CashUtil\CashUtil.h"

void CreateZip(const Path& zip_path, const Path& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<Path> files_to_add_to_root, Atomic<u64>& progress/*, ArrayView<std::wstring> ext_to_exclude*/);
bool UnzipArchive(const std::string& zip_path, const std::string& output_dir, std::vector<std::string>& filenames);