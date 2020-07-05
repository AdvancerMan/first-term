#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>

template <typename T>
struct vector {
    using iterator = T*;
    using const_iterator = T const*;

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

    iterator insert(const_iterator pos, T const&); // O(N) weak
    iterator erase(const_iterator pos);     // O(N) weak
    iterator erase(const_iterator first, const_iterator last); // O(N) weak

    vector splice(const_iterator first);       // O(N) strong

private:
    void change_capacity(size_t);                    // O(N) strong
    void change_capacity_and_insert(size_t i, T const&);  // O(N) strong if (i == size) else weak
    void change_data(T* ptr, size_t size);  // O(N) strong
    static void destroy(T*, T*);            // O(N) nothrow
    void destroy_all();                     // O(N) nothrow
    static void copy_and_construct(T* dst, T const* src, size_t size);

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
    if (v.size_ != 0) {
        data_ = static_cast<T*>(operator new(v.size_ * sizeof(T)));
        capacity_ = v.size_;

        copy_and_construct(data_, v.data_, v.size_);
        size_ = v.size_;
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
    destroy_all();
    operator delete(data_);
}

template <typename T>
T& vector<T>::operator[](size_t i) {
    return data_[i];
}

template <typename T>
T const& vector<T>::operator[](size_t i) const  {
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
    return data_[0];
}

template <typename T>
T const& vector<T>::front() const {
    return data_[0];
}

template <typename T>
T& vector<T>::back() {
    return data_[size_ - 1];
}

template <typename T>
T const& vector<T>::back() const {
    return data_[size_ - 1];
}

template <typename T>
void vector<T>::push_back(T const& e) {
    insert(end(), e);
}

template <typename T>
void vector<T>::pop_back() {
    data_[--size_].~T();
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
        change_capacity(size);
    }
}

template <typename T>
void vector<T>::shrink_to_fit() {
    if (capacity_ != size_) {
        change_capacity(size_);
    }
}

template <typename T>
void vector<T>::clear() {
    destroy_all();
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
typename vector<T>::iterator vector<T>::insert(const_iterator it, T const& e) {
    ptrdiff_t pos = it - data_;
    if (size_ == capacity_) {
        change_capacity_and_insert(pos, e);
    } else {
        new(end()) T(e);
        for (size_t i = size_; i > pos; i--) {
            std::swap(data_[i], data_[i - 1]);
        }
        size_++;
    }

    return data_ + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos) {
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last) {
    ptrdiff_t pos = first - data_;
    if (first >= last) {
        return data_ + pos;
    }

    vector<T> tail = splice(last);
    splice(first);
    copy_and_construct(data_ + pos, tail.data_, tail.size_);
    size_ += tail.size_;
    return data_ + pos;
}

template <typename T>
vector<T> vector<T>::splice(const_iterator first) {
    vector<T> res;
    res.reserve(end() - first);

    for (const_iterator it = first; it != end(); it++) {
        res.push_back(*it);
    }

    destroy(const_cast<iterator>(first), end());
    size_ = first - data_;
    return res;
}

template <typename T>
void vector<T>::change_capacity(size_t size) {
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
        destroy_all();
        operator delete(data_);
        change_data(ptr, size);
    }
}

template <typename T>
void vector<T>::change_capacity_and_insert(size_t i, T const& e) {
    size_t cap = capacity_ != 0 ? capacity_ * 2 : 1;
    T* ptr = static_cast<T*>(operator new(cap * sizeof(T)));

    size_t ptr_sz = 0;
    try {
        copy_and_construct(ptr, data_, i);
        ptr_sz += i;

        new(ptr + i) T(e);
        ptr_sz++;

        copy_and_construct(ptr + i + 1, data_ + i, size_ - i);
    } catch(...) {
        destroy(ptr, ptr + ptr_sz);
        operator delete(ptr);
        throw;
    }

    destroy_all();
    operator delete(data_);
    change_data(ptr, cap);
    size_++;
}

template <typename T>
void vector<T>::change_data(T* data, size_t capacity) {
    std::swap(data_, data);
    capacity_ = capacity;
}

template <typename T>
void vector<T>::destroy(T* first, T* last) {
    for (; last != first; last--) {
        (last - 1)->~T();
    }
}

template <typename T>
void vector<T>::destroy_all() {
    destroy(data_, end());
}

template <typename T>
void vector<T>::copy_and_construct(T* dst, T const* src, size_t size) {
    size_t i = 0;

    try {
        for (; i < size; i++) {
            new(dst + i) T(src[i]);
        }
    } catch(...) {
        destroy(dst, dst + i);
        throw;
    }
}

#endif // VECTOR_H
