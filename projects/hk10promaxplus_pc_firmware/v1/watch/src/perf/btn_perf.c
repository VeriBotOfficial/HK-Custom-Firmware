#include "./btn_perf.h"

#ifdef _WIN32

int32_t perf_button_register(uint32_t pin, uint8_t active_poll) { return 0; }
int perf_button_add_trigger(int32_t handle, button_trigger_cb_t cb) { return 0; }

#else

static button_entry_t buttons[MAX_BUTTONS];
static int button_count = 0;

int perf_button_add_trigger(int32_t handle, button_trigger_cb_t cb)
{
    if (handle < 0 || handle >= button_count) return -1;
    button_entry_t *entry = &buttons[handle];

    if (entry->trigger_count >= MAX_TRIGGERS_PER_BUTTON) return -1;

    entry->triggers[entry->trigger_count++] = cb;
    return 0;
}

static void default_button_callback(void *btn, button_action_t action)
{
    for (int i = 0; i < button_count; i++)
    {
        button_entry_t *entry = &buttons[i];
        if (entry->handle == (int32_t)btn)
        {
            for (int j = 0; j < entry->trigger_count; j++)
            {
                if (entry->triggers[j])
                    entry->triggers[j](action);
            }
            break;
        }
    }
}

int32_t perf_button_register(uint32_t pin, uint8_t active_poll)
{
    if (button_count >= MAX_BUTTONS) return -1;

    button_t *btn = button_create(pin, active_poll);
    if (!btn) return -1;

    button_bind_callback(btn, BUTTON_PRESSED, default_button_callback);
    button_bind_callback(btn, BUTTON_RELEASED, default_button_callback);

    buttons[button_count].pin = pin;
    buttons[button_count].handle = (int32_t)btn;
    buttons[button_count].trigger_count = 0;

    return button_count++;
}

#endif