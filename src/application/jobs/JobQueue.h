#ifndef SMARTEXAM_JOB_QUEUE_H
#define SMARTEXAM_JOB_QUEUE_H
#include "storage/Store.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <windows.h>
namespace exam {
class JobCancelled : public std::runtime_error {
  public: JobCancelled() : std::runtime_error("Операция отменена; уже сохранённые данные остаются в базе") {}
};
struct JobContext {
    std::atomic<bool> cancelled;
    std::chrono::steady_clock::time_point deadline;
    JobContext() : cancelled(false) {}
    void checkpoint() const;
};
struct JobResult {
    unsigned id, screen;
    bool success, cancelled;
    std::string text;
};
class JobQueue {
    struct Request {
        unsigned id, screen;
        int seconds;
        std::shared_ptr<JobContext> context;
        std::function<std::string(Store &, JobContext &)> work;
    };
    std::string path;
    CRITICAL_SECTION mutex;
    HANDLE wake, thread;
    bool stopping;
    unsigned nextId;
    std::deque<Request> requests;
    std::deque<JobResult> results;
    std::map<unsigned,Request> active;
    static DWORD WINAPI entry(void *self);
    void run();
  public:
    explicit JobQueue(const std::string &databasePath);
    ~JobQueue();
    JobQueue(const JobQueue &) = delete;
    JobQueue &operator=(const JobQueue &) = delete;
    unsigned submit(unsigned screen, const std::function<std::string(Store &, JobContext &)> &work,
                    int seconds = 30);
    void cancel(unsigned id);
    void leaveScreen(unsigned screen);
    bool poll(unsigned screen, JobResult &result);
};
}
#endif
