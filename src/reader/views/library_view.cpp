#include "./library_view.h"

#include "./file_selector.h"
#include "./selection_menu.h"
#include "reader/config.h"
#include "reader/system_styling.h"
#include "reader/view_stack.h"
#include "sys/keymap.h"
#include "sys/screen.h"
#include "util/sdl_utils.h"
#include "util/throttled.h"

#include "extern/rotozoom/SDL_rotozoom.h"

#include <SDL/SDL_image.h>
#include <algorithm>
#include <unordered_map>

struct LibraryViewState
{
    LibraryStore &library_store;
    SystemStyling &sys_styling;
    ViewStack &view_stack;
    std::function<void(std::filesystem::path)> open_book;

    bool is_done = false;
    bool needs_render = true;
    uint32_t cursor = 0;
    uint32_t scroll = 0;
    Throttled scroll_throttle;
    std::unordered_map<std::string, surface_unique_ptr> cover_cache;

    LibraryViewState(
        LibraryStore &library_store,
        SystemStyling &sys_styling,
        ViewStack &view_stack,
        std::function<void(std::filesystem::path)> open_book
    ) : library_store(library_store),
        sys_styling(sys_styling),
        view_stack(view_stack),
        open_book(std::move(open_book)),
        scroll_throttle(250, 100)
    {
    }
};

namespace
{

constexpr int HEADER_H = 8;
constexpr int FOOTER_H = 44;
constexpr int ROW_H = 96;
constexpr int PAD = 8;
constexpr int COVER_W = 56;
constexpr int COVER_H = 80;
constexpr int COVER_X = PAD;
constexpr int TEXT_X = COVER_X + COVER_W + 10;

uint32_t visible_rows()
{
    return (SCREEN_HEIGHT - HEADER_H - FOOTER_H) / ROW_H;
}

std::string progress_label(const LibraryBook &book)
{
    if (book.status == "missing")
    {
        return "!";
    }
    if (book.progress == 0)
    {
        return "New";
    }
    if (book.progress >= 99)
    {
        return "End";
    }
    return std::to_string(book.progress) + "%";
}

void draw_text(SDL_Surface *dest, TTF_Font *font, const std::string &text, Sint16 x, Sint16 y, SDL_Color fg, SDL_Color bg)
{
    auto surface = surface_unique_ptr { TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg) };
    SDL_Rect rect = {x, y, 0, 0};
    SDL_BlitSurface(surface.get(), nullptr, dest, &rect);
}

void pop_utf8_char(std::string &s)
{
    if (s.empty())
    {
        return;
    }
    s.pop_back();
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80)
    {
        s.pop_back();
    }
}

std::string fit_text(TTF_Font *font, const std::string &text, int max_w)
{
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &w, &h) == 0 && w <= max_w)
    {
        return text;
    }

    std::string out = text;
    while (!out.empty())
    {
        pop_utf8_char(out);
        std::string candidate = out + "...";
        if (TTF_SizeUTF8(font, candidate.c_str(), &w, &h) == 0 && w <= max_w)
        {
            return candidate;
        }
    }
    return "...";
}

void draw_fitted_text(SDL_Surface *dest, TTF_Font *font, const std::string &text, Sint16 x, Sint16 y, int max_w, SDL_Color fg, SDL_Color bg)
{
    draw_text(dest, font, fit_text(font, text, max_w), x, y, fg, bg);
}

SDL_Surface *cover_surface(LibraryViewState &s, const LibraryBook &book, SDL_PixelFormat *format)
{
    if (book.cover_path.empty())
    {
        return nullptr;
    }

    const auto key = book.cover_path.string();
    auto found = s.cover_cache.find(key);
    if (found != s.cover_cache.end())
    {
        return found->second.get();
    }

    auto loaded = surface_unique_ptr{IMG_Load(key.c_str())};
    if (!loaded || loaded->w <= 0 || loaded->h <= 0)
    {
        s.cover_cache.emplace(key, nullptr);
        return nullptr;
    }

    double scale = std::min(COVER_W / static_cast<double>(loaded->w), COVER_H / static_cast<double>(loaded->h));
    auto scaled = surface_unique_ptr{zoomSurface(loaded.get(), scale, scale, 1)};
    if (!scaled)
    {
        s.cover_cache.emplace(key, nullptr);
        return nullptr;
    }

    auto converted = surface_unique_ptr{SDL_ConvertSurface(scaled.get(), format, 0)};
    auto it = s.cover_cache.emplace(key, std::move(converted)).first;
    return it->second.get();
}

