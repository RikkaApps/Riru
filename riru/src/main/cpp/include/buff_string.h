//
// Created by loves on 7/14/2021.
//

#ifndef RIRU_BUFF_STRING_H
#define RIRU_BUFF_STRING_H

#include <array>
#include <string_view>

template<size_t CAP, typename CHAR=char>
class BuffString {
    std::array<CHAR, CAP> data_{'\0'};
    size_t size_{0u};
public:
    BuffString &operator+=(std::string_view str) {
        memcpy(data_.data() + size_, str.data(), str.size());
        size_ += str.size();
        data_[size_] = '\0';
        return *this;
    }

    void size(const size_t &size) {
        size_ = size;
        data_[size] = '\0';
    }

    constexpr auto data() const {
        return data_.data();
    }

    constexpr auto size() const {
        return size_;
    }

    operator std::string_view() const {
        return {data_.data(), size_};
    }

    operator const CHAR *() const {
        return data_.data();
    }
};

#endif //RIRU_BUFF_STRING_H
