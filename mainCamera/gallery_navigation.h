#pragma once

struct GalleryNavigation {
  int index = 0;
  int count = 0;
  bool grid = false;
  static constexpr int page_size = 8;
  int page_start() const { return index / page_size * page_size; }
  bool move(int direction) {
    int next = index + direction;
    if (next < 0 || next >= count) return false;
    index = next;
    return true;
  }
  void down() { if (count > 0) grid = true; }
  // Up first opens the selected thumbnail; only Up in single view exits.
  bool up_exits() {
    if (!grid) return true;
    grid = false;
    return false;
  }
};
