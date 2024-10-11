#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept : buffer_(std::exchange(other.buffer_, nullptr)), 
                                            capacity_(std::exchange(other.capacity_, 0)) {
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            capacity_ = std::exchange(rhs.capacity_, 0);
            buffer_ = std::exchange(rhs.buffer_, nullptr);
        }
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    Vector() = default;

    Vector(size_t size) : data_(size), size_(size) {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other) : data_(other.size_), 
                                    size_(other.size_) {
       std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return cbegin();
    }
    const_iterator end() const noexcept {
        return cend();
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                /* Применить copy-and-swap */
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                if (rhs.size_ < size_) {
                    std::copy(rhs.begin(), rhs.end(), begin());
                    std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
                }else {
                    std::copy(rhs.begin(), rhs.begin() + size_, begin());
                    std::uninitialized_copy_n(rhs.begin() + size_, rhs.size_ - size_, end());
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    ~Vector() {
        std::destroy_n(begin(), size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
        }
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(begin() + new_size, size_ - new_size);
        }else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(end(), new_size - size_);
        }
        size_ = new_size;
    }
    
    /*template<typename Type>
    void PushBack(Type&& value) {
        //EmplaceBack(std::move(value));
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Type>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
            }else {
                std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }else {
            new (data_ + size_) T(std::forward<Type>(value));
        }
        ++size_;
    }*/

    void PopBack() noexcept {
        assert(size_ > 0);
        std::destroy_at(end() - 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        T* ptr_ = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            ptr_ = new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                try {
                    std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
                }catch(...) {
                    std::destroy_at(new_data.GetAddress() + size_);
                    throw;
                }
                
            }else {
                try {
                    std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
                }catch(...) {
                    std::destroy_at(new_data.GetAddress() + size_);
                    throw;
                }
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }else {
            ptr_ = new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *ptr_;
    }

    void PushBack(const T& value) {
        EmplaceBack((value));
    }

    void PushBack(T&& value) {
        EmplaceBack(std::forward<T>(value));
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());
        size_t position = static_cast<size_t>(pos - begin());
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + position) T(std::forward<Args>(args)...);            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                try {
                    std::uninitialized_move_n(begin(),position, new_data.GetAddress());
                }catch(...) {
                    std::destroy_at(new_data.GetAddress() + position);
                    throw;
                }
                try {
                    std::uninitialized_move_n(begin() + position, size_ - position, new_data.GetAddress() + position + 1);
                }catch(...) {
                    std::destroy_n(begin(), position + 1);
                    throw;
                }                
            }else {
                try {
                    std::uninitialized_copy_n(begin(), position, new_data.GetAddress());
                }catch(...) {
                    std::destroy_at(new_data.GetAddress() + position);
                    throw;
                }
                try {
                    std::uninitialized_copy_n(begin() + position, size_ - position, new_data.GetAddress() + position + 1);
                }catch(...) {
                    std::destroy_n(begin(), position + 1);
                    throw;
                }
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }else {
            if (pos != end()) {
                T temp(std::forward<Args>(args)...);
                new (end()) T(std::move(*(end() - 1)));
                std::move_backward(begin() + position, end() - 1, end());
                try {
                    data_[position] = std::move(temp);
                }catch(...) {
                    std::destroy_at(end());
                    throw;
                }                
            }else {
                try {
                    new (end()) T(std::forward<Args>(args)...);
                }catch(...) {
                    std::destroy_at(end());
                    throw;
                }
            }
        }
        ++size_;
        return begin() + position;
    }

    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos < end());
        size_t position = static_cast<size_t>(pos - begin());
        std::move(begin() + (position + 1), end(), begin() + position);
        std::destroy_at(end() - 1);
        --size_;
        return begin() + position;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    static void Deallocate(T* buf) noexcept{
        operator delete(buf);
    }

    static void Destroy(T* buf) {
        buf->~T();
    }

    static void DestroyN(T* buf, size_t size) {
        for (size_t i = 0; i != size; ++i) {
            Destroy(buf + i);
        }
    }

    void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }
};