void draw_cover_placeholder(SDL_Surface *dest, Sint16 x, Sint16 y, SDL_Color border, SDL_Color bg)
{
    uint32_t border_color = SDL_MapRGB(dest->format, border.r, border.g, border.b);
    uint32_t bg_color = SDL_MapRGB(dest->format, bg.r, bg.g, bg.b);
    SDL_Rect outer = {x, y, COVER_W, COVER_H};
    SDL_Rect inner = {static_cast<Sint16>(x + 2), static_cast<Sint16>(y + 2), COVER_W - 4, COVER_H - 4};
    SDL_FillRect(dest, &outer, border_color);
    SDL_FillRect(dest, &inner, bg_color);
}

void draw_cover(LibraryViewState &s, SDL_Surface *dest, const LibraryBook &book, Sint16 x, Sint16 y, SDL_Color border, SDL_Color bg)
{
    draw_cover_placeholder(dest, x, y, border, bg);

    SDL_Surface *cover = cover_surface(s, book, dest->format);
    if (!cover)
    {
        return;
    }

    SDL_Rect rect = {
        static_cast<Sint16>(x + (COVER_W - cover->w) / 2),
        static_cast<Sint16>(y + (COVER_H - cover->h) / 2),
        0,
        0
    };
    SDL_BlitSurface(cover, nullptr, dest, &rect);
}

void move_cursor(LibraryViewState &s, int delta)
{
    const auto &books = s.library_store.get_books();
    if (books.empty())
    {
        return;
    }

    int next = static_cast<int>(s.cursor) + delta;
    next = std::max(0, std::min(next, static_cast<int>(books.size()) - 1));
    s.cursor = static_cast<uint32_t>(next);

    uint32_t rows = visible_rows();
    if (s.cursor < s.scroll)
    {
        s.scroll = s.cursor;
    }
    else if (s.cursor >= s.scroll + rows)
    {
        s.scroll = s.cursor - rows + 1;
    }
    s.needs_render = true;
}

void open_add_menu(LibraryViewState &s)
{
    auto menu = std::make_shared<SelectionMenu>(
        std::vector<std::string>{"Add file"},
        s.sys_styling
    );
    menu->set_close_on_select();
    menu->set_on_selection([&s](uint32_t index) {
        if (index == 0)
        {
            auto picker = std::make_shared<FileSelector>(DEFAULT_BROWSE_PATH, s.sys_styling);
            picker->set_on_file_selected([&s](const std::filesystem::path &path) {
                s.library_store.add_book(path);
                s.needs_render = true;
            });
            picker->set_file_marked([&s](const std::filesystem::path &path) {
                return s.library_store.contains_book(path);
            });
            s.view_stack.push(picker);
        }
    });
    s.view_stack.push(menu);
}

void open_delete_confirm(LibraryViewState &s)
{
    const auto &books = s.library_store.get_books();
    if (books.empty() || s.cursor >= books.size())
    {
        return;
    }

    std::filesystem::path path = books[s.cursor].path;
    auto menu = std::make_shared<SelectionMenu>(
        std::vector<std::string>{"Delete from library?", "No", "Yes"},
        s.sys_styling
    );
    menu->set_cursor_pos(1);
    menu->set_close_on_select();
    menu->set_on_selection([&s, path](uint32_t index) {
        if (index == 2)
        {
            s.library_store.remove_book(path);
            const auto &books = s.library_store.get_books();
            if (books.empty())
            {
                s.cursor = 0;
                s.scroll = 0;
            }
            else if (s.cursor >= books.size())
            {
                s.cursor = books.size() - 1;
            }
            if (s.cursor < s.scroll)
            {
                s.scroll = s.cursor;
            }
            s.needs_render = true;
        }
    });
    s.view_stack.push(menu);
}

} // namespace

LibraryView::LibraryView(
    LibraryStore &library_store,
    SystemStyling &sys_styling,
    ViewStack &view_stack,
    std::function<void(std::filesystem::path)> open_book
) : state(std::make_unique<LibraryViewState>(library_store, sys_styling, view_stack, std::move(open_book)))
{
}

