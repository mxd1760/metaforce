﻿#include "Runtime/ConsoleVariables/CVar.hpp"
#include "Runtime/CBasics.hpp"

#include <logvisor/logvisor.hpp>

#include <sstream>

#include "Runtime/ConsoleVariables/CVarManager.hpp"

namespace metaforce {
namespace {
// TODO: Move these to CBasics?
inline bool parseBool(std::string_view boolean, bool* valid) {
  std::string val(boolean);
  // compare must be case insensitive
  // This is the cleanest solution since I only need to do it once
  CBasics::ToLower(val);

  // Check for true first
  if (val == "true" || val == "1" || val == "yes" || val == "on") {
    if (valid)
      *valid = true;

    return true;
  }

  // Now false
  if (val == "false" || val == "0" || val == "no" || val == "off") {
    if (valid)
      *valid = true;

    return false;
  }

  // Well that could've gone better

  if (valid)
    *valid = false;

  return false;
}

static std::vector<std::string>& split(std::string_view s, char delim, std::vector<std::string>& elems) {
  std::string tmps(s);
  std::stringstream ss(tmps);
  std::string item;

  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }

  return elems;
}

std::vector<std::string> split(std::string_view s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}
}
extern CVar* com_developer;
extern CVar* com_enableCheats;

using namespace std::literals;

CVar::CVar(std::string_view name, std::string_view value, std::string_view help, CVar::EFlags flags)
: CVar(name, help, EType::Literal) {
  fromLiteral(value);
  init(flags);
}

CVar::CVar(std::string_view name, const zeus::CVector2f& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec2f) {
  fromVec2f(value);
  init(flags);
}

CVar::CVar(std::string_view name, const zeus::CVector2d& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec2d) {
  fromVec2d(value);

  init(flags);
}

CVar::CVar(std::string_view name, const zeus::CVector3f& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec3f) {
  fromVec3f(value);
  init(flags, false);
}

CVar::CVar(std::string_view name, const zeus::CVector3d& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec3d) {
  fromVec3d(value);
  init(flags, false);
}

CVar::CVar(std::string_view name, const zeus::CVector4f& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec4f) {
  fromVec4f(value);
  init(flags, false);
}

CVar::CVar(std::string_view name, const zeus::CVector4d& value, std::string_view help, EFlags flags)
: CVar(name, help, EType::Vec4d) {
  fromVec4d(value);
  init(flags, false);
}

CVar::CVar(std::string_view name, double value, std::string_view help, EFlags flags) : CVar(name, help, EType::Real) {
  fromReal(value);
  init(flags);
}

CVar::CVar(std::string_view name, bool value, std::string_view help, CVar::EFlags flags)
: CVar(name, help, EType::Boolean) {
  fromBoolean(value);
  init(flags);
}

CVar::CVar(std::string_view name, int32_t value, std::string_view help, CVar::EFlags flags)
: CVar(name, help, EType::Signed) {
  fromInteger(value);
  init(flags);
}

CVar::CVar(std::string_view name, uint32_t value, std::string_view help, CVar::EFlags flags)
: CVar(name, help, EType::Unsigned) {
  fromInteger(value);
  init(flags);
}

std::string CVar::help() const {
  return m_help + (m_defaultValue.empty() ? "" : "\ndefault: " + m_defaultValue) + (isReadOnly() ? " [ReadOnly]" : "");
}

zeus::CVector2f CVar::toVec2f(bool* isValid) const {
  if (m_type != EType::Vec2f) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<float, 2> f;
  std::sscanf(m_value.c_str(), "%g %g", &f[0], &f[1]);
  return {f[0], f[1]};
}

zeus::CVector2d CVar::toVec2d(bool* isValid) const {
  if (m_type != EType::Vec2d) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<double, 2> f;
  std::sscanf(m_value.c_str(), "%lg %lg", &f[0], &f[1]);
  return {f[0], f[1]};
}

zeus::CVector3f CVar::toVec3f(bool* isValid) const {
  if (m_type != EType::Vec3f) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<float, 3> f;
  std::sscanf(m_value.c_str(), "%g %g %g", &f[0], &f[1], &f[2]);
  return {f[0], f[1], f[2]};
}

zeus::CVector3d CVar::toVec3d(bool* isValid) const {
  if (m_type != EType::Vec3d) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<double, 3> f;
  std::sscanf(m_value.c_str(), "%lg %lg %lg", &f[0], &f[1], &f[2]);
  return {f[0], f[1], f[2]};
}

