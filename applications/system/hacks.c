#include <furi.h>
#include <gui/gui.h>
#include <notification/notification_messages.h>
#include <input/input.h>

#define PROGRESS_DURATION 4000
#define TICK_INTERVAL 100
#define BLINK_INTERVAL 500

typedef struct {
    float progress;
    uint32_t start_time;
    uint32_t last_blink_time;
    bool finished;
} AppState;

static void app_draw_callback(Canvas* canvas, void* ctx) {
    AppState* state = (AppState*)ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if (state->finished) {
        const char* message = "Successful!";
        uint8_t x = (canvas_width(canvas) - canvas_string_width(canvas, message)) / 2;
        canvas_draw_str(canvas, x, canvas_height(canvas) / 2, message);
    } else {
        const char* title = "Process Hacking..";
        uint8_t x = (canvas_width(canvas) - canvas_string_width(canvas, title)) / 2;
        canvas_draw_str(canvas, x, canvas_height(canvas) / 2 - 10, title);

        uint8_t bar_width = canvas_width(canvas) * state->progress;
        canvas_draw_box(canvas, 1, canvas_height(canvas) - 25, bar_width, 6);
    }
}

static void generate_sounds(NotificationApp* notifications) {
    char notificationString[20];
    snprintf(notificationString, sizeof(notificationString), "Generate sound");

    notification_message(notifications, &sequence_success);
    furi_delay_ms(1000);
    notification_message(notifications, &sequence_error);
    furi_delay_ms(1000);

    const NotificationSequence my_sequence = {
        &message_note_e4, // Нота E4
        &message_delay_500, // Пауза 500 мс
        &message_note_c4, // Нота C4
        &message_delay_500, // Пауза 500 мс
        &message_note_g3, // Нота G3
        &message_delay_500, // Пауза 500 мс
        &message_note_a3, // Нота A3
        NULL,   
    };

    notification_message(notifications, &my_sequence);
}

int main(void) {
    AppState state;
    state.progress = 0.0f;
    state.start_time = furi_get_tick();
    state.last_blink_time = furi_get_tick();
    state.finished = false;

    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
    if (notification == NULL) {
        FURI_LOG_E("app", "furi_record_open(RECORD_NOTIFICATION) failed");
        return 255;
    }

    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, app_draw_callback, &state);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    bool led_on = true;

    while (state.progress < 1.0f) {
        uint32_t elapsed_time = furi_get_tick() - state.start_time;
        state.progress = (float)elapsed_time / PROGRESS_DURATION;
        if (state.progress > 1.0f) {
            state.progress = 1.0f;
        }

        uint32_t elapsed_blink_time = furi_get_tick() - state.last_blink_time;
        if (elapsed_blink_time >= BLINK_INTERVAL) {
            if (led_on) {
                notification_message(notification, &sequence_blink_start_red);
            } else {
                notification_message(notification, &sequence_blink_stop);
            }
            led_on = !led_on;
            state.last_blink_time = furi_get_tick();
        }

        view_port_update(view_port);
        furi_delay_ms(TICK_INTERVAL);
    }

    state.finished = true;
    view_port_update(view_port);

    notification_message(notification, &sequence_blink_start_green);
    generate_sounds(notification);
    furi_delay_ms(1000);
    notification_message(notification, &sequence_blink_stop);

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    return 0;
}