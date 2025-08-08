#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

// Platform-specific includes
#ifdef _WIN32
#include <processthreadsapi.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#endif

namespace Common {
// Get number of CPU cores
inline int getNumCores() noexcept {
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return static_cast<int>(sysinfo.dwNumberOfProcessors);
#elif defined(__APPLE__)
  int cores;
  size_t size = sizeof(cores);
  if (sysctlbyname("hw.ncpu", &cores, &size, nullptr, 0) == 0) {
    return cores;
  }
  return std::thread::hardware_concurrency();
#else
  return std::thread::hardware_concurrency();
#endif
}

// Set thread affinity to specific core
inline bool setThreadCore(int core_id) noexcept {
  if (core_id < 0 || core_id >= getNumCores()) {
    std::cerr << "Invalid core_id: " << core_id << " (available cores: 0-"
              << (getNumCores() - 1) << ")" << std::endl;
    return false;
  }

#ifdef _WIN32
  // Windows implementation
  HANDLE thread = GetCurrentThread();
  DWORD_PTR mask = 1ULL << core_id;
  DWORD_PTR result = SetThreadAffinityMask(thread, mask);
  return result != 0;

#elif defined(__APPLE__)
  // macOS implementation
  thread_affinity_policy_data_t policy;
  policy.affinity_tag = core_id;

  kern_return_t result =
      thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY,
                        (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);

  return result == KERN_SUCCESS;

#elif defined(__linux__)
  // Linux implementation
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
          0);

#else
  // Unsupported platform
  std::cerr << "Thread affinity not supported on this platform" << std::endl;
  return false;
#endif
}

inline bool setThreadName(const std::string &name) noexcept {
#ifdef _WIN32
  // Windows 10 version 1607 and later
  try {
    std::wstring wname(name.begin(), name.end());
    HRESULT hr = SetThreadDescription(GetCurrentThread(), wname.c_str());
    return SUCCEEDED(hr);
  } catch (...) {
    return false;
  }

#elif defined(__APPLE__)
  // macOS
  return pthread_setname_np(name.c_str()) == 0;

#elif defined(__linux__)
  // Linux (name limited to 16 characters including null terminator)
  std::string truncated_name = name.substr(0, 15);
  return pthread_setname_np(pthread_self(), truncated_name.c_str()) == 0;

#else
  return false;
#endif
}

// Get current thread ID in a cross-platform way
inline uint64_t getCurrentThreadId() noexcept {
#ifdef _WIN32
  return static_cast<uint64_t>(GetCurrentThreadId());
#elif defined(__APPLE__)
  uint64_t tid;
  pthread_threadid_np(nullptr, &tid);
  return tid;
#elif defined(__linux__)
  return static_cast<uint64_t>(pthread_self());
#else
  return 0;
#endif
}

/// Creates a thread instance, sets affinity on it, assigns it a name and
/// passes the function to be run on that thread as well as the arguments to the
/// function.
template <typename T, typename... A>
inline std::thread *createAndStartThread(int core_id, const std::string &name,
                                         T &&func, A &&...args) noexcept {

  auto *t = new std::thread([&]() mutable {
    // Set thread name first
    if (!setThreadName(name)) {
      std::cerr << "Warning: Failed to set thread name to '" << name << "'"
                << std::endl;
    }

    // Set thread affinity if requested
    if (core_id >= 0) {
      if (!setThreadCore(core_id)) {
        std::cerr << "Failed to set core affinity for thread '" << name
                  << "' (ID: " << getCurrentThreadId() << ") to core "
                  << core_id << std::endl;
        // std::exit(EXIT_FAILURE);
      } else {
        std::cerr << "Set core affinity for thread '" << name
                  << "' (ID: " << getCurrentThreadId() << ") to core "
                  << core_id << std::endl;
      }
    }

    // Execute the function

    std::forward<T>(func)(std::forward<A>(args)...);
  });

  // Small delay to allow thread to start
  using namespace std::literals::chrono_literals;
  std::this_thread::sleep_for(10ms); // Reduced from 1s for better performance

  return t;
}

// RAII wrapper for thread management
class ManagedThread {
private:
  std::thread *thread_;
  std::string name_;

public:
  ManagedThread(int core_id, const std::string &name,
                std::function<void()> func)
      : thread_(nullptr), name_(name) {
    thread_ = createAndStartThread(core_id, name, std::move(func));
  }

  ~ManagedThread() {
    if (thread_) {
      if (thread_->joinable()) {
        thread_->join();
      }
      delete thread_;
    }
  }

  // Non-copyable
  ManagedThread(const ManagedThread &) = delete;
  ManagedThread &operator=(const ManagedThread &) = delete;

  // Movable
  ManagedThread(ManagedThread &&other) noexcept
      : thread_(other.thread_), name_(std::move(other.name_)) {
    other.thread_ = nullptr;
  }

  ManagedThread &operator=(ManagedThread &&other) noexcept {
    if (this != &other) {
      if (thread_) {
        if (thread_->joinable()) {
          thread_->join();
        }
        delete thread_;
      }
      thread_ = other.thread_;
      name_ = std::move(other.name_);
      other.thread_ = nullptr;
    }
    return *this;
  }

  bool isValid() const noexcept { return thread_ != nullptr; }

  void join() {
    if (thread_ && thread_->joinable()) {
      thread_->join();
    }
  }

  bool joinable() const { return thread_ && thread_->joinable(); }

  const std::string &getName() const noexcept { return name_; }
};

// Utility function to print system information
inline void printSystemInfo() {
  std::cout << "System Information:" << std::endl;
  std::cout << "  Available CPU cores: " << getNumCores() << std::endl;

#ifdef _WIN32
  std::cout << "  Platform: Windows" << std::endl;
#elif defined(__APPLE__)
  std::cout << "  Platform: macOS" << std::endl;
#elif defined(__linux__)
  std::cout << "  Platform: Linux" << std::endl;
#else
  std::cout << "  Platform: Unknown" << std::endl;
#endif
}

} // namespace Common

namespace thread_example {
auto dummyFunction(int a, int b, bool sleep);
int main();

} // namespace thread_example