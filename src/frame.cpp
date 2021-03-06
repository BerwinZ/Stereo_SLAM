/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "myslam/frame.h"
#include "myslam/feature.h"
#include "myslam/mappoint.h"
#include "myslam/config.h"


namespace myslam {

// Frame::Frame(long id, double time_stamp, const SE3 &pose, const Mat &left, const Mat &right)
//         : id_(id), time_stamp_(time_stamp), pose_(pose), left_img_(left), right_img_(right) { }

Frame::Ptr Frame::CreateFrame() {
    static long factory_id = 0;
    Frame::Ptr new_frame(new Frame);
    new_frame->id_ = factory_id++;
    return new_frame;
}

void Frame::SetKeyFrame() {
    static long keyframe_factory_id = 0;
    is_keyframe_ = true;
    keyframe_id_ = keyframe_factory_id++;
}

void Frame::UpdateCovisibleConnections() {

    std::unique_lock<std::mutex> lock(connectedframe_mutex_);

    // Calcualte the co-visible frames' count
    std::unordered_map<Frame::Ptr, int> KFCounter;
    for(auto& fe : features_left_) {
        if(auto mappoint = fe->map_point_.lock()) {
            for(auto& ob : mappoint->GetObs()) {

                if(auto observed_fea = ob.lock()) {

                    auto observed_frame = observed_fea->frame_.lock();
                    if(observed_frame && observed_frame->keyframe_id_ != this->keyframe_id_) {
                        KFCounter[observed_frame]++;
                    }
                }
            }
        }
    }
    if(KFCounter.empty())
        return;

    // Filter the connections whose weight larger than 15
    int maxCount = 0;
    Frame::Ptr maxCountKF;
    connectedKeyFramesCounter_.clear();

    for(auto& kf : KFCounter) {
        if(kf.second >= 15) {
            connectedKeyFramesCounter_[kf.first] = kf.second;
            kf.first->AddConnection(Ptr(this), kf.second);
        }
        if(kf.second > maxCount) {
            maxCount = kf.second;
            maxCountKF = kf.first;
        }
    }
    if(connectedKeyFramesCounter_.empty()) {
        connectedKeyFramesCounter_[maxCountKF] = maxCount;
        maxCountKF->AddConnection(Ptr(this), maxCount);
    }

    // Sort the connections with weight
    ResortConnectedKeyframes();
}

void Frame::AddConnection(Frame::Ptr frame, const int& weight) {
    std::unique_lock<std::mutex> lock(connectedframe_mutex_);

    connectedKeyFramesCounter_[frame] = weight;
    ResortConnectedKeyframes();
}

void Frame::ResortConnectedKeyframes() {
    std::vector<std::pair<int, Frame::Ptr>> connectedKF;
    for(auto& kf : connectedKeyFramesCounter_) {
        connectedKF.push_back(std::make_pair(kf.second, kf.first));
    }
    std::sort(connectedKF.begin(), connectedKF.end());
    std::list<Frame::Ptr> tmpOrderedConnectedKeyFrames;
    for(auto& kf : connectedKF) {
        tmpOrderedConnectedKeyFrames.push_front(kf.second);
    }
    orderedConnectedKeyFrames_.clear();
    orderedConnectedKeyFrames_ = std::vector<Frame::Ptr>(tmpOrderedConnectedKeyFrames.begin(),
                                                         tmpOrderedConnectedKeyFrames.end());
}

std::set<Frame::Ptr> Frame::GetConnectedKeyFramesSet() {
    std::unique_lock<std::mutex> lock(connectedframe_mutex_);

    std::set<Frame::Ptr> tmp;
    for(auto& kf : connectedKeyFramesCounter_) {
        tmp.insert(kf.first);
    }
    return tmp;
}

std::vector<Frame::Ptr> Frame::GetOrderedConnectedKeyFramesVector(unsigned int size) {
    std::unique_lock<std::mutex> lock(connectedframe_mutex_);

    if(orderedConnectedKeyFrames_.size() <= size) {
        return orderedConnectedKeyFrames_;
    }
    else {
        return std::vector<Frame::Ptr>(orderedConnectedKeyFrames_.begin(), 
                                       orderedConnectedKeyFrames_.begin() + size);
    }
}



}   // namespace
