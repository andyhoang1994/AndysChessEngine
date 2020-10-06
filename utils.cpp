#pragma once

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <random>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "utils.h"
#include "defines.h"
#include "thread.h"

using namespace std;

namespace {
    struct Tie : public streambuf {
        Tie(streambuf* b, streambuf* l) : buf(b), logBuf(l) {}

        int sync() { return logBuf->pubsync(), buf->pubsync(); }
        int overflow(int c) { return log(buf->sputc((char)c), "<< "); }
        int underflow() { return buf->sgetc(); }
        int uflow() { return log(buf->sbumpc(), ">> "); }

        streambuf* buf, * logBuf;

        int log(int c, const char* prefix) {

            static int last = '\n';

            if(last == '\n')
                logBuf->sputn(prefix, 3);

            return last = logBuf->sputc((char)c);
        }
    };

    class Logger {
        Logger() : in(cin.rdbuf(), file.rdbuf()), out(cout.rdbuf(), file.rdbuf()) {}
        ~Logger() { start(false); }

        ofstream file;
        Tie in, out;

    public:
        static void start(bool b) {
            static Logger l;

            if(b && !l.file.is_open()) {
                l.file.open("io_log.txt", ifstream::out);
                cin.rdbuf(&l.in);
                cout.rdbuf(&l.out);
            }
            else if(!b && l.file.is_open()) {
                cout.rdbuf(l.out.buf);
                cin.rdbuf(l.in.buf);
                l.file.close();
            }
        }
    };
}

static int64_t hits[2], means[2];

void dbg_hit_on(bool b) { ++hits[0]; if(b) ++hits[1]; }
void dbg_hit_on(bool c, bool b) { if(c) dbg_hit_on(b); }
void dbg_mean_of(int v) { ++means[0]; means[1] += v; }

void dbg_print() {
    if(hits[0])
        cerr << "Total " << hits[0] << " Hits " << hits[1]
        << " hit rate (%) " << 100 * hits[1] / hits[0] << endl;

    if(means[0])
        cerr << "Total " << means[0] << " Mean "
        << (double)means[1] / means[0] << endl;
}

std::ostream& operator<<(std::ostream& os, SyncCout sc) {
    static Mutex m;

    if(sc == IO_LOCK) {
        m.lock();
    }

    if(sc == IO_UNLOCK) {
        m.unlock();
    }

    return os;
}

void start_logger(bool b) { Logger::start(b); }

void prefetch(void* addr) {
    _mm_prefetch((char*)addr, _MM_HINT_T0);
}