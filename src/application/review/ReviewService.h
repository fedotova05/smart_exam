#ifndef SMARTEXAM_REVIEW_SERVICE_H
#define SMARTEXAM_REVIEW_SERVICE_H
#include "storage/Store.h"
namespace exam {
// Право преподавателя проверяет вызывающий слой доступа (T20).
class ReviewService {
    Store &store;

  public:
    explicit ReviewService(Store &storage) : store(storage) {}
    void revise(const ReviewVerdict &verdict) {
        store.reviewPart(verdict);
    }
    std::vector<ReviewVerdict> history(const std::string &attempt) {
        return store.reviewHistory(attempt);
    }
    void replay() {
        store.replayModels();
    }
};
} // namespace exam
#endif
