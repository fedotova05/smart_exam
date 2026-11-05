#include "JobQueue.h"
namespace exam {
namespace {
class Lock {
    CRITICAL_SECTION &mutex;

  public:
    explicit Lock(CRITICAL_SECTION &value) : mutex(value) {
        EnterCriticalSection(&mutex);
    }
    ~Lock() {
        LeaveCriticalSection(&mutex);
    }
};
} // namespace
void JobContext::checkpoint() const {
    if (cancelled.load())
        throw JobCancelled();
    if (std::chrono::steady_clock::now() > deadline)
        throw std::runtime_error("Превышено время операции");
}
JobQueue::JobQueue(const std::string &databasePath)
    : path(databasePath), wake(0), thread(0), stopping(false), nextId(1) {
    if (path.empty() || path == ":memory:")
        throw std::runtime_error("Для фоновых операций нужна файловая база");
    InitializeCriticalSection(&mutex);
    wake = CreateEvent(0, FALSE, FALSE, 0);
    if (wake)
        thread = CreateThread(0, 0, entry, this, 0, 0);
    if (!wake || !thread) {
        if (wake)
            CloseHandle(wake);
        DeleteCriticalSection(&mutex);
        throw std::runtime_error("Не удалось запустить рабочий поток");
    }
}
JobQueue::~JobQueue() {
    {
        Lock lock(mutex);
        stopping = true;
        for (auto &entry : active)
            entry.second.context->cancelled = true;
    }
    SetEvent(wake);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(wake);
    DeleteCriticalSection(&mutex);
}
DWORD WINAPI JobQueue::entry(void *self) {
    static_cast<JobQueue *>(self)->run();
    return 0;
}
unsigned JobQueue::submit(unsigned screen,
                          const std::function<std::string(Store &, JobContext &)> &work,
                          int seconds) {
    if (!work || seconds < 1 || seconds > 300)
        throw std::invalid_argument("Неверная фоновая операция");
    Lock lock(mutex);
    if (active.size() >= 100 || requests.size() >= 100 || results.size() >= 100)
        throw std::runtime_error("Очередь заполнена");
    Request request;
    request.id = nextId++;
    request.screen = screen;
    request.seconds = seconds;
    request.work = work;
    request.context.reset(new JobContext);
    requests.push_back(request);
    active[request.id] = request;
    SetEvent(wake);
    return request.id;
}
void JobQueue::cancel(unsigned id) {
    Lock lock(mutex);
    auto found = active.find(id);
    if (found != active.end())
        found->second.context->cancelled = true;
}
void JobQueue::leaveScreen(unsigned screen) {
    Lock lock(mutex);
    for (auto it = active.begin(); it != active.end();) {
        if (it->second.screen == screen) {
            it->second.context->cancelled = true;
            it = active.erase(it);
        } else
            ++it;
    }
    for (auto it = results.begin(); it != results.end();)
        if (it->screen == screen)
            it = results.erase(it);
        else
            ++it;
}
bool JobQueue::poll(unsigned screen, JobResult &result) {
    Lock lock(mutex);
    for (auto it = results.begin(); it != results.end(); ++it)
        if (it->screen == screen) {
            result = *it;
            results.erase(it);
            return true;
        }
    return false;
}
void JobQueue::run() {
    // Соединение создаётся и используется только этим потоком.
    std::unique_ptr<Store> storage;
    std::string openError;
    try {
        storage.reset(new Store(path));
    } catch (const std::exception &error) {
        openError = error.what();
    }
    for (;;) {
        Request request;
        bool available = false;
        {
            Lock lock(mutex);
            if (stopping)
                return;
            if (!requests.empty()) {
                request = requests.front();
                requests.pop_front();
                available = true;
            }
        }
        if (!available) {
            WaitForSingleObject(wake, INFINITE);
            continue;
        }
        JobResult result;
        result.id = request.id;
        result.screen = request.screen;
        result.success = false;
        result.cancelled = false;
        request.context->deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(request.seconds);
        try {
            request.context->checkpoint();
            if (!storage)
                throw std::runtime_error(openError);
            result.text = request.work(*storage, *request.context);
            result.success = true;
        } catch (const JobCancelled &error) {
            result.cancelled = true;
            result.text = error.what();
        } catch (const std::exception &error) {
            result.text = error.what();
        } catch (...) {
            result.text = "Неизвестная ошибка фоновой операции";
        }
        Lock lock(mutex);
        if (active.erase(request.id))
            results.push_back(result);
    }
}
} // namespace exam