zeus::CVector4f CVar::toVec4f(bool* isValid) const {
  if (m_type != EType::Vec4f) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<float, 4> f;
  std::sscanf(m_value.c_str(), "%g %g %g %g", &f[0], &f[1], &f[2], &f[3]);
  return {f[0], f[1], f[2], f[3]};
}

zeus::CVector4d CVar::toVec4d(bool* isValid) const {
  if (m_type != EType::Vec4d) {
    if (isValid != nullptr)
      *isValid = false;

    return {};
  }

  if (isValid != nullptr)
    *isValid = true;

  std::array<double, 4> f{};
  std::sscanf(m_value.c_str(), "%lg %lg %lg %lg", &f[0], &f[1], &f[2], &f[3]);
  return {f[0], f[1], f[2], f[3]};
}

double CVar::toReal(bool* isValid) const {
  if (m_type != EType::Real) {
    if (isValid)
      *isValid = false;
    return 0.0f;
  }

  if (isValid != nullptr)
    *isValid = true;

  return strtod(m_value.c_str(), nullptr);
}

bool CVar::toBoolean(bool* isValid) const {
  if (m_type != EType::Boolean) {
    if (isValid)
      *isValid = false;

    return false;
  }

  return parseBool(m_value, isValid);
}

int32_t CVar::toSigned(bool* isValid) const {
  if (m_type != EType::Signed && m_type != EType::Unsigned) {
    if (isValid)
      *isValid = false;
    return 0;
  }

  if (isValid != nullptr)
    *isValid = true;

  return strtol(m_value.c_str(), nullptr, 0);
}

uint32_t CVar::toUnsigned(bool* isValid) const {
  if (m_type != EType::Signed && m_type != EType::Unsigned) {
    if (isValid)
      *isValid = false;
    return 0;
  }

  if (isValid != nullptr)
    *isValid = true;

  return strtoul(m_value.c_str(), nullptr, 0);
}

std::string CVar::toLiteral(bool* isValid) const {
  if (m_type != EType::Literal && (com_developer && com_developer->toBoolean())) {
    if (isValid != nullptr)
      *isValid = false;
  } else if (isValid != nullptr) {
    *isValid = true;
  }

  // Even if it's not a literal, it's still safe to return
  return m_value;
}

bool CVar::fromVec2f(const zeus::CVector2f& val) {
  if (!safeToModify(EType::Vec2f))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {}"), val.x(), val.y()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromVec2d(const zeus::CVector2d& val) {
  if (!safeToModify(EType::Vec2d))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {}"), val.x(), val.y()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromVec3f(const zeus::CVector3f& val) {
  if (!safeToModify(EType::Vec3f))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {} {}"), val.x(), val.y(), val.z()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromVec3d(const zeus::CVector3d& val) {
  if (!safeToModify(EType::Vec3d))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {} {}"), val.x(), val.y(), val.z()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromVec4f(const zeus::CVector4f& val) {
  if (!safeToModify(EType::Vec4f))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {} {} {}"), val.x(), val.y(), val.z(), val.w()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromVec4d(const zeus::CVector4d& val) {
  if (!safeToModify(EType::Vec4d))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{} {} {} {}"), val.x(), val.y(), val.z(), val.w()));
  m_flags |= EFlags::Modified;
  return true;
}

bool CVar::fromReal(double val) {
  if (!safeToModify(EType::Real))
    return false;

  m_value.assign(fmt::format(FMT_STRING("{}"), val));
  setModified();
  return true;
}

bool CVar::fromBoolean(bool val) {
  if (!safeToModify(EType::Boolean))
    return false;

  if (val)
    m_value = "true"sv;
  else
    m_value = "false"sv;

  setModified();
  return true;
}

bool CVar::fromInteger(int32_t val) {
  if ((com_developer && com_enableCheats) && (!com_developer->toBoolean() || !com_enableCheats->toBoolean()) &&
      isCheat())
    return false;

  // We'll accept both signed an unsigned input
  if (m_type != EType::Signed && m_type != EType::Unsigned)
    return false;

  if (isReadOnly() && (com_developer && !com_developer->toBoolean()))
    return false;

  // Properly format based on signedness
  m_value = fmt::format(FMT_STRING("{}"), (m_type == EType::Signed ? val : static_cast<uint32_t>(val)));
  setModified();
  return true;
}

bool CVar::fromInteger(uint32_t val) {
  if ((com_developer && com_enableCheats) && (!com_developer->toBoolean() || !com_enableCheats->toBoolean()) &&
      isCheat())
    return false;

  // We'll accept both signed an unsigned input
  if (m_type != EType::Signed && m_type != EType::Unsigned)
    return false;

  if (isReadOnly() && (com_developer && !com_developer->toBoolean()))
    return false;

  // Properly format based on signedness
  m_value = fmt::format(FMT_STRING("{}"), (m_type == EType::Unsigned ? val : static_cast<int32_t>(val)));
  setModified();
  return true;
}

