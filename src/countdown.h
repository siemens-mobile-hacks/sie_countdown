#pragma once

#define COUNTDOWN_MAX_ITEMS 5

typedef struct {
    int reset_time;
    int manual_time;
    WSHDR *names[COUNTDOWN_MAX_ITEMS];
    int values[COUNTDOWN_MAX_ITEMS];
} COUNTDOWN;

COUNTDOWN *ReadPDFile();
int SavePDFile(const COUNTDOWN *countdown, uint64_t end_time);
void DestroyCountdown(COUNTDOWN *countdown);

void StartCountdown(COUNTDOWN *countdown, TTime *timer);
