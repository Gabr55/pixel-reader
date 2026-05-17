#ifndef LIBRARY_VIEW_H_
#define LIBRARY_VIEW_H_

#include "reader/library_store.h"
#include "reader/view.h"

#include <filesystem>
#include <functional>
#include <memory>

struct LibraryViewState;
struct SystemStyling;
class ViewStack;

class LibraryView: public View
{
    std::unique_ptr<LibraryViewState> state;

public:
    LibraryView(
        LibraryStore &library_store,
        SystemStyling &sys_styling,
        ViewStack &view_stack,
        std::function<void(std::filesystem::path)> open_book
    );
    virtual ~LibraryView();

    bool render(SDL_Surface *dest_surface, bool force_render) override;
    bool is_done() override;
    void on_keypress(SDLKey key) override;
    void on_keyheld(SDLKey key, uint32_t held_time_ms) override;
    void on_focus() override;
};

#endif
