#pragma once

#include "common.hpp"

#include <array>
#include <cinttypes>
#include <type_traits>
#include <vector>
#include <memory>
#ifndef ENABLE_BITWISE_ENUM
#define ENABLE_BITWISE_ENUM(type)                                                                                      \
  constexpr type operator|(type a, type b) noexcept {                                                                  \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(static_cast<T>(a) | static_cast<T>(b));                                                                \
  }                                                                                                                    \
  constexpr type operator&(type a, type b) noexcept {                                                                  \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(static_cast<T>(a) & static_cast<T>(b));                                                                \
  }                                                                                                                    \
  constexpr type& operator|=(type& a, type b) noexcept {                                                               \
    using T = std::underlying_type_t<type>;                                                                            \
    a = type(static_cast<T>(a) | static_cast<T>(b));                                                                   \
    return a;                                                                                                          \
  }                                                                                                                    \
  constexpr type& operator&=(type& a, type b) noexcept {                                                               \
    using T = std::underlying_type_t<type>;                                                                            \
    a = type(static_cast<T>(a) & static_cast<T>(b));                                                                   \
    return a;                                                                                                          \
  }                                                                                                                    \
  constexpr type operator~(type key) noexcept {                                                                        \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(~static_cast<T>(key));                                                                                 \
  }                                                                                                                    \
  constexpr bool True(type key) noexcept {                                                                             \
    using T = std::underlying_type_t<type>;                                                                            \
    return static_cast<T>(key) != 0;                                                                                   \
  }                                                                                                                    \
  constexpr bool False(type key) noexcept {                                                                            \
    using T = std::underlying_type_t<type>;                                                                            \
    return static_cast<T>(key) == 0;                                                                                   \
  }
#endif

namespace aurora {
enum class SpecialKey : uint8_t {
  None = 0,
  F1 = 1,
  F2 = 2,
  F3 = 3,
  F4 = 4,
  F5 = 5,
  F6 = 6,
  F7 = 7,
  F8 = 8,
  F9 = 9,
  F10 = 10,
  F11 = 11,
  F12 = 12,
  F13 = 13,
  F14 = 14,
  F15 = 15,
  F16 = 16,
  F17 = 17,
  F18 = 18,
  F19 = 19,
  F20 = 20,
  F21 = 21,
  F22 = 22,
  F23 = 23,
  F24 = 24,
  Esc = 25,
  Enter = 26,
  Backspace = 27,
  Insert = 28,
  Delete = 29,
  Home = 30,
  End = 31,
  PgUp = 32,
  PgDown = 33,
  Left = 34,
  Right = 35,
  Up = 36,
  Down = 37,
  Tab = 38,
  PrintScreen = 39,
  ScrollLock = 40,
  Pause = 41,
  NumLockClear = 42,
  KpDivide = 43,
  KpMultiply = 44,
  KpMinus = 45,
  KpPlus = 46,
  KpEnter = 47,
  KpNum0 = 48,
  KpNum1 = 49,
  KpNum2 = 51,
  KpNum3 = 52,
  KpNum4 = 53,
  KpNum5 = 54,
  KpNum6 = 55,
  KpNum7 = 56,
  KpNum8 = 57,
  KpNum9 = 58,
  KpPercent = 59,
  KpPeriod = 60,
  KpComma = 61,
  KpEquals = 62,
  Application = 63,
  Power = 64,
  Execute = 65,
  Help = 66,
  Menu = 67,
  Select = 68,
  Stop = 69,
  Again = 70,
  Undo = 71,
  Cut = 72,
  Paste = 73,
  Find = 74,
  VolumeUp = 75,
  VolumeDown = 76,
  MAX,
};
enum class ModifierKey : uint16_t {
  None = 0,
  LeftShift = 1 << 0,
  RightShift = 1 << 1,
  LeftControl = 1 << 2,
  RightControl = 1 << 3,
  LeftAlt = 1 << 3,
  RightAlt = 1 << 4,
  LeftGui = 1 << 5,
  RightGui = 1 << 6,
  Num = 1 << 7,
  Caps = 1 << 8,
  Mode = 1 << 9,
  // SDL has a reserved value we don't need
};
ENABLE_BITWISE_ENUM(ModifierKey);

enum class ControllerButton : uint8_t {
  A,
  B,
  X,
  Y,
  Back,
  Guide,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DPadUp,
  DPadDown,
  DPadLeft,
  DPadRight,
  Other,
  MAX,
};
enum class ControllerAxis : uint8_t {
  LeftX,
  LeftY,
  RightX,
  RightY,
  TriggerLeft,
  TriggerRight,
  MAX,
};
struct Icon {
  std::unique_ptr<uint8_t[]> data;
  uint32_t width;
  uint32_t height;
};
struct WindowSize {
  uint32_t width;
  uint32_t height;
};
enum class MouseButton {
  None = 0,
  Primary = 1 << 0,
  Middle = 1 << 1,
  Secondary = 1 << 2,
  Aux1 = 1 << 3,
  Aux2 = 1 << 4,
};
ENABLE_BITWISE_ENUM(MouseButton);

enum class Backend : uint8_t {
  Invalid,
  Vulkan,
  Metal,
  D3D12,
  D3D11,
  OpenGL,
  OpenGLES,
  WebGPU,
};

struct App;
struct AppDelegate {
  AppDelegate() = default;
  virtual ~AppDelegate() noexcept = default;
  AppDelegate(const AppDelegate&) = delete;
  AppDelegate& operator=(const AppDelegate&) = delete;
  AppDelegate(AppDelegate&&) = delete;
  AppDelegate& operator=(AppDelegate&&) = delete;

