// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_NETWORK_IP_CONFIG_H_
#define CHROME_BROWSER_CHROMEOS_CROS_NETWORK_IP_CONFIG_H_

#include <string>
#include <vector>

#include "third_party/cros/chromeos_network.h"

namespace chromeos {

 // IP Configuration.
struct NetworkIPConfig {
  NetworkIPConfig(const std::string& device_path, IPConfigType type,
                  const std::string& address, const std::string& netmask,
                  const std::string& gateway, const std::string& name_servers);
  ~NetworkIPConfig();

  // Gets the PrefixLength for an IPv4 netmask.
  // For example, "255.255.255.0" => 24
  // If the netmask is invalid, this will return -1;
  // TODO(chocobo): Add support for IPv6.
  int32 GetPrefixLength() const;
  std::string device_path;  // This looks like "/device/0011aa22bb33"
  IPConfigType type;
  std::string address;
  std::string netmask;
  std::string gateway;
  std::string name_servers;
};

typedef std::vector<NetworkIPConfig> NetworkIPConfigVector;

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_NETWORK_IP_CONFIG_H_
