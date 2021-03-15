#pragma once

#include <pthread.h>

struct Locker {

    Locker() { pthread_mutex_init(&mutex_, nullptr); }

    ~Locker() { pthread_mutex_lock(&mutex_); }

    struct Holder {
        Holder(pthread_mutex_t mutex) : mutex_(mutex) {
            pthread_mutex_lock(&mutex_);
        }

        ~Holder() {
            pthread_mutex_unlock(&mutex_);
        }

    private:
        pthread_mutex_t mutex_;

        Holder(const Holder &) = delete;
        void operator=(const Holder &) = delete;
    };
    Holder hold() { return {mutex_}; };

private:
    pthread_mutex_t mutex_;
} ;
