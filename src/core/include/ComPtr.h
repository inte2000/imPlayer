#ifndef __COM_PTR_H__
#define __COM_PTR_H__

#include <unknwn.h> 


template <typename T>
class ComPtr final
{
public:
    ComPtr() noexcept : ptr_(nullptr) {}
    // 从原始指针构造（不增加引用计数）
    explicit ComPtr(T* ptr) noexcept : ptr_(ptr) {}
    // 拷贝构造函数（增加引用计数）
    ComPtr(const ComPtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_ != nullptr) {
            ptr_->AddRef();
        }
    }
    
    // 移动构造函数（不增加引用计数）
    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    // 析构函数（减少引用计数）
    ~ComPtr() {
        Release();
    }
    
    // 拷贝赋值运算符
    ComPtr& operator=(const ComPtr& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            if (ptr_ != nullptr) {
                ptr_->AddRef();
            }
        }
        return *this;
    }
    
    // 移动赋值运算符
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    // 从原始指针赋值
    ComPtr& operator=(T* ptr) noexcept {
        if (ptr_ != ptr) {
            Release();
            ptr_ = ptr;
        }
        return *this;
    }
    
    // 释放当前持有的指针
    void Release() noexcept {
        if (ptr_ != nullptr) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }
    
    // 获取原始指针（不转移所有权）
    T* Get() const noexcept { return ptr_; }
    
    // 指针操作符重载
    T* operator->() const noexcept { return ptr_; }
    
    // 获取指针的地址（用于接收输出参数）
    //T** GetAddressOf() noexcept {
    //    Release();
    //    return &ptr_;
    //}
    // 获取地址，供 CreateInstance 等使用
    T** operator&() noexcept {
        Release();
        return &ptr_;
    } 
    
    // 转换到其他接口类型
    template <typename U>
    HRESULT As(ComPtr<U>* other) const noexcept {
        if (other == nullptr || ptr_ == nullptr) {
            return E_INVALIDARG;
        }
        
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(other->GetAddressOf()));
    }
    
    // 与 nullptr 比较
    bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
    
    // 显式 bool 转换
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    
    // 获取并释放所有权（不减少引用计数）
    T* Detach() noexcept {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
    
    // 附加指针（不增加引用计数）
    void Attach(T* ptr) noexcept {
        Release();
        ptr_ = ptr;
    }
    
private:
    T* ptr_;
};

#endif // __COM_PTR_H__