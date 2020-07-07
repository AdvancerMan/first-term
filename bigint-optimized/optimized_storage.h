#ifndef OPTIMIZED_STORAGE_H
#define OPTIMIZED_STORAGE_H

#include <cstddef>
#include <cstring>
#include <vector>
#include "cow_buffer.h"

template <typename T>
struct optimized_storage {
    static_assert(std::is_trivially_constructible<T>::value, "T should be trivially constructible");
    static_assert(std::is_trivially_destructible<T>::value, "T should be trivially destructible");
    static_assert(std::is_trivially_copyable<T>::value, "T should be trivially copyable");

    optimized_storage(size_t size, T const& value);
    ~optimized_storage();

    optimized_storage(optimized_storage const&);
    optimized_storage& operator=(optimized_storage const&);


    T const& operator[](size_t) const;
    T& operator[](size_t);

    T const& back() const;
    T& back();

    void push_back(T const&);
    void pop_back();

    void assign(size_t, T);
    void resize(size_t, T const&);

    size_t size() const;

    void swap(optimized_storage &);

private:
    void become_big(buffer<T>* new_buffer);
    void become_big(size_t cap, T const& value);
    void become_big(size_t cap);

    static constexpr size_t SMALL_SIZE = sizeof(buffer<T>*) / sizeof(T);

    size_t size_;
    bool is_small_object;

    union {
        buffer<T>* buf;
        T values[SMALL_SIZE];
    } shared;
};

template <typename T>
optimized_storage<T>::optimized_storage(size_t size, T const& value)
    : size_(size)
    , is_small_object(size <= SMALL_SIZE) {
    if (is_small_object) {
        std::fill(shared.values, shared.values + size_, value);
    } else {
        shared.buf = buffer<T>::allocate_buffer(size_, value);
    }
}

template <typename T>
optimized_storage<T>::optimized_storage(optimized_storage const& other)
    : size_(other.size_)
    , is_small_object(other.size_ <= SMALL_SIZE) {
    if (is_small_object) {
        if (other.is_small_object) {
            std::copy(other.shared.values, other.shared.values + size_, shared.values);
        } else {
            std::copy(other.shared.buf->values, other.shared.buf->values + size_, shared.values);
        }
    } else {
        shared.buf = other.shared.buf->share();
    }
}

template <typename T>
optimized_storage<T>::~optimized_storage() {
    if (!is_small_object) {
        shared.buf->unshare();
    }
}

template <typename T>
optimized_storage<T>& optimized_storage<T>::operator=(optimized_storage const& other) {
    optimized_storage copy(other);
    swap(copy);
    return *this;
}

template <typename T>
T const& optimized_storage<T>::operator[](size_t i) const {
    return is_small_object ? shared.values[i] : shared.buf->values[i];
}

template <typename T>
T& optimized_storage<T>::operator[](size_t i) {
    if (is_small_object) {
        return shared.values[i];
    }

    if (shared.buf->count > 1) {
        shared.buf = shared.buf->copy_and_unshare(shared.buf->capacity, size_);
    }
    return shared.buf->values[i];
}

template <typename T>
T const& optimized_storage<T>::back() const {
    return (*this)[size_ - 1];
}

template <typename T>
T& optimized_storage<T>::back() {
    return (*this)[size_ - 1];
}

template <typename T>
void optimized_storage<T>::push_back(T const& e) {
    if ((is_small_object && size_ == SMALL_SIZE) ||
            (!is_small_object && (size_ == shared.buf->capacity || shared.buf->count > 1))) {
        T copy(e);

        if (is_small_object) {
            // size_ == SMALL_SIZE from first if
            become_big(SMALL_SIZE * 2);
        } else if (size_ == shared.buf->capacity) {
            shared.buf = shared.buf->copy_and_unshare(shared.buf->capacity == 0 ? 1 : 2 * shared.buf->capacity, size_);
        } else {
            // shared.buf->count > 1
            shared.buf = shared.buf->copy_and_unshare(shared.buf->capacity, size_);
        }
        new(shared.buf->values + size_) T(copy);
    } else {
        new((is_small_object ? shared.values : shared.buf->values) + size_) T(e);
    }

    size_++;
}

template <typename T>
void optimized_storage<T>::pop_back() {
    size_--;
}

template <typename T>
void optimized_storage<T>::assign(size_t size, T value) {
    resize(size, value);
    if (is_small_object) {
        std::fill(shared.values, shared.values + size, value);
    } else {
        if (shared.buf->count > 1) {
            shared.buf = shared.buf->copy_and_unshare(size, size_);
        }
        std::fill(shared.buf->values, shared.buf->values + size, value);
    }
}

template <typename T>
void optimized_storage<T>::resize(size_t size, T const& value) {
    if (is_small_object) {
        if (size <= SMALL_SIZE) {
            if (size_ < size) {
                std::fill(shared.values + size_, shared.values + size, value);
            }
        } else {
            become_big(size, value);
        }
    } else {
        // if buf should increase capacity OR we have to copy buf to fill size_..size with value
        if (size > shared.buf->capacity || (size > size_ && shared.buf->count > 1)) {
            // size > shared.buf->capacity ==> size > size_
            T copy(value);
            shared.buf = shared.buf->copy_and_unshare(std::max(size, shared.buf->capacity), size_);
            std::fill(shared.buf->values + size_, shared.buf->values + size, copy);
        } else if (size > size_) {
            std::fill(shared.buf->values + size_, shared.buf->values + size, value);
        }
    }

    size_ = size;
}

template <typename T>
size_t optimized_storage<T>::size() const {
    return size_;
}

template <typename T>
void optimized_storage<T>::swap(optimized_storage &other) {
    std::swap(size_, other.size_);
    std::swap(is_small_object, other.is_small_object);
    std::swap(shared, other.shared);
}

template <typename T>
void optimized_storage<T>::become_big(buffer<T> *new_buffer) {
    std::copy(shared.values, shared.values + size_, new_buffer->values);
    shared.buf = new_buffer;
    is_small_object = false;
}

template <typename T>
void optimized_storage<T>::become_big(size_t cap, T const& value) {
    become_big(buffer<T>::allocate_buffer(cap, value));
}

template <typename T>
void optimized_storage<T>::become_big(size_t cap) {
    become_big(buffer<T>::allocate_buffer(cap));
}

#endif // OPTIMIZED_STORAGE_H
