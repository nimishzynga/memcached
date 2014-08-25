/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "config.h"
#include "timings.h"
#include <memcached/protocol_binary.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

typedef struct timings_st {
    /* We collect timings for <=1 us */
    uint32_t ns;

    /* We collect timings per 10usec */
    uint32_t usec[100];

    /* we collect timings from 0-49 ms (entry 0 is never used!) */
    uint32_t msec[50];

    uint32_t halfsec[10];

    uint32_t wayout;
} timings_t;

timings_t timings[0x100];

void collect_timing(uint8_t cmd, hrtime_t nsec)
{
    timings_t *t = &timings[cmd];
    hrtime_t usec = nsec / 1000;
    hrtime_t msec = usec / 1000;
    hrtime_t hsec = msec / 500;

    if (usec == 0) {
        t->ns++;
    } else if (usec < 1000) {
        t->usec[usec / 10]++;
    } else if (msec < 50) {
        t->msec[msec]++;
    } else if (hsec < 10) {
        t->halfsec[hsec]++;
    } else {
        t->wayout++;
    }
}

void initialize_timings(void)
{
    int ii, jj;
    for (ii = 0; ii < 0x100; ++ii) {
        timings[ii].ns = 0;
        for (jj = 0; jj < 100; ++jj) {
            timings[ii].usec[jj] = 0;
        }
        for (jj = 0; jj < 50; ++jj) {
            timings[ii].msec[jj] = 0;
        }
        for (jj = 0; jj < 10; ++jj) {
            timings[ii].halfsec[jj] = 0;
        }
        timings[ii].wayout = 0;
    }
}

void generate_timings(uint8_t opcode, const void *cookie)
{
    std::stringstream ss;
    timings_t *t = &timings[opcode];

    ss << "{\"ns\":" << t->ns << ",\"us\":[";
    for (int ii = 0; ii < 99; ++ii) {
        ss << t->usec[ii] << ",";
    }
    ss << t->usec[99] << "],\"ms\":[";
    for (int ii = 1; ii < 49; ++ii) {
        ss << t->msec[ii] << ",";
    }
    ss << t->msec[49] << "],\"500ms\":[";
    for (int ii = 0; ii < 9; ++ii) {
        ss << t->halfsec[ii] << ",";
    }
    ss << t->halfsec[9] << "],\"wayout\":" << t->wayout << "}";
    std::string str = ss.str();

    binary_response_handler(NULL, 0, NULL, 0, str.data(), str.length(),
                            PROTOCOL_BINARY_RAW_BYTES,
                            PROTOCOL_BINARY_RESPONSE_SUCCESS,
                            0, cookie);
}
