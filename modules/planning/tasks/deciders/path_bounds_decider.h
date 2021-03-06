/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#pragma once

#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "modules/planning/proto/decider_config.pb.h"
#include "modules/planning/proto/planning_config.pb.h"
#include "modules/planning/tasks/deciders/decider.h"

namespace apollo {
namespace planning {

class PathBoundsDecider : public Decider {
 public:
  enum class LaneBorrowInfo {
    LEFT_BORROW,
    NO_BORROW,
    RIGHT_BORROW,
  };
  explicit PathBoundsDecider(const TaskConfig& config);

 private:
  common::Status Process(Frame* frame,
                         ReferenceLineInfo* reference_line_info) override;

  void InitPathBoundsDecider(const Frame& frame,
                             const ReferenceLineInfo& reference_line_info);

  /** @brief: The regular path boundary generation considers the ADC itself
   *   and other static environments:
   *   - ADC's position (lane-changing considerations)
   *   - lane info
   *   - static obstacles
   *   The philosophy is: static environment must be and can only be taken
   *   care of by the path planning.
   * @param: frame
   * @param: reference_line_info
   * @param: lane_borrow_info: which lane to borrow.
   * @param: The generated regular path_boundary, if there is one.
   * @return: A failure message. If succeeded, return "" (empty string).
   */
  std::string GenerateRegularPathBoundary(
      const Frame& frame, const ReferenceLineInfo& reference_line_info,
      const LaneBorrowInfo lane_borrow_info,
      std::vector<std::tuple<double, double, double>>* const path_boundary);

  /** @brief: The fallback path only considers:
   *   - ADC's position (so that boundary must contain ADC's position)
   *   - lane info
   *   It is supposed to be the last resort in case regular path generation
   *   fails so that speed decider can at least have some path and won't
   *   fail drastically.
   *   Therefore, it be reliable so that optimizer will not likely to
   *   fail with this boundary, and therefore doesn't consider any static
   *   obstacle. When the fallback path is used, stopping before static
   *   obstacles should be taken care of by the speed decider. Also, it
   *   doesn't consider any lane-borrowing.
   * @param: frame
   * @param: reference_line_info
   * @param: The generated fallback path_boundary, if there is one.
   * @return: A failure message. If succeeded, return "" (empty string).
   */
  std::string GenerateFallbackPathBoundary(
      Frame* frame, ReferenceLineInfo* reference_line_info,
      std::vector<std::tuple<double, double, double>>* const path_boundaries);

  /** @brief: Initializes an empty path boundary.
   */
  bool InitPathBoundary(
      const ReferenceLine& reference_line,
      std::vector<std::tuple<double, double, double>>* const path_boundary);

  /** @brief: Refine the boundary based on lane-info and ADC's location.
   *   It will comply to the lane-boundary. However, if the ADC itself
   *   is out of the given lane(s), it will adjust the boundary
   *   accordingly to include ADC's current position.
   */
  bool GetBoundaryFromLanesAndADC(
      const ReferenceLine& reference_line,
      const LaneBorrowInfo lane_borrow_info, double ADC_buffer,
      std::vector<std::tuple<double, double, double>>* const path_boundaries);

  bool GetBoundaryFromStaticObstacles(
      const PathDecision& path_decision,
      std::vector<std::tuple<double, double, double>>* const path_boundaries);

  bool GetLaneInfoFromPoint(double point_x, double point_y, double point_z,
                            double point_theta,
                            hdmap::LaneInfoConstPtr* const lane);

  double GetBufferBetweenADCCenterAndEdge();

  std::vector<std::tuple<int, double, double, double, std::string>>
  SortObstaclesForSweepLine(
      const IndexedList<std::string, Obstacle>& indexed_obstacles);

  std::vector<std::vector<std::tuple<double, double, double>>>
  ConstructSubsequentPathBounds(
      const std::vector<std::tuple<int, double, double, double, std::string>>&
          sorted_obstacles,
      size_t path_idx, size_t obs_idx,
      std::unordered_map<std::string, std::tuple<bool, double>>* const
          obs_id_to_details,
      std::vector<std::tuple<double, double, double>>* const curr_path_bounds);

  std::vector<std::vector<bool>> DecidePassDirections(
      double l_min, double l_max,
      const std::vector<std::tuple<int, double, double, double, std::string>>&
          new_entering_obstacles);

  /** @brief Update the path_boundary at "idx", as well as the new center-line.
   *        It also checks if ADC is blocked (lmax < lmin).
   * @param The current index of the path_bounds
   * @param The minimum left boundary (l_max)
   * @param The maximum right boundary (l_min)
   * @param The path_boundaries (its content at idx will be updated)
   * @param The center_line (to be updated)
   * @return If path is good, true; if path is blocked, false.
   */
  bool UpdatePathBoundaryAndCenterLine(
      size_t idx, double left_bound, double right_bound,
      std::vector<std::tuple<double, double, double>>* const path_boundaries,
      double* const center_line);

  void TrimPathBounds(
      const int path_blocked_idx,
      std::vector<std::tuple<double, double, double>>* const path_boundaries);

  void PathBoundsDebugString(
      const std::vector<std::tuple<double, double, double>>& path_boundaries);

 private:
  std::string blocking_obstacle_id_ = "";
  double adc_frenet_s_ = 0.0;
  double adc_frenet_sd_ = 0.0;
  double adc_frenet_l_ = 0.0;
  double adc_frenet_ld_ = 0.0;
  double adc_lane_width_ = 0.0;
  hdmap::LaneInfoConstPtr adc_lane_info_;
};

}  // namespace planning
}  // namespace apollo
