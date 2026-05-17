#ifndef LIBRARY_STORE_H_
#define LIBRARY_STORE_H_

#include <filesystem>
#include <string>
#include <vector>

struct LibraryBook
{
    std::string id;
    std::filesystem::path path;
    std::string title;
    std::string author;
    std::string format;
    std::filesystem::path cover_path;
    uint32_t progress = 0;
    std::string status = "new";
    int64_t added_at = 0;
    int64_t last_opened_at = 0;
    uintmax_t file_size = 0;
    int64_t last_modified = 0;
};

class LibraryStore
{
    std::filesystem::path library_path;
    std::filesystem::path cover_dir;
    std::vector<LibraryBook> books;

public:
    LibraryStore(std::filesystem::path state_dir);

    const std::vector<LibraryBook> &get_books() const;
    void load();
    void save() const;

    bool contains_book(const std::filesystem::path &path) const;
    bool add_book(const std::filesystem::path &path);
    bool remove_book(const std::filesystem::path &path);
    void mark_opened(const std::filesystem::path &path);
    void update_progress(const std::filesystem::path &path, uint32_t progress);
};

#endif
