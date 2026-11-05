#ifndef SMARTEXAM_REVIEW_SERVICE_H
#define SMARTEXAM_REVIEW_SERVICE_H
#include "storage/Store.h"
namespace exam {
// Изменять оценку может только вошедший преподаватель.
class ReviewService {
    Store &store;

  public:
    explicit ReviewService(Store &storage) : store(storage) {}
    void revise(const ReviewVerdict &verdict) {
        store.requireTeacher();
        store.reviewPart(verdict);
    }
    std::vector<ReviewVerdict> history(const std::string &attempt) {
        return store.reviewHistory(attempt);
    }
    void replay() {
        store.requireTeacher();
        store.replayModels();
    }
};
} // namespace exam
#endif
