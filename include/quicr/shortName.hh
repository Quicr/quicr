#pragma once

#include <cstdint>
#include <iostream>

namespace MediaNet {

class ShortName
{
public:
  ShortName();
  ShortName(uint64_t resourceID);
  ShortName(uint64_t resourceID, uint32_t senderID);
  ShortName(uint64_t resourceID, uint32_t senderID, uint8_t sourceID);

  uint64_t resourceID;
  uint32_t senderID;
  uint8_t sourceID;
  uint32_t mediaTime;
  uint8_t fragmentID;

  // experimental api
  static ShortName fromString(const std::string& name_str);
};

std::ostream&
operator<<(std::ostream& stream, const ShortName& name);
bool
operator==(const ShortName&, const ShortName&);

} // namespace MediaNet
