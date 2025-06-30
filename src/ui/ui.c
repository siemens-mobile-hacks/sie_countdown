#include <swilib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "edit.h"
#include "timer.h"
#include "icons.h"

#define MENU_ITEMS     COUNTDOWN_MAX_ITEMS
#define MENU_WIDGET_ID 20

GUI_METHODS METHODS;

int HEADER_ICON[] = {ICON_COUNTDOWN};

HEADER_DESC HEADER_D = {{0, 0, 0, 0}, HEADER_ICON, (int)"Countdown", LGP_NULL};

static SOFTKEY_DESC SOFTKEYS_D[] = {
    {0x0018, 0x0000, (int)"Change"},
    {0x0001, 0x0000, (int)"Exit"},
    {0x0A01, 0x0000, (int)"Start"},
    {0x0A02, 0x0000, (int)"Reset"},
    {0x0A01, 0x0000, (int)LGP_PLAY_PIC},
};

static const SOFTKEYSTAB SOFTKEYS_TAB = {
    SOFTKEYS_D, 0,
};

static void OnRedraw(GUI *gui) {
    WIDGET *menu = GetDataOfItemByID(gui, MENU_WIDGET_ID);
    if (menu) {
        menu->methods->onRedraw(menu);
    }
    WIDGET *input = GetDataOfItemByID(GetDataOfItemByID(gui, 4), 4);
    input->methods->onRedraw(input);

    INPUTDIA_DESC *desc = gui->definition;
    if (desc->global_hook_proc) {
        desc->global_hook_proc(gui, TI_CMD_REDRAW);
    }
    for (int i = 0; i < 3; i++) {
        WIDGET *widget = GetDataOfItemByID(gui, i);
        if (widget) {
            widget->methods->onRedraw(widget);
        }
    }
    int y = input->canvas->y + 46;
    DrawLine(input->canvas->x, y, input->canvas->x2, y, 0, GetPaletteAdrByColorIndex(PC_FOREGROUND));
}

#define IsFocusEdit(gui) (EDIT_GetFocus(gui) == 1)
#define UnFocusEdit(gui) EDIT_SetFocus(gui, 2)

void FocusEdit(GUI *gui) {
    UI_DATA *data = EDIT_GetUserPointer(gui);
    TTime time;
    GetTimeFromMilliseconds(&time, data->countdown->manual_time);
    EDIT_SetTime(gui, 1, &time);
    EDIT_SetFocus(gui, 1);
}

__attribute__((always_inline)) inline int IsFocusMenu(void *menu) {
    return (*(int *)((int)menu + 0x1b0) == 0);
}

__attribute__((always_inline)) inline void FocusMenu(void *menu) {
    *(int *)((int)menu + 0x1b0) = 0;
}

__attribute__((always_inline)) inline void UnFocusMenu(void *menu) {
    *(int *)((int)menu + 0x1b0) = 1;
}

void SetTime(GUI *gui, int milliseconds) {
    TTime time;
    GetTimeFromMilliseconds(&time, milliseconds);
    EDIT_SetTime(gui, 1, &time);
}

static int OnKey(GUI *gui, GUI_MSG *msg) {
    UI_DATA *data = EDIT_GetUserPointer(gui);

    if (msg->keys == 0xA01) {
        TTime time;
        EDIT_GetTime(gui, 1, &time);
        StartCountdown(data->countdown, &time);
        Timer_CreateUI(gui);
    } else if (msg->keys == 0xA02) {
        data->countdown->manual_time = data->countdown->reset_time;
        SetTime(gui, data->countdown->manual_time);
        RefreshGUI();
    } else if (msg->keys == 0x18) {
        Edit_CreateUI(gui);
    } else if (msg->keys == 0x01) {
        return 1;
    } else if (msg->gbsmsg->msg == KEY_DOWN || msg->gbsmsg->msg == LONG_PRESS) {
        if (msg->gbsmsg->submess == UP_BUTTON) {
            int cursor = GetCurMenuItem(data->menu) - 1;
            if (IsFocusEdit(gui)) {
                cursor = MENU_ITEMS - 1;
                UnFocusEdit(gui);
                FocusMenu(data->menu);
            } else if (cursor < 0) {
                cursor = 0;
                UnFocusMenu(data->menu);
                FocusEdit(gui);
            }
            SetCursorToMenuItem(data->menu, cursor);
            RefreshGUI();
            return -1;
        } else if (msg->gbsmsg->submess == DOWN_BUTTON) {
            int cursor = GetCurMenuItem(data->menu) + 1;
            if (IsFocusEdit(gui)) {
                cursor = 0;
                UnFocusEdit(gui);
                FocusMenu(data->menu);
            } else if (cursor >= MENU_ITEMS) {
                cursor = 0;
                UnFocusMenu(data->menu);
                FocusEdit(gui);
            }
            SetCursorToMenuItem(data->menu, cursor);
            RefreshGUI();
            return -1;
        }
    }
    return 0;
}

