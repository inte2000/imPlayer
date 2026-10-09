#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

#include <streambuf>
#include <istream>
#include <ostream>
#include <vector>


template<typename T>
class mem_streambuf : public std::basic_streambuf<T, std::char_traits<T>>
{
public:
    using char_type = T;
    using traits_type = std::char_traits<T>;
    using int_type = typename std::char_traits<T>::int_type;
    using pos_type = typename std::char_traits<T>::pos_type;
    using off_type = typename std::char_traits<T>::off_type;
    using base_type = typename std::basic_streambuf<T, std::char_traits<T>>;

    mem_streambuf(const T* begin, const T* end) {
        base_type::setg(const_cast<T*>(begin), const_cast<T*>(begin), const_cast<T*>(end));
    }
    mem_streambuf(const T* data, std::size_t size) {
        base_type::setg(const_cast<T*>(data), const_cast<T*>(data), const_cast<T*>(data) + size);
    }
    void Attach(const T* data, std::size_t size) {
        base_type::setg(const_cast<T*>(data), const_cast<T*>(data), const_cast<T*>(data) + size);
    }

protected:
    // Override underflow to handle buffer replenishment (not strictly needed for fixed-size buffers)
    int_type underflow() override {
        if (base_type::gptr() == base_type::egptr()) {
            return traits_type::eof(); // End of buffer
        }
        return traits_type::to_int_type(*base_type::gptr());
    }

    // 重写 seekoff：支持相对偏移定位
    pos_type seekoff(off_type offset, std::ios_base::seekdir dir,
        std::ios_base::openmode which = std::ios_base::in) override
    {
        if (which != std::ios_base::in) {
            return pos_type(-1); // 仅支持输入流
        }

        char* new_pos = nullptr;
        switch (dir) {
        case std::ios_base::beg:
            new_pos = base_type::eback() + offset;
            break;
        case std::ios_base::cur:
            new_pos = base_type::gptr() + offset;
            break;
        case std::ios_base::end:
            new_pos = base_type::egptr() + offset;
            break;
        default:
            return pos_type(-1); // 无效方向
        }

        // 检查边界
        if (new_pos < base_type::eback() || new_pos > base_type::egptr()) {
            return pos_type(-1); // 越界
        }

        // 更新指针位置
        base_type::setg(base_type::eback(), new_pos, base_type::egptr());
        return new_pos - base_type::eback(); // 返回新位置
    }

    // 重写 seekpos：支持绝对位置定位
    pos_type seekpos(pos_type pos,
        std::ios_base::openmode which = std::ios_base::in) override
    {
        return seekoff(pos, std::ios_base::beg, which); // 转换为相对偏移
    }
};

constexpr std::size_t BLOCK_SIZE = 256 * 1024;

template<typename T>
class memo_streambuf : public std::basic_streambuf<T, std::char_traits<T>> {
public:
    using char_type = T;
    using traits_type = std::char_traits<T>;
    using int_type = typename std::char_traits<T>::int_type;
    using pos_type = typename std::char_traits<T>::pos_type;
    using off_type = typename std::char_traits<T>::off_type;
    using base_type = typename std::basic_streambuf<T, std::char_traits<T>>;

    memo_streambuf() { set_capacity(BLOCK_SIZE); }

    virtual ~memo_streambuf() { if (free_in_destructor_) { std::free(memptr_); } }

    // If set to false, does not free the buffer on destruction, user has to call std::free() itself
    constexpr void free_in_destructor(bool do_free) { free_in_destructor_ = do_free; }

    // Changes the buffer capacity to an exact value
    constexpr void set_capacity(std::streamsize new_size) {
        // minimum size is one byte
        if (new_size <= 0) { throw std::bad_alloc(); }

        // (re-)allocate memory, returns either ptr to memory or nullptr on failure
        void* new_memptr = std::realloc(memptr_, new_size);
        if (new_memptr == nullptr) { throw std::bad_alloc(); }
        memptr_ = static_cast<char_type*>(new_memptr);

        // store currently used size of underlying streambuf
        const std::streamsize current_size = get_size();

        // set pbase, pptr and epptr to new buffer
        base_type::setp(memptr_, memptr_ + new_size);

        // advance pptr to old position or end of buffer is smaller
        base_type::pbump((int)std::min(current_size, new_size));

        // update get area pointers
        const std::streamsize current_getpos = base_type::gptr() - base_type::eback();
        base_type::setg(base_type::pbase(), base_type::pbase() + current_getpos, base_type::pptr());
    }

