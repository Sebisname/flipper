#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

static void draw_callback(Canvas* canvas, void* context){
    UNUSED(context);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 30, 30, "Hello World");
}

static void input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, input_event, FuriWaitForever);
}

int32_t hello_world_main(void* p) {
    UNUSED(p);

    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, NULL);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    view_port_input_callback_set(view_port, input_callback, event_queue);
    
    InputEvent event;
    bool running = true;

    while(running) {
    if(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk) {
        if(event.type == InputTypeShort) {
            if(event.key == InputKeyBack) {
                running = false; 
                }
            }
        }
    }

    furi_message_queue_free(event_queue); 

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    
    return 0;
}



