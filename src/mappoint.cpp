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

#include "myslam/mappoint.h"
#include "myslam/feature.h"

namespace myslam {

MapPoint::MapPoint(long id, Vec3 position) : id_(id), pos_(position) {}

MapPoint::Ptr MapPoint::CreateNewMappoint() {
    static long factory_id = 0;
    MapPoint::Ptr new_mappoint(new MapPoint);
    new_mappoint->id_ = factory_id++;
    return new_mappoint;
}

void MapPoint::AddKFObservation(std::shared_ptr<Feature> feature) {
    std::unique_lock<std::mutex> lck(data_mutex_);
    observations_.push_back(feature);
    active_observations_.push_back(feature);
}

void MapPoint::RemoveKFObservation(std::shared_ptr<Feature> feature) {
    std::unique_lock<std::mutex> lck(data_mutex_);
    for (auto iter = observations_.begin(); iter != observations_.end(); iter++) {
        if (iter->lock() == feature) {
            observations_.erase(iter);
            // This observed feature is determined mismatch by backend, it shouldn't related to this mappoint
            break;
        }
    }

    LOG(INFO) << "Remove observation, current is " << observations_.size();

    RemoveActiveKFObservation(feature);

    if (observations_.size() == 0)
        is_outlier_ = true;
}

void MapPoint::RemoveActiveKFObservation(std::shared_ptr<Feature> feature) {
    std::unique_lock<std::mutex> lck(data_mutex_);
    for (auto iter = active_observations_.begin(); iter != active_observations_.end(); iter++) {
        if (iter->lock() == feature) {
            active_observations_.erase(iter);
            break;
        }
    }
}

}  // namespace myslam
