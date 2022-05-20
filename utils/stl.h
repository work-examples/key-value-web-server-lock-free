#pragma once

// Temporary implementation of `std::atomic<std::shared_ptr>`
// GCC supports this this template specialization since version 12 (version 11.3 does not support)
// CLang supports this this template specialization only in trunk on 2022-05-20 (version 14.0 does not support)
// https://stackoverflow.com/questions/67436535/stdatomicstdshared-ptrt-is-not-working-in-c20
// https://stackoverflow.com/a/67504000/739731

// https://en.cppreference.com/w/cpp/feature_test
#if !defined(__cpp_lib_atomic_shared_ptr) || (__cpp_lib_atomic_shared_ptr == 0)

#include <atomic>
#include <memory>


namespace std
{

    template<typename T>
    struct atomic<shared_ptr<T>>
    {

        static constexpr bool is_always_lock_free = false;
        bool is_lock_free() const noexcept {
            return std::atomic_is_lock_free(this);
        }

        constexpr atomic() noexcept {};
        atomic(shared_ptr<T> desired) noexcept : ptr_(std::move(desired))
        {
        }

        atomic(const atomic&) = delete;
        void operator=(const atomic&) = delete;

        void store(shared_ptr<T> desired, memory_order order = memory_order::seq_cst) noexcept
        {
            std::atomic_store_explicit(&ptr_, std::move(desired), order);
        }

        void operator=(shared_ptr<T> desired) noexcept
        {
            store(std::move(desired));
        }

        shared_ptr<T> load(memory_order order = memory_order::seq_cst) const noexcept
        {
            return std::atomic_load_explicit(&ptr_, order);
        }

        operator shared_ptr<T>() const noexcept
        {
            return load();
        }

    private:
        std::shared_ptr<T> ptr_;
    };

}

#endif
