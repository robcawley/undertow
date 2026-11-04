#pragma once
#include "Specs.h"
#include <atomic>
namespace ut {
struct MotionPoint {
  float bar = 0, x = .5f, y = .5f;
};
struct ExtraState {
  std::array<float, count> sceneB{};
  std::array<std::array<float, count>, 4> corners{};
  std::array<uint16_t, 6> patterns{65535, 65535, 65535, 65535, 65535, 65535};
  std::array<MotionPoint, 1024> motion{};
  int motionCount = 0, width = 1280, height = 800, preset = 0;
  bool edited = false; // RETURN is the xy_return parameter (0.6.0)
};
// One writer and one reader each own a slot. The atomic middle slot transfers
// ownership; the writer never overwrites the reader's current frame.
class StateExchange {
public:
  void publish(const ExtraState &state) noexcept {
    slots[(size_t)back] = state;
    back = middle.exchange(back | dirty, std::memory_order_acq_rel) & mask;
  }
  const ExtraState &consume() noexcept {
    if (middle.load(std::memory_order_acquire) & dirty)
      front = middle.exchange(front, std::memory_order_acq_rel) & mask;
    return slots[(size_t)front];
  }

private:
  static_assert(std::atomic<int>::is_always_lock_free);
  static constexpr int dirty = 4, mask = 3;
  std::array<ExtraState, 3> slots{};
  std::atomic<int> middle{1};
  int front = 0, back = 2;
};
inline bool snapshotParameter(int index) {
  return specs[(size_t)index].kind == 0 && index != mix && index != out_gain &&
         index != bloom && !(index >= xy_x && index <= xy_motion_play);
}
} // namespace ut
