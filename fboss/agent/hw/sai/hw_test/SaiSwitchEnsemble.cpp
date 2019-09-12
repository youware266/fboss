/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"

#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

#include <memory>

namespace facebook {
namespace fboss {

SaiSwitchEnsemble::SaiSwitchEnsemble(uint32_t featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {
  // TODO pass in agent config
  auto platform = initSaiPlatform();
  // TODO get SaiSwitch to honor features desired and pass them here
  auto hwSwitch =
      std::make_unique<SaiSwitch>(static_cast<SaiPlatform*>(platform.get()));
  setupEnsemble(
      std::move(platform),
      std::move(hwSwitch),
      nullptr, // TODO add SaiLinkStateToggler once we add support for it
      nullptr // Add support for accessing HW shell once we add support for it
  );
}

const std::vector<PortID>& SaiSwitchEnsemble::logicalPortIds() const {
  // TODO
  static std::vector<PortID> logicalPorts;
  return logicalPorts;
}

const std::vector<PortID>& SaiSwitchEnsemble::masterLogicalPortIds() const {
  // TODO
  static std::vector<PortID> logicalPorts;
  return logicalPorts;
}

std::vector<PortID> SaiSwitchEnsemble::getAllPortsinGroup(PortID portID) const {
  // TODO
  return {};
}

std::vector<FlexPortMode> SaiSwitchEnsemble::getSupportedFlexPortModes() const {
  // TODO
  return {};
}

void SaiSwitchEnsemble::dumpHwCounters() const {
  // TODO once hw shell access is supported
}

std::map<PortID, HwPortStats> SaiSwitchEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  // TODO
  return {};
}

void SaiSwitchEnsemble::recreateHwSwitchFromWBState() {
  // Maybe TODO - will implement this if we decide to support in process
  // recosntruction of warm boot state from serialized switch state.
}

void SaiSwitchEnsemble::stopHwCallLogging() const {
  // TODO - if we support cint style h/w call logging
}

} // namespace fboss
} // namespace facebook