#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "countdown.h"

#define COUNTDOWN_PD_ID 0x1F0C

void SetDefault(COUNTDOWN *countdown, int id, const char *name, int value) {
    if (wstrlen(countdown->names[id]) == 0) {
        wsprintf(countdown->names[id], "%s", name);
    }
    if (countdown->values[id] == 0) {
        countdown->values[id] = value;
    }
}

COUNTDOWN *ReadPDFile() {
    COUNTDOWN *countdown = malloc(sizeof(COUNTDOWN));
    zeromem(countdown, sizeof(COUNTDOWN));

    char key[32];
    char value[128];
    size_t len;

    len = 127;
    sprintf(key, "%s", "manual_time");
    if (ReadValueFromPDFile(COUNTDOWN_PD_ID, key, value, &len) == 0) {
        countdown->manual_time = atoi(value);
    } else {
        countdown->manual_time = 60000;
    }

    len = 127;
    sprintf(key, "%s", "reset_time");
    if (ReadValueFromPDFile(COUNTDOWN_PD_ID, key, value, &len) == 0) {
        countdown->reset_time = atoi(value);
    } else {
        countdown->reset_time = 0;
    }
    if (countdown->reset_time == 0) {
        countdown->reset_time = 60000; // 1m
    }

    for (int i = 0; i < COUNTDOWN_MAX_ITEMS; i++) {
        len = 127;
        sprintf(key, "name_%d", i + 1);
        countdown->names[i] = AllocWS(128);
        if (ReadValueFromPDFile(COUNTDOWN_PD_ID, key, value, &len) == 0) {
            value[len] = '\0';
            utf8_2ws(countdown->names[i], value, 127);
        }
        len = 127;
        sprintf(key, "value_%d", i + 1);
        if (ReadValueFromPDFile(COUNTDOWN_PD_ID, key, value, &len) == 0) {
            value[len] = '\0';
            countdown->values[i] = atoi(value);
        } else {
            countdown->values[i] = 0;
        }
    }

    SetDefault(countdown, 0, "Eggs", 60000 * 5); // 5m
    SetDefault(countdown, 1, "Rice", 60000 * 20); //20m
    SetDefault(countdown, 2, "Vegetables", 60000 * 15); // 15m
    SetDefault(countdown, 3, "Pasta", 60000 * 10); // 10m
    SetDefault(countdown, 4, "Potatoes", 60000 * 25); // 25m

    return countdown;
}

int SavePDFile(const COUNTDOWN *countdown, uint64_t end_time) {
    uint32_t err;
    const char *path = "1:\\system\\countdown.pd";

    int fp = sys_open(path, A_WriteOnly | A_Truncate | A_Create | A_TXT, P_WRITE, &err);
    if (fp != -1) {
        size_t len;
        char line[256];
        char value[192];

        sprintf(value, "%d", countdown->manual_time);
        len = 21 + strlen(value);
        sprintf(line, "%06d:T:manual_time=%s\r\n", len, value);
        sys_write(fp, line, len + 2, &err);

        sprintf(value, "%d", countdown->reset_time);
        len = 20 + strlen(value);
        sprintf(line, "%06d:T:reset_time=%s\r\n", len, value);
        sys_write(fp, line, len + 2, &err);

        int new_length = 0;
        for (int i = 0; i< COUNTDOWN_MAX_ITEMS; i++) {
            ws_2utf8(countdown->names[i], value, &new_length, 191);
            len = 16 + strlen(value);
            sprintf(line, "%06d:T:name_%d=%s\r\n", len, i + 1, value);
            sys_write(fp, line, len + 2, &err);

            sprintf(value, "%d", countdown->values[i]);
            len = 17 + strlen(value);
            sprintf(line, "%06d:T:value_%d=%s\r\n", len, i + 1, value);
            sys_write(fp, line, len + 2, &err);
        }

        if (end_time) {
            sprintf(value, "%llu", end_time);
            len = 18 + strlen(value);
            sprintf(line, "%06d:T:end_time=%s\r\n", len, value);
            sys_write(fp, line, len + 2, &err);
        }

        sys_close(fp, &err);
        return 1;
    }
    return 0;
}

void DestroyCountdown(COUNTDOWN *countdown) {
    for (int i = 0; i < COUNTDOWN_MAX_ITEMS; i++) {
        FreeWS(countdown->names[i]);
    }
    mfree(countdown);
}

void StartCountdown(COUNTDOWN *countdown, TTime *timer) {
    TTime time;
    TDate date;
    TDate epoch;
    GetDateTime(&date, &time);
    InitDate(&epoch, 1970, 1, 1);

    int secs = 0, seconds = GetSecondsFromTime(timer);
    GetSecondsFromDateTime(&secs, &date, &time, &epoch);
    secs -= GetTimeZoneShift(&date, &time, GetCurrentTimeZone()) * 60;

    uint64_t end_time = secs + seconds;
    end_time *= 1000;

    countdown->reset_time = seconds * 1000;

    SavePDFile(countdown, end_time);
    Countdown_Start();
}
