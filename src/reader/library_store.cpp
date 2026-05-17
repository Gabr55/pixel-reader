#include "./library_store.h"

#include "filetypes/open_doc.h"
#include "filetypes/epub/epub_metadata.h"
#include "util/str_utils.h"
#include "util/zip_utils.h"
#include "extern/hash-library/md5.h"

#include <libxml/parser.h>
#include <zip.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>

namespace
{

int64_t now_sec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

int64_t file_mtime(const std::filesystem::path &path)
{
    try
    {
        return std::filesystem::last_write_time(path).time_since_epoch().count();
    }
    catch (...)
    {
        return 0;
    }
}

uintmax_t safe_file_size(const std::filesystem::path &path)
{
    try
    {
        return std::filesystem::file_size(path);
    }
    catch (...)
    {
        return 0;
    }
}

std::string json_escape(const std::string &s)
{
    std::string out;
    for (char c : s)
    {
        switch (c)
        {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string json_unescape(std::string s)
{
    std::string out;
    for (uint32_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            char n = s[++i];
            if (n == 'n') out += '\n';
            else if (n == 'r') out += '\r';
            else if (n == 't') out += '\t';
            else out += n;
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

std::string json_string_field(const std::string &obj, const std::string &name)
{
    std::regex field("\"" + name + "\"[[:space:]]*:[[:space:]]*\"((\\\\.|[^\"])*)\"");
    std::smatch match;
    if (std::regex_search(obj, match, field))
    {
        return json_unescape(match[1]);
    }
    return "";
}

int64_t json_int_field(const std::string &obj, const std::string &name)
{
    std::regex field("\"" + name + "\"[[:space:]]*:[[:space:]]*(-?[0-9]+)");
    std::smatch match;
    if (std::regex_search(obj, match, field))
    {
        return std::stoll(match[1]);
    }
    return 0;
}

uint32_t json_uint_field(const std::string &obj, const std::string &name)
{
    int64_t val = json_int_field(obj, name);
    return val < 0 ? 0 : static_cast<uint32_t>(val);
}

std::string file_format(const std::filesystem::path &path)
{
    auto ext = to_lower(path.extension().string());
    if (ext.size() && ext[0] == '.')
    {
        ext.erase(0, 1);
    }
    return ext;
}

std::string stable_book_id(const std::filesystem::path &path)
{
    auto abs = std::filesystem::absolute(path).lexically_normal().string();
    auto input = abs + "|" + std::to_string(safe_file_size(path)) + "|" + std::to_string(file_mtime(path));
    return MD5()(input.c_str(), input.size());
}

std::string status_for_progress(uint32_t progress)
{
    if (progress == 0)
    {
        return "new";
    }
    if (progress >= 98)
    {
        return "finished";
    }
    return "reading";
}

std::string first_text(xmlNodePtr node, const char *name)
{
    while (node)
    {
        if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, BAD_CAST name))
        {
            xmlNodePtr child = node->children;
            while (child)
            {
                if (child->type == XML_TEXT_NODE && child->content)
                {
                    return strip_whitespace((const char*)child->content);
                }
                child = child->next;
            }
        }

        if (auto found = first_text(node->children, name); !found.empty())
        {
            return found;
        }
        node = node->next;
    }
    return "";
}

std::pair<std::string, std::string> epub_title_author(const std::filesystem::path &path)
{
    int err = 0;
    zip_t *zip = zip_open(path.c_str(), ZIP_RDONLY, &err);
    if (!zip)
    {
        return {"", ""};
    }

    std::string title, author;
    auto container_xml = read_zip_file_str(zip, EPUB_CONTAINER_PATH);
    auto rootfile_path = container_xml.empty() ? std::string() : epub_parse_rootfile_path(container_xml.data());
    if (!rootfile_path.empty())
    {
        auto package_xml = read_zip_file_str(zip, rootfile_path);
        if (!package_xml.empty())
        {
            xmlDocPtr doc = xmlReadMemory(package_xml.data(), package_xml.size(), nullptr, nullptr, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
            if (doc)
            {
                xmlNodePtr root = xmlDocGetRootElement(doc);
                title = first_text(root, "title");
                author = first_text(root, "creator");
                xmlFreeDoc(doc);
            }
        }
    }

    zip_close(zip);
    return {title, author};
}

LibraryBook make_book(const std::filesystem::path &path)
{
    LibraryBook book;
    book.path = std::filesystem::absolute(path).lexically_normal();
    book.id = stable_book_id(book.path);
    book.format = file_format(book.path);
    book.file_size = safe_file_size(book.path);
    book.last_modified = file_mtime(book.path);
    book.added_at = now_sec();
    book.last_opened_at = 0;

    if (to_lower(book.path.extension().string()) == ".epub")
    {
        auto [title, author] = epub_title_author(book.path);
        book.title = title;
        book.author = author;
    }

    if (book.title.empty())
    {
        book.title = book.path.stem().string();
    }
    if (book.author.empty())
    {
        book.author = "Unknown";
    }

    return book;
}

bool same_book_identity(const LibraryBook &book, const std::filesystem::path &path)
{
    auto abs = std::filesystem::absolute(path).lexically_normal();
    return book.path == abs ||
        (book.file_size == safe_file_size(abs) && book.last_modified == file_mtime(abs));
}

void sort_books(std::vector<LibraryBook> &books)
{
    std::sort(books.begin(), books.end(), [](const LibraryBook &a, const LibraryBook &b) {
        if (a.last_opened_at != b.last_opened_at)
        {
            return a.last_opened_at > b.last_opened_at;
        }
        if (a.added_at != b.added_at)
        {
            return a.added_at > b.added_at;
        }
        return a.title < b.title;
    });
}

} // namespace

LibraryStore::LibraryStore(std::filesystem::path state_dir)
    : library_path(state_dir / "library.json")
{
    load();
}

const std::vector<LibraryBook> &LibraryStore::get_books() const
{
    return books;
}

void LibraryStore::load()
{
    books.clear();
    std::ifstream in(library_path);
    if (!in.is_open())
    {
        return;
    }

    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::regex object_re("\\{[^\\{\\}]*\"path\"[^\\{\\}]*\\}");
    for (auto it = std::sregex_iterator(json.begin(), json.end(), object_re); it != std::sregex_iterator(); ++it)
    {
        std::string obj = it->str();
        LibraryBook book;
        book.id = json_string_field(obj, "id");
        book.path = json_string_field(obj, "path");
        book.title = json_string_field(obj, "title");
        book.author = json_string_field(obj, "author");
        book.format = json_string_field(obj, "format");
        book.progress = json_uint_field(obj, "progress");
        book.status = json_string_field(obj, "status");
        book.added_at = json_int_field(obj, "added_at");
        book.last_opened_at = json_int_field(obj, "last_opened_at");
        book.file_size = static_cast<uintmax_t>(json_int_field(obj, "file_size"));
        book.last_modified = json_int_field(obj, "last_modified");

        if (!book.id.empty() && !book.path.empty())
        {
            if (!std::filesystem::exists(book.path))
            {
                book.status = "missing";
            }
            books.push_back(book);
        }
    }
    sort_books(books);
}

void LibraryStore::save() const
{
    std::filesystem::create_directories(library_path.parent_path());
    std::ofstream out(library_path);
    out << "{\n  \"version\": 1,\n  \"books\": [\n";
    for (uint32_t i = 0; i < books.size(); ++i)
    {
        const auto &b = books[i];
        out << "    {\n";
        out << "      \"id\": \"" << json_escape(b.id) << "\",\n";
        out << "      \"path\": \"" << json_escape(b.path.string()) << "\",\n";
        out << "      \"title\": \"" << json_escape(b.title) << "\",\n";
        out << "      \"author\": \"" << json_escape(b.author) << "\",\n";
        out << "      \"format\": \"" << json_escape(b.format) << "\",\n";
        out << "      \"progress\": " << b.progress << ",\n";
        out << "      \"status\": \"" << json_escape(b.status) << "\",\n";
        out << "      \"added_at\": " << b.added_at << ",\n";
        out << "      \"last_opened_at\": " << b.last_opened_at << ",\n";
        out << "      \"file_size\": " << b.file_size << ",\n";
        out << "      \"last_modified\": " << b.last_modified << "\n";
        out << "    }" << (i + 1 == books.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
}

bool LibraryStore::add_book(const std::filesystem::path &path)
{
    if (!std::filesystem::is_regular_file(path) || !file_type_is_supported(path))
    {
        return false;
    }
    for (const auto &book : books)
    {
        if (same_book_identity(book, path))
        {
            return false;
        }
    }

    books.push_back(make_book(path));
    sort_books(books);
    save();
    return true;
}

uint32_t LibraryStore::add_folder(const std::filesystem::path &path)
{
    uint32_t count = 0;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file() && add_book(entry.path()))
        {
            ++count;
        }
    }
    return count;
}

void LibraryStore::mark_opened(const std::filesystem::path &path)
{
    for (auto &book : books)
    {
        if (same_book_identity(book, path))
        {
            book.last_opened_at = now_sec();
            save();
            break;
        }
    }
    sort_books(books);
}

void LibraryStore::update_progress(const std::filesystem::path &path, uint32_t progress)
{
    for (auto &book : books)
    {
        if (same_book_identity(book, path))
        {
            book.progress = progress;
            book.status = status_for_progress(progress);
            save();
            break;
        }
    }
}
