/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/mdio/MdioError.h"
#include "folly/json.h"

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace {
template <typename T>
folly::dynamic thriftToDynamic(const T& val) {
  return folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(val));
}

template <typename T>
folly::dynamic thriftOptToDynamic(const std::optional<T>& opt) {
  return opt.has_value() ? thriftToDynamic(opt.value()) : "null";
}
} // namespace

namespace facebook::fboss::phy {

bool LaneConfig::operator==(const LaneConfig& rhs) const {
  return (polaritySwap == rhs.polaritySwap) && (tx == rhs.tx);
}

LaneSettings LaneConfig::toLaneSettings() const {
  LaneSettings settings;
  if (!polaritySwap.has_value()) {
    settings.polaritySwap = {};
  } else {
    settings.polaritySwap = polaritySwap.value();
  }
  if (tx.has_value()) {
    settings.tx_ref() = tx.value();
  }
  return settings;
}

LaneConfig LaneConfig::fromLaneSettings(const LaneSettings& settings) {
  LaneConfig config;
  if (auto tx = settings.tx_ref()) {
    config.tx = *tx;
  }
  config.polaritySwap = settings.polaritySwap;
  return config;
}

bool PhySideConfig::operator==(const PhySideConfig& rhs) const {
  return std::equal(
      lanes.begin(), lanes.end(), rhs.lanes.begin(), rhs.lanes.end());
}

PhySideConfig PhySideConfig::fromPhyPortSideSettings(
    const PhyPortSideSettings& settings) {
  PhySideConfig phySideConfig;

  for (auto in : settings.lanes) {
    phySideConfig.lanes.insert(std::pair<int32_t, LaneConfig>(
        in.first, LaneConfig::fromLaneSettings(in.second)));
  }
  return phySideConfig;
}

bool ExternalPhyConfig::operator==(const ExternalPhyConfig& rhs) const {
  return (system == rhs.system) && (line == rhs.line);
}

bool ExternalPhyProfileConfig::operator==(
    const ExternalPhyProfileConfig& rhs) const {
  return (speed == rhs.speed) && (system == rhs.system) && (line == rhs.line);
}

ExternalPhyConfig ExternalPhyConfig::fromConfigeratorTypes(
    PortPinConfig portPinConfig,
    const std::map<int32_t, PolaritySwap>& linePolaritySwapMap) {
  ExternalPhyConfig xphyCfg;

  if (!portPinConfig.xphySys_ref()) {
    throw MdioError("Port pin config is missing xphySys");
  }
  if (!portPinConfig.xphyLine_ref()) {
    throw MdioError("Port pin config is missing xphyLine");
  }

  auto fillLaneConfigs =
      [](const std::vector<PinConfig>& pinConfigs,
         const std::map<int32_t, PolaritySwap>& polaritySwapMap,
         std::map<int32_t, LaneConfig>& laneConfigs) {
        std::map<int32_t, TxSettings> txMap;
        for (auto pinCfg : pinConfigs) {
          LaneConfig laneCfg;
          if (pinCfg.tx_ref()) {
            laneCfg.tx = *pinCfg.tx_ref();
          }
          if (auto it = polaritySwapMap.find(pinCfg.id.lane);
              it != polaritySwapMap.end()) {
            laneCfg.polaritySwap = it->second;
          }
          laneConfigs.emplace(pinCfg.id.lane, laneCfg);
        }
      };

  fillLaneConfigs(*portPinConfig.xphySys_ref(), {}, xphyCfg.system.lanes);
  fillLaneConfigs(
      *portPinConfig.xphyLine_ref(), linePolaritySwapMap, xphyCfg.line.lanes);

  return xphyCfg;
}

bool PhyPortConfig::operator==(const PhyPortConfig& rhs) const {
  return config == rhs.config && profile == rhs.profile;
}

bool PhyPortConfig::operator!=(const PhyPortConfig& rhs) const {
  return !(*this == rhs);
}

ExternalPhyProfileConfig ExternalPhyProfileConfig::fromPortProfileConfig(
    const PortProfileConfig& portCfg) {
  if (!portCfg.xphyLine_ref()) {
    throw MdioError(
        "Attempted to create xphy config without xphy line settings");
  }
  ExternalPhyProfileConfig xphyCfg;
  xphyCfg.speed = portCfg.speed;
  xphyCfg.system = portCfg.iphy;
  xphyCfg.line = *portCfg.xphyLine_ref();
  return xphyCfg;
}

PhyPortSettings PhyPortConfig::toPhyPortSettings(int16_t phyID) const {
  PhyPortSettings settings;

  for (auto in : config.system.lanes) {
    settings.system.lanes.insert(
        std::pair<int16_t, LaneSettings>(in.first, in.second.toLaneSettings()));
  }
  for (auto in : config.line.lanes) {
    settings.line.lanes.insert(
        std::pair<int16_t, LaneSettings>(in.first, in.second.toLaneSettings()));
  }

  settings.phyID = phyID;
  settings.speed = profile.speed;
  settings.line.modulation = profile.line.modulation;
  settings.system.modulation = profile.system.modulation;
  settings.line.fec = profile.line.fec;
  settings.system.fec = profile.system.fec;

  return settings;
}

PhyPortConfig PhyPortConfig::fromPhyPortSettings(
    const PhyPortSettings& settings) {
  PhyPortConfig result;

  for (auto in : settings.system.lanes) {
    result.config.system.lanes.insert(std::pair<int32_t, LaneConfig>(
        in.first, LaneConfig::fromLaneSettings(in.second)));
  }

  for (auto in : settings.line.lanes) {
    result.config.line.lanes.insert(std::pair<int32_t, LaneConfig>(
        in.first, LaneConfig::fromLaneSettings(in.second)));
  }

  result.config.system =
      PhySideConfig::fromPhyPortSideSettings(settings.system);
  result.config.line = PhySideConfig::fromPhyPortSideSettings(settings.line);

  result.profile.line.numLanes = settings.line.lanes.size();
  result.profile.line.modulation = settings.line.modulation;
  result.profile.line.fec = settings.line.fec;

  result.profile.system.numLanes = settings.system.lanes.size();
  result.profile.system.modulation = settings.system.modulation;
  result.profile.system.fec = settings.system.fec;

  result.profile.speed = settings.speed;

  return result;
}

folly::dynamic LaneConfig::toDynamic() {
  folly::dynamic obj = folly::dynamic::object;
  obj["polaritySwap"] = thriftOptToDynamic(polaritySwap);
  obj["tx"] = thriftOptToDynamic(tx);

  return obj;
}

folly::dynamic PhySideConfig::toDynamic() {
  folly::dynamic elements = folly::dynamic::array;
  for (auto pair : lanes) {
    elements.push_back(folly::dynamic::object(
        std::to_string(pair.first), pair.second.toDynamic()));
  }

  return elements;
}

folly::dynamic ExternalPhyConfig::toDynamic() {
  folly::dynamic obj = folly::dynamic::object;
  obj["system"] = system.toDynamic();
  obj["line"] = line.toDynamic();

  return obj;
}

folly::dynamic ExternalPhyProfileConfig::toDynamic() {
  folly::dynamic obj = folly::dynamic::object;
  obj["speed"] = apache::thrift::util::enumNameSafe(speed);
  obj["system"] = thriftToDynamic(system);
  obj["line"] = thriftToDynamic(line);

  return obj;
}

folly::dynamic PhyPortConfig::toDynamic() {
  folly::dynamic obj = folly::dynamic::object;
  obj["config"] = config.toDynamic();
  obj["profile"] = profile.toDynamic();
  return obj;
}

} // namespace facebook::fboss::phy
