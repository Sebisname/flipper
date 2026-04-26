#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <cookie_clicker_icons.h>


typedef struct {
    const char* name;
    int cost;
    int cps;
    int owned;
} ShopItem;

static ShopItem shop_items[] = {
    {"Auto Clicker",  10,   1,  0},
    {"Grandma",       100,  5,  0},
    {"Farm",          500,  20, 0},
};

#define SHOP_ITEM_COUNT 3

typedef enum {
    ScreenGame,
    ScreenShop,
} AppScreen;

typedef struct {
    int cookies;
    bool is_eating;
    AppScreen screen;
    int shop_cursor;
    int cps_total;
    ViewPort* view_port;
} AppState;


static void timer_callback(void* context) {
    AppState* state = context;
    state->cookies += state->cps_total;
    view_port_update(state->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    AppState* state = context;
    canvas_clear(canvas);

    if(state->screen == ScreenGame) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Cookies: %d", state->cookies);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, buffer);

        if(state->cps_total > 0) {
            char cps_buf[24];
            snprintf(cps_buf, sizeof(cps_buf), "CPS: %d", state->cps_total);
            canvas_draw_str_aligned(canvas, 64, 12, AlignCenter, AlignTop, cps_buf);
        }

       
        if(state->is_eating) {
            canvas_draw_icon(canvas, 48, 22, &I_icon_cookie_eat);
        } else {
            canvas_draw_icon(canvas, 48, 22, &I_icon_cookie);
        }

        // Pfeil zum Shop
        canvas_draw_icon(canvas, 0, 48, &I_arrow_down);
        canvas_draw_str(canvas, 18, 58, "Shop");

    } else if(state->screen == ScreenShop) {
        // Titel
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "Shop");

        canvas_set_font(canvas, FontSecondary);

        // Cookie Anzeige
        char buf[32];
        snprintf(buf, sizeof(buf), "Cookies: %d", state->cookies);
        canvas_draw_str_aligned(canvas, 64, 14, AlignCenter, AlignTop, buf);

        // Items
        for(int i = 0; i < SHOP_ITEM_COUNT; i++) {
            int y = 32 + (i * 13);

            // Cursor
            if(i == state->shop_cursor) {
                canvas_draw_str(canvas, 0, y, ">");
            }

            // Name und Preis
            char item_buf[32];
            snprintf(
                item_buf,
                sizeof(item_buf),
                "%s %d$",
                shop_items[i].name,
                shop_items[i].cost);
            canvas_draw_str(canvas, 8, y, item_buf);

            // Anzahl gekauft
            char owned_buf[8];
            snprintf(owned_buf, sizeof(owned_buf), "[%d]", shop_items[i].owned);
            canvas_draw_str(canvas, 110, y, owned_buf);
        }

    }
}

static void input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, input_event, FuriWaitForever);
}

int32_t cookie_clicker_main(void* p) {
    UNUSED(p);

    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    AppState state;
    state.cookies = 0;
    state.is_eating = false;
    state.screen = ScreenGame;
    state.shop_cursor = 0;
    state.cps_total = 0;
    state.view_port = view_port;

    view_port_draw_callback_set(view_port, draw_callback, &state);
    view_port_input_callback_set(view_port, input_callback, event_queue);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Timer
    FuriTimer* timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, &state);
    furi_timer_start(timer, 1000);

    InputEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk) {
            if(event.type == InputTypeShort) {

                if(state.screen == ScreenGame) {
                    if(event.key == InputKeyOk) {
                        state.cookies++;
                        state.is_eating = true;
                        view_port_update(view_port);

                        furi_delay_ms(200);

                        state.is_eating = false;
                        view_port_update(view_port);
                    }
                    if(event.key == InputKeyDown) {
                        state.screen = ScreenShop;
                        state.shop_cursor = 0;
                        view_port_update(view_port);
                    }
                    if(event.key == InputKeyBack) {
                        running = false;
                    }

                } else if(state.screen == ScreenShop) {
                    if(event.key == InputKeyUp) {
                        if(state.shop_cursor > 0) {
                            state.shop_cursor--;
                            view_port_update(view_port);
                        }
                    }
                    if(event.key == InputKeyDown) {
                        if(state.shop_cursor < SHOP_ITEM_COUNT - 1) {
                            state.shop_cursor++;
                            view_port_update(view_port);
                        }
                    }
                    if(event.key == InputKeyOk) {
                        ShopItem* item = &shop_items[state.shop_cursor];
                        if(state.cookies >= item->cost) {
                            state.cookies -= item->cost;
                            item->owned++;
                            item->cost = item->cost * 2;
                            state.cps_total += item->cps;
                            view_port_update(view_port);
                        }
                    }
                    if(event.key == InputKeyBack) {
                        state.screen = ScreenGame;
                        view_port_update(view_port);
                    }
                }
            }
        }
    }

    // Cleanup
    furi_timer_stop(timer);
    furi_timer_free(timer);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    return 0;
}