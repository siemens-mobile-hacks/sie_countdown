#pragma once

#include "../countdown.h"

typedef struct {
    WIDGET *menu;
    COUNTDOWN *countdown;
} UI_DATA;

int CreateUI(COUNTDOWN *countdown);
void SetTime(GUI *gui, int milliseconds);