static void GHook(GUI *gui, int cmd) {
    UI_DATA *data = EDIT_GetUserPointer(gui);
    if (cmd == TI_CMD_REDRAW) {
        if (!IsFocusMenu(data->menu)) {
            TTime time;
            EDIT_GetTime(gui, 1, &time);
            data->countdown->manual_time = GetSecondsFromTime(&time) * 1000;
            if (data->countdown->manual_time != data->countdown->reset_time) {
                SetSoftKey(gui, &SOFTKEYS_D[3], SET_LEFT_SOFTKEY);
            } else {
                SetSoftKey(gui, &SOFTKEYS_D[2], SET_LEFT_SOFTKEY);
            }
        } else {
            int cursor = GetCurMenuItem(data->menu);
            SetTime(gui, data->countdown->values[cursor]);
            SetSoftKey(gui, &SOFTKEYS_D[0], SET_LEFT_SOFTKEY);
        }
        SetSoftKey(gui, &SOFTKEYS_D[4], SET_MIDDLE_SOFTKEY);
        SetSoftKey(gui, &SOFTKEYS_D[1], SET_RIGHT_SOFTKEY);
    }
    else if (cmd == TI_CMD_CREATE) {
        WIDGET *menu = GetDataOfItemByID(gui, MENU_WIDGET_ID);
        UnFocusMenu(menu);
    }
    else if (cmd == TI_CMD_FOCUS) {
        if (Countdown_IsEnabled()) {
            Timer_CreateUI(gui);
        }
    } else if (cmd == TI_CMD_DESTROY) {
        mfree(data);
    }
}

void ItemProc(void *gui, int item_n, void *user_pointer) {
    UI_DATA *data = user_pointer;
    static int icons[] = {ICON_EMPTY};

    void *item = AllocMenuItem(gui);
    WSHDR *ws = AllocMenuWS(gui, 128);
    wsprintf(ws, "%w", data->countdown->names[item_n]);
    SetMenuItemIconArray(gui, item, icons);
    SetMenuItemIcon(gui, item_n, 0);
    SetMenuItemText(gui, item, ws, item_n);
}

static INPUTDIA_DESC INPUTDIA_D = {
    1,
    OnKey,
    GHook,
    NULL,
    0,
    &SOFTKEYS_TAB,
    {0, 0, 0, 0},
    FONT_MEDIUM,
    0x64,
    0x65,
    0,
    TEXT_ALIGNMIDDLE,
    INPUTDIA_FLAGS_SWAP_SOFTKEYS,
};

static const MENU_DESC MENU_D = {
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    MENU_FLAGS_ENABLE_ICONS,
    ItemProc,
    NULL,
    NULL,
    MENU_ITEMS,
};

int CreateUI(COUNTDOWN *countdown) {
    RECT *main_area_rect = GetMainAreaRECT();

    memcpy(&(HEADER_D.rc), GetHeaderRECT(), sizeof(RECT));
    memcpy(&(INPUTDIA_D.rc), main_area_rect, sizeof(RECT));

    UI_DATA *data = malloc(sizeof(UI_DATA));
    zeromem(data, sizeof(UI_DATA));
    data->countdown = countdown;

    void *ma = malloc_adr();
    void *mf = mfree_adr();
    EDITQ *eq = AllocEQueue(ma, mf);

    WSHDR ws;
    uint16_t wsbody[128];
    CreateLocalWS(&ws, wsbody, 127);

    EDITCONTROL ec;
    PrepareEditControl(&ec);

    TTime time;
    GetTimeFromMilliseconds(&time, countdown->manual_time);

    ConstructComboBox(&ec, ECT_TIME, ECF_APPEND_EOL, NULL, 0, 0, 4, 0);
    ConstructEditTime(&ec, &time);
    AddEditControlToEditQend(eq, &ec, ma);

    ConstructEditControl(&ec, ECT_READ_ONLY, ECF_NORMAL_STR, &ws, 0);
    AddEditControlToEditQend(eq, &ec, ma);

    GUI *gui = EDIT_GetGUI(ma, mf);
    EDIT_SetDefinition(gui, &INPUTDIA_D);
    EDIT_SetEQueue(gui, eq, 1, ma);
    EDIT_SetUserPointer(gui, data);
    SetHeader(gui, &HEADER_D, ma);

    data->menu = GetMenuGUI(ma, mf);
    SetMenuToGUI(data->menu, &MENU_D);
    MenuSetUserPointer(data->menu, data);
    SetMenuRect(data->menu, main_area_rect->x, main_area_rect->y + 58, main_area_rect->x2, main_area_rect->y2);
    AttachWidget(gui, data->menu, MENU_WIDGET_ID, ma);

    memcpy(&METHODS, gui->methods, sizeof(GUI_METHODS));
    METHODS.onRedraw = OnRedraw;
    gui->methods = &METHODS;

    return CreateGUI(gui);
}