  virtual void onAppLaunched() noexcept = 0;
  virtual bool onAppIdle(float dt) noexcept = 0;
  virtual void onAppDraw() noexcept = 0;
  virtual void onAppPostDraw() noexcept = 0;
  virtual void onAppWindowResized(const WindowSize& size) noexcept = 0;
  virtual void onAppWindowMoved(int32_t x, int32_t y) noexcept = 0;
  virtual void onAppDisplayScaleChanged(float scale) noexcept = 0;
  virtual void onAppExiting() noexcept = 0;

  // ImGui
  virtual void onImGuiInit(float scale) noexcept = 0;
  virtual void onImGuiAddTextures() noexcept = 0;

  // Input
  virtual void onCharKeyDown(uint8_t charCode, ModifierKey mods, bool isRepeat) noexcept = 0;
  virtual void onCharKeyUp(uint8_t charCode, ModifierKey mods) noexcept = 0;
  virtual void onSpecialKeyDown(SpecialKey key, ModifierKey mods, bool isRepeat) noexcept = 0;
  virtual void onSpecialKeyUp(SpecialKey key, ModifierKey mods) noexcept = 0;
  virtual void onModifierKeyDown(ModifierKey mods, bool isRepeat) noexcept = 0;
  virtual void onModifierKeyUp(ModifierKey mods) noexcept = 0;
  virtual void onTextInput(const std::string& input) noexcept = 0;
  virtual void onMouseMove(int32_t x, int32_t y, int32_t xrel, int32_t yrel, MouseButton state) noexcept = 0;
  virtual void onMouseButtonDown(int32_t x, int32_t y, MouseButton button, int32_t clicks) noexcept = 0;
  virtual void onMouseButtonUp(int32_t x, int32_t y, MouseButton button) noexcept = 0;

  // Controller
  virtual void onControllerAdded(uint32_t which) noexcept = 0;
  virtual void onControllerRemoved(uint32_t which) noexcept = 0;
  virtual void onControllerButton(uint32_t which, ControllerButton button, bool pressed) noexcept = 0;
  virtual void onControllerAxis(uint32_t which, ControllerAxis axis, int16_t value) noexcept = 0;
};

void app_run(std::unique_ptr<AppDelegate> app, Icon icon, int argc, char** argv) noexcept;
[[nodiscard]] std::vector<std::string> get_args() noexcept;
[[nodiscard]] WindowSize get_window_size() noexcept;
void set_window_title(zstring_view title) noexcept;
[[nodiscard]] Backend get_backend() noexcept;
[[nodiscard]] std::string_view get_backend_string() noexcept;
void set_fullscreen(bool fullscreen) noexcept;
[[nodiscard]] int32_t get_controller_player_index(uint32_t which) noexcept;
void set_controller_player_index(uint32_t which, int32_t index) noexcept;
[[nodiscard]] bool is_controller_gamecube(uint32_t which) noexcept;
[[nodiscard]] bool controller_has_rumble(uint32_t which) noexcept;
void controller_rumble(uint32_t which, uint16_t low_freq_intensity, uint16_t high_freq_intensity, uint32_t duration_ms = 0) noexcept;
[[nodiscard]] std::string get_controller_name(uint32_t which) noexcept;
} // namespace aurora