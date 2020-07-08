#ifndef COW_BUFFER_H
#define COW_BUFFER_H

#include <cstddef>
#include <algorithm>
#include <type_traits>

template <typename T>
struct buffer {
    static_assert(std::is_trivially_constructible<T>::value, "T should be trivially constructible");
    static_assert(std::is_trivially_destructible<T>::value, "T should be trivially destructible");
    static_assert(std::is_trivially_copyable<T>::value, "T should be trivially copyable");

    buffer() = delete;
    buffer(buffer const&) = delete;
    buffer& operator=(buffer const&) = delete;

    static buffer* allocate_buffer(size_t cap);
    static buffer* allocate_buffer(size_t cap, T const& e);

    buffer* copy_and_unshare(size_t new_cap, size_t size);

    void unshare();
    buffer* share();

    bool not_unique() const;

    size_t count;
    size_t capacity;
    T values[];
};

template <typename T>
buffer<T>* buffer<T>::allocate_buffer(size_t cap) {
    buffer* res = reinterpret_cast<buffer*>(operator new(sizeof(buffer<T>) + cap * sizeof(T)));
    res->count = 1;
    res->capacity = cap;
    return res;
}

template <typename T>
buffer<T>* buffer<T>::allocate_buffer(size_t cap, T const& e) {
    buffer* buf = allocate_buffer(cap);
    std::fill(buf->values, buf->values + cap, e);
    return buf;
}

template <typename T>
buffer<T>* buffer<T>::copy_and_unshare(size_t new_cap, size_t size) {
    buffer* res = allocate_buffer(new_cap);
    std::copy(values, values + size, res->values);
    unshare();
    return res;
}

template <typename T>
void buffer<T>::unshare() {
    count--;
    if (count == 0) {
        operator delete(this);
    }
}

template <typename T>
buffer<T>* buffer<T>::share() {
    count++;
    return this;
}

template <typename T>
bool buffer<T>::not_unique() const {
    return count > 1;
}

#endif // COW_BUFFER_H
