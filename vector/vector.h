#pragma once

#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <algorithm>

template <typename T>
void destroy_all(T* ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ptr[i].~T();
    }
}

template <typename T>
void copy_and_construct(T* dst, T const* src, size_t size, size_t skip) {
    size_t i = 0;
    bool skipped = false;

    try {
        for (; i < size; i++) {
            skipped = i >= skip;
            new(dst + i + skipped) T(src[i]);
        }
    } catch(...) {
        if (skipped) {
            destroy_all(dst, skip);
            destroy_all(dst + skip + 1, i - skip - 1);
        } else {
            destroy_all(dst, i);
        }

        throw;
    }
}

template <typename T>
void copy_and_construct(T* dst, T const* src, size_t size) {
    copy_and_construct(dst, src, size, size);
}

template <typename T>
struct vector {
    typedef T* iterator;
    typedef T const* const_iterator;

    vector();                               // O(1) nothrow
    vector(vector const&);                  // O(N) strong
    vector& operator=(vector const& other); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t i);                // O(1) nothrow
    T const& operator[](size_t i) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    T const* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    T const& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    T const& back() const;                  // O(1) nothrow
    void push_back(T const&);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector&);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(iterator it, T const&); // O(N) weak
    iterator insert(const_iterator pos, T const&); // O(N) weak

    iterator erase(iterator pos);           // O(N) weak
    iterator erase(const_iterator pos);     // O(N) weak

    iterator erase(iterator first, iterator last); // O(N) weak
    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
    void resize(size_t);                    // O(N) strong
    void resize_and_insert(size_t i, T const&);  // O(N) strong if (i == size) else weak
    void change_data(T* ptr, size_t size);  // O(N) strong

    T* data_;
    size_t size_;
    size_t capacity_;
};

template <typename T>
vector<T>::vector()
        : data_(nullptr)
        , size_(0)
        , capacity_(0) {}

template <typename T>
vector<T>::vector(vector<T> const& v)
        : vector() {
    if (v.size() != 0) {
        data_ = static_cast<T*>(operator new(v.size() * sizeof(T)));
        size_ = 0;

        copy_and_construct(data_, v.data(), v.size());

        capacity_ = v.size();
        size_ = v.size();
    }
}

template <typename T>
vector<T>& vector<T>::operator=(vector<T> const& other) {
    vector<T> copy(other);
    swap(copy);
    return *this;
}

template <typename T>
vector<T>::~vector() {
    destroy_all(data_, size_);
    operator delete(data_);
}

template <typename T>
T& vector<T>::operator[](size_t i) {
    assert(i < size_);
    return data_[i];
}

template <typename T>
T const& vector<T>::operator[](size_t i) const  {
    assert(i < size_);
    return data_[i];
}

template <typename T>
T* vector<T>::data() {
    return data_;
}

template <typename T>
T const* vector<T>::data() const {
    return data_;
}

template <typename T>
size_t vector<T>::size() const  {
    return size_;
}

template <typename T>
T& vector<T>::front() {
    assert(size_ > 0);
    return data_[0];
}

template <typename T>
T const& vector<T>::front() const {
    assert(size_ > 0);
    return data_[0];
}

template <typename T>
T& vector<T>::back() {
    assert(size_ > 0);
    return data_[size_ - 1];
}

template <typename T>
T const& vector<T>::back() const {
    assert(size_ > 0);
    return data_[size_ - 1];
}

template <typename T>
void vector<T>::push_back(T const& e) {
    insert(end(), e);
}

template <typename T>
void vector<T>::pop_back() {
    assert(size_ > 0);
    back().~T();
    size_--;
}

template <typename T>
bool vector<T>::empty() const {
    return size_ == 0;
}

template <typename T>
size_t vector<T>::capacity() const {
    return capacity_;
}

template <typename T>
void vector<T>::reserve(size_t size) {
    if (size > capacity_) {
        resize(size);
    }
}

template <typename T>
void vector<T>::shrink_to_fit() {
    if (capacity_ != size_) {
        resize(size_);
    }
}

template <typename T>
void vector<T>::clear() {
    destroy_all(data_, size_);
    size_ = 0;
}

template <typename T>
void vector<T>::swap(vector<T> &other) {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

template <typename T>
typename vector<T>::iterator vector<T>::begin() {
    return data_;
}

template <typename T>
typename vector<T>::iterator vector<T>::end() {
    return data_ + size_;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    return data_;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    return data_ + size_;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(iterator it, T const& e) {
    const size_t pos = it - begin();
    if (size_ == capacity_) {
        resize_and_insert(pos, e);
    } else {
        size_t i = size_;

        try {
            for (; i > pos; i--) {
                new(data_ + i) T(data_[i - 1]);
                data_[i - 1].~T();
            }
            new(data_ + i) T(e);
        } catch(...) {
            destroy_all(data_ + i + 1, size_ - i);
            size_ = i;
            throw;
        }
        size_++;
    }

    return begin() + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(const_iterator pos, T const& e) {
    return insert(const_cast<iterator>(pos), e);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator pos) {
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos) {
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator first, iterator last) {
    return erase(static_cast<const_iterator>(first), static_cast<const_iterator>(last));
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last) {
    size_t end_i = last - begin();
    size_t begin_i = first - begin();
    size_t count = end_i - begin_i;

    if (count == 0) {
        return const_cast<iterator>(first);
    }

    for (size_t i = begin_i; i + count < size_; i++) {
        data_[i].~T();

        try {
            new(data_ + i) T(data_[i + count]);
        } catch(...) {
            destroy_all(data_ + i + 1, size_ - i - 1);
            size_ = i;
            throw;
        }
    }

    for (size_t i = size_ - count; i < size_; i++) {
        data_[i].~T();
    }

    size_ -= count;
    return data_+ begin_i;
}

template <typename T>
void vector<T>::resize(size_t size) {
    if (size == 0) {
        operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
    } else {
        T* ptr = static_cast<T*>(operator new(size * sizeof(T)));
        try {
            copy_and_construct(ptr, data_, size_);
        } catch (...) {
            operator delete(ptr);
            throw;
        }
        change_data(ptr, size);
    }
}

template <typename T>
void vector<T>::resize_and_insert(size_t i, T const& e) {
    size_t cap = capacity_ ? capacity_ * 2 : 1;
    T* ptr = static_cast<T*>(operator new(cap * sizeof(T)));

    try {
        new(ptr + i) T(e);
        copy_and_construct(ptr, data_, size_, i);
    } catch(...) {
        ptr[i].~T();
        operator delete(ptr);
        throw;
    }

    change_data(ptr, cap);
    size_++;
}

template <typename T>
void vector<T>::change_data(T* data, size_t capacity) {
    std::swap(data_, data);
    capacity_ = capacity;
    if (data) {
        destroy_all(data, size_);
        operator delete(data);
    }
}
