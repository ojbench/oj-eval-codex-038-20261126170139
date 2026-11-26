#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"
#include <iostream>
#include <cstring>

namespace sjtu {
template<bool B, class Yes, class No> struct vector_select { using type = Yes; };
template<class Yes, class No> struct vector_select<false, Yes, No> { using type = No; };
template<bool B> struct vector_enable {};
template<> struct vector_enable<true> { using type = int; };
template<class T> class vector {
    T *data_ = nullptr;
    size_t size_ = 0, capacity_ = 0;

    static T *allocate(size_t n) {
        if (n > size_t(-1) / sizeof(T)) throw runtime_error();
        if constexpr (__is_trivially_copyable(T) && alignof(T) <= alignof(std::max_align_t)) {
            void *p = std::malloc(n * sizeof(T));
            if (!p) throw runtime_error();
            return static_cast<T *>(p);
        }
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            return static_cast<T *>(::operator new(n * sizeof(T), std::align_val_t(alignof(T))));
        else return static_cast<T *>(::operator new(n * sizeof(T)));
    }
    static void release(T *p) {
        if constexpr (__is_trivially_copyable(T) && alignof(T) <= alignof(std::max_align_t)) std::free(p);
        else if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            ::operator delete(p, std::align_val_t(alignof(T)));
        else ::operator delete(p);
    }
    static void destroy(T *p, size_t n) {
        if constexpr (!__has_trivial_destructor(T)) {
            for (size_t i = 0; i < n; ++i) p[i].~T();
        }
    }
    void grow() {
        const size_t next = capacity_ ? capacity_ * 2 : 4;
        if (next < capacity_ || next > size_t(-1) / sizeof(T)) throw runtime_error();
        // Trivial objects can be relocated by the allocator, often in place.
        if constexpr (__is_trivially_copyable(T) && alignof(T) <= alignof(std::max_align_t)) {
            void *p = std::realloc(data_, next * sizeof(T));
            if (!p) throw runtime_error();
            data_ = static_cast<T *>(p);
            capacity_ = next;
            return;
        }
        T *p = allocate(next);
        if constexpr (__is_trivially_copyable(T)) {
            if (size_) std::memcpy(p, data_, size_ * sizeof(T));
        } else {
            size_t i = 0;
            try {
                for (; i < size_; ++i) new (p + i) T(static_cast<T &&>(data_[i]));
            } catch (...) {
                destroy(p, i);
                release(p);
                throw;
            }
            destroy(data_, size_);
        }
        release(data_);
        data_ = p;
        capacity_ = next;
    }

public:
    template<bool Constant> class basic_iterator {
        friend class vector;
        template<bool> friend class basic_iterator;
        const vector *owner_ = nullptr;
        std::ptrdiff_t index_ = 0;
        basic_iterator(const vector *v, std::ptrdiff_t i) : owner_(v), index_(i) {}
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using iterator_category = std::random_access_iterator_tag;
        using pointer = typename vector_select<Constant, const T *, T *>::type;
        using reference = typename vector_select<Constant, const T &, T &>::type;
        basic_iterator() = default;
        template<bool C, typename vector_enable<Constant || !C>::type = 0>
        basic_iterator(const basic_iterator<C> &o) : owner_(o.owner_), index_(o.index_) {}
        reference operator*() const {
            if (!owner_ || index_ < 0 || size_t(index_) >= owner_->size_) throw invalid_iterator();
            return owner_->data_[index_];
        }
        pointer operator->() const { return &operator*(); }
        reference operator[](difference_type n) const { return *(*this + n); }
        basic_iterator &operator+=(difference_type n) { index_ += n; return *this; }
        basic_iterator &operator-=(difference_type n) { index_ -= n; return *this; }
        basic_iterator operator+(difference_type n) const { auto t = *this; return t += n; }
        basic_iterator operator-(difference_type n) const { auto t = *this; return t -= n; }
        friend basic_iterator operator+(difference_type n, basic_iterator t) { return t += n; }
        basic_iterator &operator++() { ++index_; return *this; }
        basic_iterator &operator--() { --index_; return *this; }
        basic_iterator operator++(int) { auto t = *this; ++*this; return t; }
        basic_iterator operator--(int) { auto t = *this; --*this; return t; }
        template<bool C> difference_type operator-(const basic_iterator<C> &o) const {
            if (owner_ != o.owner_) throw invalid_iterator();
            return index_ - o.index_;
        }
        template<bool C> bool operator==(const basic_iterator<C> &o) const { return owner_ == o.owner_ && index_ == o.index_; }
        template<bool C> bool operator!=(const basic_iterator<C> &o) const { return !(*this == o); }
        template<bool C> bool operator<(const basic_iterator<C> &o) const { return (*this - o) < 0; }
        template<bool C> bool operator>(const basic_iterator<C> &o) const { return o < *this; }
        template<bool C> bool operator<=(const basic_iterator<C> &o) const { return !(o < *this); }
        template<bool C> bool operator>=(const basic_iterator<C> &o) const { return !(*this < o); }
    };
    using iterator = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;

    vector() = default;
    vector(const vector &o) {
        if (!o.size_) return;
        data_ = allocate(o.size_);
        capacity_ = o.size_;
        if constexpr (__is_trivially_copyable(T)) {
            std::memcpy(data_, o.data_, o.size_ * sizeof(T));
            size_ = o.size_;
            return;
        }
        try {
            for (; size_ < o.size_; ++size_) new (data_ + size_) T(o.data_[size_]);
        } catch (...) {
            destroy(data_, size_);
            release(data_);
            throw;
        }
    }
    vector(vector &&o) noexcept : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
        o.data_ = nullptr; o.size_ = o.capacity_ = 0;
    }
    ~vector() { destroy(data_, size_); release(data_); }
    vector &operator=(const vector &o) {
        if (this != &o) { vector copy(o); swap(copy); }
        return *this;
    }
    vector &operator=(vector &&o) noexcept {
        if (this != &o) { vector moved(static_cast<vector &&>(o)); swap(moved); }
        return *this;
    }
    void swap(vector &o) noexcept {
        T *p = data_; data_ = o.data_; o.data_ = p;
        size_t n = size_; size_ = o.size_; o.size_ = n;
        n = capacity_; capacity_ = o.capacity_; o.capacity_ = n;
    }
    T &at(const size_t &i) { if (i >= size_) throw index_out_of_bound(); return data_[i]; }
    const T &at(const size_t &i) const { if (i >= size_) throw index_out_of_bound(); return data_[i]; }
    T &operator[](const size_t &i) { return at(i); }
    const T &operator[](const size_t &i) const { return at(i); }
    const T &front() const { if (!size_) throw container_is_empty(); return data_[0]; }
    const T &back() const { if (!size_) throw container_is_empty(); return data_[size_ - 1]; }
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size_); }
    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, size_); }
    bool empty() const { return !size_; }
    size_t size() const { return size_; }
    void clear() { destroy(data_, size_); size_ = 0; }

    template<class U> void append(U &&value) {
        if (size_ == capacity_) {
            // Preserve values referring into our own buffer across reallocation.
            T saved(static_cast<U &&>(value));
            grow();
            new (data_ + size_) T(static_cast<T &&>(saved));
        } else new (data_ + size_) T(static_cast<U &&>(value));
        ++size_;
    }
    void push_back(const T &v) { append(v); }
    void push_back(T &&v) { append(static_cast<T &&>(v)); }
    void pop_back() { if (!size_) throw container_is_empty(); data_[--size_].~T(); }
    iterator insert(const size_t &i, const T &value) {
        if (i > size_) throw index_out_of_bound();
        if (i == size_) { push_back(value); return iterator(this, i); }
        T saved(value);
        if (size_ == capacity_) grow();
        if constexpr (__is_trivially_copyable(T)) {
            std::memmove(data_ + i + 1, data_ + i, (size_ - i) * sizeof(T));
            data_[i] = saved;
        } else {
            new (data_ + size_) T(static_cast<T &&>(data_[size_ - 1]));
            ++size_;
            for (size_t j = size_ - 2; j > i; --j) data_[j] = static_cast<T &&>(data_[j - 1]);
            data_[i] = static_cast<T &&>(saved);
            return iterator(this, i);
        }
        ++size_;
        return iterator(this, i);
    }
    iterator insert(iterator p, const T &v) {
        if (p.owner_ != this || p.index_ < 0 || size_t(p.index_) > size_) throw invalid_iterator();
        return insert(size_t(p.index_), v);
    }
    iterator erase(const size_t &i) {
        if (i >= size_) throw index_out_of_bound();
        if constexpr (__is_trivially_copyable(T)) {
            std::memmove(data_ + i, data_ + i + 1, (size_ - i - 1) * sizeof(T));
        } else {
            for (size_t j = i; j + 1 < size_; ++j) data_[j] = static_cast<T &&>(data_[j + 1]);
        }
        data_[--size_].~T();
        return iterator(this, i);
    }
    iterator erase(iterator p) {
        if (p.owner_ != this || p.index_ < 0 || size_t(p.index_) >= size_) throw invalid_iterator();
        return erase(size_t(p.index_));
    }
};
}
#endif
