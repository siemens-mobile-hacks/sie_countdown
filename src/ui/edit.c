#include <stdio.h>
#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

typedef struct {
    GUI *main_gui;
    int item_id;
} EDIT_DATA;

extern HEADER_DESC HEADER_D;

static SOFTKEY_DESC SOFTKEYS_D[] = {
    {0x0018, 0x0000, (int)"Save"},
    {0x0018, 0x0000, (int)LGP_SAVE_PIC},
    {0x0001, 0x0000, (int)"Back"},
};

static const SOFTKEYSTAB SOFTKEYS_TAB = {
    SOFTKEYS_D, 0,
};

int Save(GUI *gui) {
    EDIT_DATA *data = EDIT_GetUserPointer(gui);
    UI_DATA *ui_data = EDIT_GetUserPointer(data->main_gui);

    TTime time;
    EDITCONTROL ec;
    ExtractEditControl(gui, 2, &ec);
    EDIT_GetTime(gui, 4, &time);

    if (!wstrlen(ec.pWS)) {
        MsgBoxError(1, (int)"Name can't be empty!");
        return 0;
    }
    int milliseconds = GetSecondsFromTime(&time) * 1000;
    if (!milliseconds) {
        MsgBoxError(1, (int)"Time can't be equal to zero!");
        return 0;
    }

    wstrcpy(ui_data->countdown->names[data->item_id], ec.pWS);
    ui_data->countdown->values[data->item_id] = milliseconds;
    EDIT_SetTime(data->main_gui, 1, &time);

    return 1;
}

static int OnKey(GUI *gui, GUI_MSG *msg) {
    if (msg->keys == 0x18) {
        return Save(gui);
    } else if (msg->keys == 0x01) {
        return 1;
    }
    return 0;
}

static void GHook(GUI *gui, int cmd) {
    EDIT_DATA *data = EDIT_GetUserPointer(gui);

    if (cmd == TI_CMD_REDRAW) {
        SetSoftKey(gui, &SOFTKEYS_D[0], SET_LEFT_SOFTKEY);
        if (EDIT_GetFocus(gui) != 2 || EDIT_GetCursorPos(gui) == 1) {
            SetSoftKey(gui, &SOFTKEYS_D[2], SET_RIGHT_SOFTKEY);
        }
        if (EDIT_GetFocus(gui) == 4) {
            SetSoftKey(gui, &SOFTKEYS_D[1], SET_MIDDLE_SOFTKEY);
        }
    } else if (cmd == TI_CMD_DESTROY) {
        mfree(data);
    }
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
    0,
    INPUTDIA_FLAGS_SWAP_SOFTKEYS,
};

int Edit_CreateUI(GUI *main_gui) {
    memcpy(&(INPUTDIA_D.rc), GetMainAreaRECT(), sizeof(RECT));

    UI_DATA *ui_data = EDIT_GetUserPointer(main_gui);
    EDIT_DATA *data = malloc(sizeof(EDIT_DATA));
    zeromem(data, sizeof(EDIT_DATA));
    data->main_gui = main_gui;
    data->item_id = GetCurMenuItem(ui_data->menu);

    void *ma = malloc_adr();
    EDITQ *eq = AllocEQueue(ma, mfree_adr());

    WSHDR ws;
    uint16_t wsbody[128];
    CreateLocalWS(&ws, wsbody, 127);

    EDITCONTROL ec;
    PrepareEditControl(&ec);

    wsprintf(&ws, "%s", "Name:")
    ConstructEditControl(&ec, ECT_HEADER, ECF_APPEND_EOL, &ws, wstrlen(&ws));
    AddEditControlToEditQend(eq, &ec, ma);

    ConstructEditControl(&ec, ECT_NORMAL_TEXT, ECF_APPEND_EOL, ui_data->countdown->names[data->item_id], 127);
    AddEditControlToEditQend(eq, &ec, ma);

    wsprintf(&ws, "%s", "Time:")
    ConstructEditControl(&ec, ECT_HEADER, ECF_APPEND_EOL, &ws, wstrlen(&ws));
    AddEditControlToEditQend(eq, &ec, ma);

    TTime time;
    GetTimeFromMilliseconds(&time, ui_data->countdown->values[data->item_id]);
    ConstructComboBox(&ec, ECT_TIME, ECF_APPEND_EOL, NULL, 0, 0, 4, 0);
    ConstructEditTime(&ec, &time);
    AddEditControlToEditQend(eq, &ec, ma);

    return CreateInputTextDialog(&INPUTDIA_D, &HEADER_D, eq, 1, data);
}