#ifndef MYSLAM_LOOPCLOSING_H
#define MYSLAM_LOOPCLOSING_H

#include "myslam/common_include.h"
#include "myslam/ORBVocabulary.h"
#include <opencv2/features2d.hpp>

namespace myslam {

class Map;
class Frame;

class LoopClosing {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    typedef std::shared_ptr<LoopClosing> Ptr;

    // Start the loop closure detection thread and keep it
    LoopClosing(ORBVocabulary* vocabulary) {
        mpORBvocabulary_ = std::shared_ptr<ORBVocabulary>(vocabulary);
        loopclosing_running_.store(true);
        loopclosing_thread_ = std::thread(std::bind(&LoopClosing::Run, this));
    }

    // Start the detection once
    void DetectLoop(std::shared_ptr<Frame> frame) {
        std::unique_lock<std::mutex> lock(data_mutex_);
        curr_keyframe_ = frame;
        map_update_.notify_one();
    }

    // Stop the loop closure detection thread
    void Stop() {
        loopclosing_running_.store(false);
        loopclosing_thread_.join();
    }

    void SetMap(std::shared_ptr<Map> map) {map_ = map; }

    void SetORBExtractor(cv::Ptr<cv::ORB> orb) { orb_ = orb; }

private:
    // The main function of loop closure detection
    void Run();

    // Covert the type of descriptors to use for computing BoW
    std::vector<cv::Mat> toDescriptorVector(const cv::Mat &Descriptors);

    // Compute the BoW
    void ComputeBoW();

    // Compute the score of current key_frame and previous keyframes
    void ComputeScore();

    // The thread of loop closure detection
    std::thread loopclosing_thread_;

    // The flag to indicate whether the thread is running
    std::atomic<bool> loopclosing_running_;

    // Lock of current thread. Used by std::unique_lock
    std::mutex data_mutex_;

    // The variable to trigger detect once
    std::condition_variable map_update_;

    // New insert key frame
    std::shared_ptr<Frame> curr_keyframe_;

    // ORB dictionary
    std::shared_ptr<ORBVocabulary> mpORBvocabulary_;

    // ORB extractor
    cv::Ptr<cv::ORB> orb_;

    // Map
    std::shared_ptr<Map> map_;
};

} // namespace

#endif // LOOPCLOSING_H