#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#include "../ipc.h"
#include "../countdown.h"
#include "icons.h"
#include "ui.h"

typedef struct {
    GUI *main_gui;
    WSHDR *ws;
    int timer;
    int gui_id;
    int animation_icon_id;
} TIMER_DATA;

extern int CSM_ID;
extern HEADER_DESC HEADER_D;

const int ANIMATION_ICONS[ANIMATION_ICONS_COUNT] = {ICON_ANIMATION_1, ICON_ANIMATION_2, ICON_ANIMATION_3, ICON_ANIMATION_4, ICON_ANIMATION_5};

const int SOFTKEYS[] = {SET_LEFT_SOFTKEY, SET_MIDDLE_SOFTKEY, SET_RIGHT_SOFTKEY};

static SOFTKEY_DESC SOFTKEYS_D[] = {
    {0x0001, 0x0000, (int)"Stop"},
    {0x0001, 0x0000, (int)LGP_STOP_PIC},
    {0x00FF, 0x0000, (int)"Exit"},
};

static const SOFTKEYSTAB SOFTKEYS_TAB = {
    SOFTKEYS_D, 3,
};

void GetText(void *gui, int remaning_time) {
    TIMER_DATA *data = TViewGetUserPointer(gui);

    if (data->animation_icon_id >= ANIMATION_ICONS_COUNT) {
        data->animation_icon_id = 0;
    }

    TTime time;
    GetTimeFromMilliseconds(&time, remaning_time);
    wsprintf(data->ws, "%02d:%02d:%02d\n\n%c", time.hour, time.min, time.sec,
             GetUnicodeSymbolByDynIcon(20010 + data->animation_icon_id));

    data->animation_icon_id++;
}

void TimerProc(void *gui) {
    TIMER_DATA *data = TViewGetUserPointer(gui);

    int remaining_time = Countdown_GetRemainingTime();
    GetText(gui, remaining_time);
    DirectRedrawGUI();

    if (remaining_time < 1000) {
        Countdown_PlaySound(COUNTDOWN_TONE_ALERT);
        GeneralFuncF1(1);
        return;
    } else if (remaining_time < 4000) {
        static IPC_REQ ipc;
        ipc.data = (void*)CSM_ID;
        IPC_SendToXTask(&ipc, IPC_XTASK_SHOW_CSM);
        Countdown_PlaySound(COUNTDOWN_TONE_BEEP);
    }
    GUI_StartTimerProc(gui, data->timer, 1000, TimerProc);
}

static int OnKey(GUI *gui, GUI_MSG *msg) {
    TIMER_DATA *data = TViewGetUserPointer(gui);
    UI_DATA *ui_data = EDIT_GetUserPointer(data->main_gui);

    if (msg->keys == 0x01) {
        ui_data->countdown->manual_time = Countdown_GetRemainingTime();
        Countdown_Stop();
        SetTime(data->main_gui, ui_data->countdown->manual_time);
        return 1;
    } else if (msg->keys == 0xFF) {
        CloseCSM(CSM_ID);
    }
    return -1;
}

void FreeDynIcons() {
    for (int i = 0; i < ANIMATION_ICONS_COUNT; i++) {
        FreeDynIcon(20010 + i);
    }
}

static void GHook(GUI *gui, int cmd) {
    TIMER_DATA *data = TViewGetUserPointer(gui);

    if (cmd == TI_CMD_CREATE) {
        *RamIsCountdownRingUIActive() = 1;
        data->timer = GUI_NewTimer(gui);
        TimerProc(gui);
    }
    else if (cmd == TI_CMD_FOCUS) {
        if (!Countdown_IsEnabled()) {
            GeneralFunc_flag1(data->gui_id, 1);
        } else {
            for (int i = 0; i < ANIMATION_ICONS_COUNT; i++) {
                IMGHDR *img = GetPITaddr(ANIMATION_ICONS[i]);
                SetDynIcon(20010 + i, img, (char*)img->bitmap);
            }
        }
    } else if (cmd == TI_CMD_UNFOCUS) {
        FreeDynIcons();
    } else if (cmd == TI_CMD_DESTROY) {
        *RamIsCountdownRingUIActive() = 0;
        GUI_DeleteTimer(gui, data->timer);
        FreeDynIcons();
        mfree(data);
    }
}

static TVIEW_DESC TVIEW_D = {
    8,
    OnKey,
    GHook,
    NULL,
    SOFTKEYS,
    &SOFTKEYS_TAB,
    {0, 0, 0, 0},
    FONT_MEDIUM_BOLD,
    0x64,
    0x65,
    0,
    TEXT_ALIGNMIDDLE,
};

int Timer_CreateUI(GUI *main_gui) {
    memcpy(&(TVIEW_D.rc), GetMainAreaRECT(), sizeof(RECT));
    TVIEW_D.rc.y = 0x4C;

    TIMER_DATA *data = malloc(sizeof(TIMER_DATA));
    zeromem(data, sizeof(TIMER_DATA));
    data->main_gui = main_gui;
    data->ws = AllocWS(32);

    void *ma = malloc_adr();
    void *mf = mfree_adr();

    GUI *gui = TViewGetGUI(ma, mf);
    TViewSetDefinition(gui, &TVIEW_D);
    TViewSetUserPointer(gui, data);
    SetHeader(gui, &HEADER_D, ma);
    TViewSetText(gui, data->ws, ma, mf);

    data->gui_id = CreateGUI(gui);
    return data->gui_id;
}
