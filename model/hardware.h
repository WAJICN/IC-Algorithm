#ifndef MODEL_HARDWARE_H_
#define MODEL_HARDWARE_H_

#include <algorithm>
#include <vector>

namespace dfg_ilp {

struct Hardware {
  int min_slice = -46;
  int max_slice = 46;
  int stream_count = 32;
  int k_stream = 1;
  int default_icu_capacity = 1;
  int max_issue_time = 2000;
  int big_m = 10000;

  std::vector<int> AllSlices() const {
    std::vector<int> slices;
    for (int p = min_slice; p <= max_slice; ++p) {
      slices.push_back(p);
    }
    return slices;
  }

  bool IsHardwareLegalSlice(int slice) const {
    return slice >= min_slice && slice <= max_slice;
  }

  int IcuCapacity(int slice) const {
    if (!IsHardwareLegalSlice(slice)) {
      return 0;
    }
    return default_icu_capacity;
  }

  std::vector<int> IntersectLegalSlices(const std::vector<int>& requested) const {
    std::vector<int> legal;
    for (const int slice : requested) {
      if (IsHardwareLegalSlice(slice) &&
          std::find(legal.begin(), legal.end(), slice) == legal.end()) {
        legal.push_back(slice);
      }
    }
    std::sort(legal.begin(), legal.end());
    return legal;
  }
};

}  // namespace dfg_ilp

#endif  // MODEL_HARDWARE_H_