bool CVar::fromLiteral(std::string_view val) {
  if (!safeToModify(EType::Literal))
    return false;

  m_value.assign(val);
  setModified();
  return true;
}

bool CVar::fromLiteralToType(std::string_view val) {
  if (!safeToModify(m_type) || !isValidInput(val))
    return false;
  m_value = val;
  setModified();
  return true;
}

bool CVar::isModified() const { return True(m_flags & EFlags::Modified); }
bool CVar::modificationRequiresRestart() const { return True(m_flags & EFlags::ModifyRestart); }

bool CVar::isReadOnly() const { return True(m_flags & EFlags::ReadOnly); }

bool CVar::isCheat() const { return True(m_flags & EFlags::Cheat); }

bool CVar::isHidden() const { return True(m_flags & EFlags::Hidden); }

bool CVar::isArchive() const { return True(m_flags & EFlags::Archive); }

bool CVar::isInternalArchivable() const { return True(m_flags & EFlags::InternalArchivable); }

bool CVar::isColor() const {
  return True(m_flags & EFlags::Color) && (m_type == EType::Vec3f || m_type == EType::Vec3d || m_type == EType::Vec3f ||
                                           m_type == EType::Vec4f || m_type == EType::Vec4d);
}

bool CVar::isNoDeveloper() const { return True(m_flags & EFlags::NoDeveloper); }

bool CVar::wasDeserialized() const { return m_wasDeserialized; }

bool CVar::hasDefaultValue() const { return m_defaultValue == m_value; }

void CVar::clearModified() {
  if (!modificationRequiresRestart())
    m_flags &= ~EFlags::Modified;
}

void CVar::setModified() { m_flags |= EFlags::Modified; }

void CVar::unlock() {
  if (isReadOnly() && !m_unlocked) {
    m_oldFlags = m_flags;
    m_flags &= ~EFlags::ReadOnly;
    m_unlocked = true;
  }
}

void CVar::lock() {
  if (!isReadOnly() && m_unlocked) {
    // We want to keep if we've been modified so we can inform our listeners
    bool modified = True(m_flags & EFlags::Modified);
    m_flags = m_oldFlags;
    // If we've been modified insert that back into m_flags
    if (modified) {
      m_flags |= EFlags::Modified;
    }
    m_unlocked = false;
  }
}

void CVar::dispatch() {
  for (const ListenerFunc& listen : m_listeners)
    listen(this);
  for (auto* ref : m_valueReferences) {
    ref->updateValue();
  }
}

bool isReal(std::string_view v) {
  char* p;
  std::strtod(v.data(), &p);
  return *p == 0;
}
bool isReal(const std::vector<std::string>& v) {
  for (auto& s : v) {
    if (!isReal(s))
      return false;
  }
  return true;
}

bool CVar::isValidInput(std::string_view input) const {
  std::vector<std::string> parts = split(input, ' ');
  char* p;
  switch (m_type) {
  case EType::Boolean: {
    bool valid = false;
    parseBool(input, &valid);
    return valid;
  }
  case EType::Signed:
    std::strtol(input.data(), &p, 0);
    return p == nullptr;
  case EType::Unsigned:
    std::strtoul(input.data(), &p, 0);
    return p == nullptr;
  case EType::Real: {
    bool size = parts.size() == 1;
    bool ret = isReal(input);
    return ret && size;
  }
  case EType::Literal:
    return true;
  case EType::Vec2f:
  case EType::Vec2d:
    return parts.size() == 2 && isReal(parts);
  case EType::Vec3f:
  case EType::Vec3d:
    return parts.size() == 3 && isReal(parts);
  case EType::Vec4f:
  case EType::Vec4d:
    return parts.size() == 4 && isReal(parts);
  }

  return false;
}

bool CVar::safeToModify(EType type) const {
  // Are we NoDevelper?
  if (isNoDeveloper())
    return false;

  // Are we a cheat?
  if (isCheat() && (com_developer && com_enableCheats) &&
      (!com_developer->toBoolean() || !com_enableCheats->toBoolean()))
    return false;

  // Are we read only?
  if (isReadOnly() && (com_developer && !com_developer->toBoolean()))
    return false;

  return m_type == type;
}

void CVar::init(EFlags flags, bool removeColor) {
  m_defaultValue = m_value;
  m_flags = flags;
  if (removeColor) {
    // If the user specifies color, we don't want it
    m_flags &= ~EFlags::Color;
  }
}

} // namespace hecl