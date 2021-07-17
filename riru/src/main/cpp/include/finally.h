//
// Created by loves on 7/14/2021.
//

#ifndef RIRU_FINALLY_H
#define RIRU_FINALLY_H

#include <functional>

struct finally {
    std::function<void()> body;

    finally(const std::function<void()> &body) : body(body) {}

    finally(std::function<void()> &&body) : body(std::move(body)) {};

    ~finally() {
        body();
    }
};

#endif //RIRU_FINALLY_H
