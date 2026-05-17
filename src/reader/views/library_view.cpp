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

#include <algorithm>

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

constexpr int HEADER_H = 32;
constexpr int FOOTER_H = 26;
constexpr int ROW_H = 74;
constexpr int PAD = 8;

uint32_t visible_rows()
{
    return (SCREEN_HEIGHT - HEADER_H - FOOTER_H) / ROW_H;
}

std::string progress_label(const LibraryBook &book)
{
    if (book.status == "missing")
    {
        return "MISS";
    }
    if (book.progress == 0)
    {
        return "NEW";
    }
    if (book.progress >= 98)
    {
        return "DONE";
    }
    return std::to_string(book.progress) + "%";
}

void draw_text(SDL_Surface *dest, TTF_Font *font, const std::string &text, Sint16 x, Sint16 y, SDL_Color fg, SDL_Color bg)
{
    auto surface = surface_unique_ptr { TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg) };
    SDL_Rect rect = {x, y, 0, 0};
    SDL_BlitSurface(surface.get(), nullptr, dest, &rect);
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
        std::vector<std::string>{"Add file", "Add folder", "Search (later)"},
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
            s.view_stack.push(picker);
        }
        else if (index == 1)
        {
            auto picker = std::make_shared<FileSelector>(DEFAULT_BROWSE_PATH, s.sys_styling);
            picker->set_on_folder_selected([&s](const std::filesystem::path &path) {
                s.library_store.add_folder(path);
                s.needs_render = true;
            });
            s.view_stack.push(picker);
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

    draw_text(dest_surface, font, "Library", PAD, 5, theme.secondary_text, theme.background);

    const auto &books = state->library_store.get_books();
    if (books.empty())
    {
        draw_text(dest_surface, font, "+ Add books", PAD, HEADER_H + 40, theme.main_text, theme.background);
        draw_text(dest_surface, font, "A add  Select menu", PAD, SCREEN_HEIGHT - FOOTER_H + 4, theme.secondary_text, theme.background);
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

        draw_text(dest_surface, font, book.title, PAD, y + 8, text, row_bg);
        draw_text(dest_surface, font, author, PAD, y + 36, theme.secondary_text, row_bg);
        draw_text(dest_surface, font, progress, SCREEN_WIDTH - 78, y + 22, text, row_bg);
    }

    draw_text(dest_surface, font, "A open  Select add/search  B exit", PAD, SCREEN_HEIGHT - FOOTER_H + 4, theme.secondary_text, theme.background);
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
