#ifndef OPTIMIZED_STORAGE_H
#define OPTIMIZED_STORAGE_H

#include <cstddef>
#include <cstring>
#include <vector>

// T is trivially constructible, trivially destructible and trivially copiable
template <typename T>
struct optimized_storage {
    optimized_storage(size_t size);
    optimized_storage(size_t size, T const& value);

    optimized_storage(optimized_storage const&);
    optimized_storage& operator=(optimized_storage const&);

    T const& operator[](size_t) const;
    T& operator[](size_t);

    T const& back() const;
    T& back();

    void push_back(T);
    void pop_back();

    void reserve(size_t);
    void assign(size_t, T const&);

    size_t size() const;
private:
    struct buffer {
        buffer() = delete;
        buffer(buffer const&) = delete;
        buffer& operator=(buffer const&) = delete;

        static buffer* allocate_buffer(size_t cap) {
            buffer* res = reinterpret_cast<buffer*>(operator new(2 * sizeof(size_t) + cap * sizeof(T)));
            res->count = 1;
            res->capacity = cap;
            return res;
        }

        static buffer* allocate_buffer(size_t cap, size_t size, T const& e) {
            buffer* buf = allocate_buffer(cap);
            std::fill(buf->values, buf->values + size, e);
            return buf;
        }

        buffer* copy_and_unshare(size_t new_cap) {
            buffer* res = allocate_buffer(new_cap);
            memcpy(res->values, values, sizeof(T) * std::min(new_cap, capacity));
            unshare();
            return res;
        }

        void unshare() {
            count--;
            if (count == 0) {
                operator delete(this);
            }
        }

        buffer* share() {
            count++;
            return this;
        }

        T& get(size_t i) {
            return values[i];
        }

        T const& get(size_t i) const {
            return values[i];
        }

        size_t count;
        size_t capacity;
        T values[];
    };

    static const size_t SMALL_SIZE = sizeof(buffer*) / sizeof(T);
    size_t size_;

    union {
        buffer* buf;
        T values[SMALL_SIZE];
    };
};

template <typename T>
optimized_storage<T>::optimized_storage(size_t size)
    : optimized_storage(size, 0) {}

template <typename T>
optimized_storage<T>::optimized_storage(size_t size, T const& value)
    : size_(size) {
    if (size_ <= SMALL_SIZE) {
        std::fill(values, values + size_, value);
    } else {
        buf = buffer::allocate_buffer(size_, size_, value);
    }
}

template <typename T>
optimized_storage<T>::optimized_storage(optimized_storage const& other)
    : size_(other.size_) {
    if (size_ <= SMALL_SIZE) {
        memmove(values, other.values, sizeof(T) * size_);
    } else {
        buf = other.buf->share();
    }
}

template <typename T>
optimized_storage<T>& optimized_storage<T>::operator=(optimized_storage const& other) {
    buffer* tmp = nullptr;
    if (other.size_ > SMALL_SIZE) {
        tmp = other.buf->share();
    }
    if (size_ > SMALL_SIZE) {
        buf->unshare();
    }

    if (other.size_ <= SMALL_SIZE) {
        memmove(values, other.values, sizeof(T) * other.size_);
    } else {
        buf = tmp;
    }

    size_ = other.size_;
    return *this;
}

template <typename T>
T const& optimized_storage<T>::operator[](size_t i) const {
    return size_ <= SMALL_SIZE ? values[i] : buf->get(i);
}

template <typename T>
T& optimized_storage<T>::operator[](size_t i) {
    if (size_ <= SMALL_SIZE) {
        return values[i];
    }

    if (buf->count > 1) {
        buf = buf->copy_and_unshare(buf->capacity);
    }
    return buf->get(i);
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
void optimized_storage<T>::push_back(T e) {
    if (size_ == SMALL_SIZE) {
        buffer* tmp = buffer::allocate_buffer(SMALL_SIZE * 2);
        memcpy(tmp->values, values, sizeof(T) * SMALL_SIZE);
        buf = tmp;
    } else if (size_ > SMALL_SIZE) {
        if (size_ == buf->capacity) {
            buf = buf->copy_and_unshare(buf->capacity == 0 ? 1 : 2 * buf->capacity);
        } else if (buf->count > 1) {
            buf = buf->copy_and_unshare(buf->capacity);
        }
    }

    size_++;
    back() = e;
}

template <typename T>
void optimized_storage<T>::pop_back() {
    if (--size_ == SMALL_SIZE) {
        T tmp[size_];
        memcpy(tmp, buf->values, sizeof(T) * size_);
        buf->unshare();
        memcpy(values, tmp, sizeof(T) * size_);
    }
}

// TODO doesn't work for SO
template <typename T>
void optimized_storage<T>::reserve(size_t size) {
    if (size_ > SMALL_SIZE && size > buf->capacity) {
        buf = buf->copy_and_unshare(size);
    }
}

template <typename T>
void optimized_storage<T>::assign(size_t size, T const& value) {
    if (size_ > SMALL_SIZE) {
        buf->unshare();
    }

    if (size <= SMALL_SIZE) {
        std::fill(values, values + size, value);
    } else {
        buf = buffer::allocate_buffer(size, size, value);
    }
    size_ = size;
}

template <typename T>
size_t optimized_storage<T>::size() const {
    return size_;
}

#endif // OPTIMIZED_STORAGE_H
