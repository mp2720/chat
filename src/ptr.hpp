#pragma once

#include <cstddef>
#include <gsl/pointers>
#include <memory>
#include <utility>

namespace chat {

using gsl::not_null, gsl::owner;
using std::unique_ptr, std::weak_ptr, std::shared_ptr;

template <typename T> class HeapArray {
  private:
    T *ptr = nullptr;

  public:
    HeapArray() noexcept {}

    HeapArray(size_t size) {
        ptr = new T[size];
    }

    HeapArray(HeapArray &&other) {
        *this = std::move(other);
    }

    HeapArray &operator=(HeapArray &&other) {
        if (this != &other) {
            this->~HeapArray();

            this->ptr = other.ptr;

            other.ptr = 0;
        }

        return *this;
    }

    operator T *() noexcept {
        return ptr;
    }

    operator const T *() const noexcept {
        return ptr;
    }

    ~HeapArray() {
        delete[] ptr;
    }
};

} // namespace chat
