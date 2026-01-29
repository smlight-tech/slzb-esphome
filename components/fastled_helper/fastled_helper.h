#pragma once

namespace esphome {
namespace fastled_helper {

static const char *const FASTLED_HELPER_VERSION = "2025.10.1";
static const char *const TAG = "fastled_helper";

class FastledHelper : public Component {
 public:
  void setup() override;
  void dump_config() override;

#ifdef PALETTES
  void set_current_palette(int index);
#endif
};  // FastledHelper

}  // namespace fastled_helper
}  // namespace esphome