LibraryView::~LibraryView()
{
}

bool LibraryView::render(SDL_Surface *dest_surface, bool force_render)
{
    if (!state->needs_render && !force_render)
    {
        return false;
    }
    state->needs_render = false;

    const auto &theme = state->sys_styling.get_loaded_color_theme();
    TTF_Font *font = state->sys_styling.get_loaded_font();
    uint32_t bg = SDL_MapRGB(dest_surface->format, theme.background.r, theme.background.g, theme.background.b);
    uint32_t hl = SDL_MapRGB(dest_surface->format, theme.highlight_background.r, theme.highlight_background.g, theme.highlight_background.b);

    SDL_Rect full = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_FillRect(dest_surface, &full, bg);

    const auto &books = state->library_store.get_books();
    if (books.empty())
    {
        draw_text(dest_surface, font, "+ Add books", PAD, HEADER_H + 40, theme.main_text, theme.background);
        draw_text(dest_surface, font, "A add  B exit", PAD, SCREEN_HEIGHT - FOOTER_H + 4, theme.secondary_text, theme.background);
        return true;
    }

    uint32_t rows = visible_rows();
    for (uint32_t i = 0; i < rows; ++i)
    {
        uint32_t book_i = state->scroll + i;
        if (book_i >= books.size())
        {
            break;
        }

        const auto &book = books[book_i];
        Sint16 y = HEADER_H + i * ROW_H;
        bool selected = book_i == state->cursor;
        if (selected)
        {
            SDL_Rect rect = {0, y, SCREEN_WIDTH, ROW_H};
            SDL_FillRect(dest_surface, &rect, hl);
        }

        SDL_Color text = selected ? theme.highlight_text : theme.main_text;
        SDL_Color row_bg = selected ? theme.highlight_background : theme.background;
        std::string author = book.author.empty() ? "Unknown" : book.author;
        std::string progress = progress_label(book);

        draw_cover(*state, dest_surface, book, COVER_X, y + 8, theme.secondary_text, row_bg);
        draw_fitted_text(dest_surface, font, book.title, TEXT_X, y + 10, SCREEN_WIDTH - TEXT_X - 78, text, row_bg);
        draw_fitted_text(dest_surface, font, author, TEXT_X, y + 44, SCREEN_WIDTH - TEXT_X - 78, theme.secondary_text, row_bg);
        draw_text(dest_surface, font, progress, SCREEN_WIDTH - 70, y + 58, text, row_bg);
    }

    draw_text(dest_surface, font, "A open  Y del  Select add", PAD, SCREEN_HEIGHT - FOOTER_H + 4, theme.secondary_text, theme.background);
    return true;
}

bool LibraryView::is_done()
{
    return state->is_done;
}

void LibraryView::on_keypress(SDLKey key)
{
    const auto &books = state->library_store.get_books();
    switch (key)
    {
        case SW_BTN_UP:
            move_cursor(*state, -1);
            break;
        case SW_BTN_DOWN:
            move_cursor(*state, 1);
            break;
        case SW_BTN_LEFT:
            move_cursor(*state, -static_cast<int>(visible_rows()));
            break;
        case SW_BTN_RIGHT:
            move_cursor(*state, visible_rows());
            break;
        case SW_BTN_A:
            if (books.empty())
            {
                open_add_menu(*state);
            }
            else if (state->cursor < books.size() && books[state->cursor].status != "missing")
            {
                state->open_book(books[state->cursor].path);
            }
            break;
        case SW_BTN_SELECT:
            open_add_menu(*state);
            break;
        case SW_BTN_Y:
            open_delete_confirm(*state);
            break;
        case SW_BTN_B:
            state->is_done = true;
            break;
        default:
            break;
    }
}

void LibraryView::on_keyheld(SDLKey key, uint32_t held_time_ms)
{
    switch (key)
    {
        case SW_BTN_UP:
        case SW_BTN_DOWN:
        case SW_BTN_LEFT:
        case SW_BTN_RIGHT:
            if (state->scroll_throttle(held_time_ms))
            {
                on_keypress(key);
            }
            break;
        default:
            break;
    }
}

void LibraryView::on_focus()
{
    state->library_store.load();
    if (state->cursor >= state->library_store.get_books().size())
    {
        state->cursor = 0;
        state->scroll = 0;
    }
    state->needs_render = true;
}