    // Increase the buffer capacity by additional_size
    constexpr void increase_capacity(std::streamsize additional_size) {
        set_capacity(get_capacity() + additional_size);
    }

    // Increases the buffer capacity by N blocks such that additional_size < N * BLOCK_SIZE
    constexpr void increase_block_capacity(std::streamsize additional_size) {
        const std::streamsize new_blocks = 1 + additional_size / BLOCK_SIZE;
        increase_capacity(new_blocks * BLOCK_SIZE);
    }

    // Shrinks the buffer to the used data
    constexpr void shrink_to_fit() { set_capacity(get_size()); }

    // Returns the current memory pointer of the buffer, can change if data is added
    constexpr void* get_memptr() const { return memptr_; }

    // Returns the current size of the data in the buffer currently in use
    constexpr std::streamsize get_size() const { return base_type::pptr() - base_type::pbase(); }

    // Returns the current capacity of the buffer
    constexpr std::streamsize get_capacity() const { return base_type::epptr() - base_type::pbase();; }

    // Returns the free capacity of the buffer
    constexpr std::streamsize get_free_capacity() const { return base_type::epptr() - base_type::pptr(); }

    // Implements std::basic_streambuf::xsputn
    std::streamsize xsputn(const char_type* s, std::streamsize count) override {
        if (count > get_free_capacity()) {
            // more to write than free capacity, try to increase to at least count for speedup
            try {
                increase_block_capacity(count);
            }
            catch (const std::bad_alloc& ) {
                // fine if throw, streambuf::xsputn will call overflow
            }
        }

        // use streambuf::xsputn, store written chars for get area
        std::streamsize written = base_type::xsputn(s, count);
        // update streambuf::egptr
        base_type::setg(base_type::eback(), base_type::gptr(), base_type::pptr());

        return written;
    }

    // Implements std::basic_streambuf::overflow
    int_type overflow(int_type ch) override {
        // try to increase the size by one block
        try {
            increase_block_capacity(sizeof(char_type));
        }
        catch (const std::bad_alloc& ) {
            // size increase did not work, return eof
            return traits_type::eof();
        }
        // put char if not eof
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            char_type ch_char = traits_type::to_char_type(ch);
            xsputn(&ch_char, 1);
        }
        return traits_type::not_eof(ch);
    }

    // Implements std::basic_streambuf::showmanyc
    std::streamsize showmanyc() override {
        // never characters in associated character since xsputn automatically sets streambuf::egptr
        return -1;
    }

    // Implements std::basic_streambuf::setbuf
    // Note: either the the buffer needs to be freed by the user via free_in_destructor(false)
    //       or the buffer needs to be allocated via std::malloc so that std::free works
    memo_streambuf* setbuf(char_type* s, std::streamsize n) override {
        // free current memory
        std::free(memptr_);
        // update memptr to new character sequence
        memptr_ = s;
        // set pbase, pptr and epptr to new buffer
        base_type::setp(memptr_, memptr_ + n);

        return this;
    }

private:
    bool free_in_destructor_{ true };
    char_type* memptr_{ nullptr };
};

template<typename T>
class mem_istream : public std::basic_istream<T, std::char_traits<T>>
{
public:
    mem_istream(const T* data, std::size_t size)
        : std::basic_istream<T, std::char_traits<T>>(&m_buffer), m_buffer(data, size)
    {
        // 重置流状态
        //clear();
    }
    mem_istream() : std::basic_istream<T, std::char_traits<T>>(&m_buffer)
    {
    }
    
    void Attach(const T* data, std::size_t size) {
        //std::basic_istream<T, std::char_traits<T>>::rdbuf();
        m_buffer.Attach(data, size);
    }

private:
    mem_streambuf<T> m_buffer;
};

template<typename T>
class mem_ostream : public std::basic_ostream<T, std::char_traits<T>>
{
public:
    mem_ostream() : std::basic_ostream<T, std::char_traits<T>>(&m_buffer)
    {
    }

private:
    memo_streambuf<T> m_buffer;
};
#endif //MEMORY_STREAM_H
