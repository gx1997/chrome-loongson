// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_NETWORK_LIBRARY_IMPL_CROS_H_
#define CHROME_BROWSER_CHROMEOS_CROS_NETWORK_LIBRARY_IMPL_CROS_H_

#include "chrome/browser/chromeos/cros/network_library_impl_base.h"

namespace chromeos {

class NetworkLibraryImplCros : public NetworkLibraryImplBase  {
 public:
  NetworkLibraryImplCros();
  virtual ~NetworkLibraryImplCros();

  virtual void Init() OVERRIDE;
  virtual bool IsCros() const OVERRIDE;

  //////////////////////////////////////////////////////////////////////////////
  // NetworkLibraryImplBase implementation.

  virtual void MonitorNetworkStart(const std::string& service_path) OVERRIDE;
  virtual void MonitorNetworkStop(const std::string& service_path) OVERRIDE;
  virtual void MonitorNetworkDeviceStart(
      const std::string& device_path) OVERRIDE;
  virtual void MonitorNetworkDeviceStop(
      const std::string& device_path) OVERRIDE;

  virtual void CallConfigureService(const std::string& identifier,
                                    const DictionaryValue* info) OVERRIDE;
  virtual void CallConnectToNetwork(Network* network) OVERRIDE;
  virtual void CallRequestWifiNetworkAndConnect(
      const std::string& ssid, ConnectionSecurity security) OVERRIDE;
  virtual void CallRequestVirtualNetworkAndConnect(
      const std::string& service_name,
      const std::string& server_hostname,
      ProviderType provider_type) OVERRIDE;
  virtual void CallDeleteRememberedNetwork(
      const std::string& profile_path,
      const std::string& service_path) OVERRIDE;

  //////////////////////////////////////////////////////////////////////////////
  // NetworkLibrary implementation.

  virtual void SetCheckPortalList(
      const std::string& check_portal_list) OVERRIDE;
  virtual void SetDefaultCheckPortalList() OVERRIDE;
  virtual void ChangePin(const std::string& old_pin,
                         const std::string& new_pin) OVERRIDE;
  virtual void ChangeRequirePin(bool require_pin,
                                const std::string& pin) OVERRIDE;
  virtual void EnterPin(const std::string& pin) OVERRIDE;
  virtual void UnblockPin(const std::string& puk,
                          const std::string& new_pin) OVERRIDE;
  virtual void RequestCellularScan() OVERRIDE;
  virtual void RequestCellularRegister(const std::string& network_id) OVERRIDE;
  virtual void SetCellularDataRoamingAllowed(bool new_value) OVERRIDE;
  virtual bool IsCellularAlwaysInRoaming() OVERRIDE;
  virtual void RequestNetworkScan() OVERRIDE;
  virtual bool GetWifiAccessPoints(WifiAccessPointVector* result) OVERRIDE;

  virtual void DisconnectFromNetwork(const Network* network) OVERRIDE;
  virtual void CallEnableNetworkDeviceType(
      ConnectionType device, bool enable) OVERRIDE;
  virtual void CallRemoveNetwork(const Network* network) OVERRIDE;

  virtual void EnableOfflineMode(bool enable) OVERRIDE;

  virtual NetworkIPConfigVector GetIPConfigs(
      const std::string& device_path,
      std::string* hardware_address,
      HardwareAddressFormat format) OVERRIDE;
  virtual void SetIPConfig(const NetworkIPConfig& ipconfig) OVERRIDE;

  //////////////////////////////////////////////////////////////////////////////
  // Calbacks.
  static void NetworkStatusChangedHandler(void* object,
                                          const std::string& path,
                                          const std::string& key,
                                          const base::Value& value);
  void UpdateNetworkStatus(
      const std::string& path, const std::string& key, const Value& value);

  static void NetworkDevicePropertyChangedHandler(void* object,
                                                  const std::string& path,
                                                  const std::string& key,
                                                  const base::Value& value);
  void UpdateNetworkDeviceStatus(
      const std::string& path, const std::string& key, const Value& value);
  // Cellular specific updates. Returns false if update was ignored / reverted
  // and notification should be skipped.
  bool UpdateCellularDeviceStatus(NetworkDevice* device, PropertyIndex index);

  static void PinOperationCallback(void* object,
                                   const char* path,
                                   NetworkMethodErrorType error,
                                   const char* error_message);

  static void CellularRegisterCallback(void* object,
                                       const char* path,
                                       NetworkMethodErrorType error,
                                       const char* error_message);

  static void NetworkConnectCallback(void* object,
                                     const char* service_path,
                                     NetworkMethodErrorType error,
                                     const char* error_message);

  static void WifiServiceUpdateAndConnect(
      void* object,
      const std::string& service_path,
      const base::DictionaryValue* properties);
  static void VPNServiceUpdateAndConnect(
      void* object,
      const std::string& service_path,
      const base::DictionaryValue* properties);

  static void NetworkManagerStatusChangedHandler(void* object,
                                                 const std::string& path,
                                                 const std::string& key,
                                                 const base::Value& value);
  static void NetworkManagerUpdate(void* object,
                                   const std::string& manager_path,
                                   const base::DictionaryValue* properties);

  static void DataPlanUpdateHandler(
      void* object,
      const std::string& modem_service_path,
      CellularDataPlanVector* data_plan_vector);

  static void NetworkServiceUpdate(void* object,
                                   const std::string& service_path,
                                   const base::DictionaryValue* properties);
  static void RememberedNetworkServiceUpdate(
      void* object,
      const std::string& service_path,
      const base::DictionaryValue* properties);
  static void ProfileUpdate(void* object,
                            const std::string& profile_path,
                            const base::DictionaryValue* properties);
  static void NetworkDeviceUpdate(void* object,
                                  const std::string& device_path,
                                  const base::DictionaryValue* properties);

 private:
  // This processes all Manager update messages.
  void NetworkManagerStatusChanged(const std::string& key, const Value* value);
  void ParseNetworkManager(const DictionaryValue& dict);
  void UpdateTechnologies(const ListValue* technologies, int* bitfieldp);
  void UpdateAvailableTechnologies(const ListValue* technologies);
  void UpdateEnabledTechnologies(const ListValue* technologies);
  void UpdateConnectedTechnologies(const ListValue* technologies);

  // Update network lists.
  void UpdateNetworkServiceList(const ListValue* services);
  void UpdateWatchedNetworkServiceList(const ListValue* services);
  Network* ParseNetwork(const std::string& service_path,
                        const DictionaryValue& info);

  void UpdateRememberedNetworks(const ListValue* profiles);
  void RequestRememberedNetworksUpdate();
  void UpdateRememberedServiceList(const std::string& profile_path,
                                   const ListValue* profile_entries);
  Network* ParseRememberedNetwork(const std::string& service_path,
                                  const DictionaryValue& info);

  // NetworkDevice list management functions.
  void UpdateNetworkDeviceList(const ListValue* devices);
  void ParseNetworkDevice(const std::string& device_path,
                          const DictionaryValue& info);

  // Empty device observer to ensure that device property updates are received.
  class NetworkLibraryDeviceObserver : public NetworkDeviceObserver {
   public:
    virtual ~NetworkLibraryDeviceObserver() {}
    virtual void OnNetworkDeviceChanged(
        NetworkLibrary* cros, const NetworkDevice* device) OVERRIDE {}
  };

  typedef std::map<std::string, CrosNetworkWatcher*> NetworkWatcherMap;

  // For monitoring network manager status changes.
  scoped_ptr<CrosNetworkWatcher> network_manager_watcher_;

  // For monitoring data plan changes to the connected cellular network.
  scoped_ptr<CrosNetworkWatcher> data_plan_watcher_;

  // Network device observer.
  scoped_ptr<NetworkLibraryDeviceObserver> network_device_observer_;

  // Map of monitored networks.
  NetworkWatcherMap monitored_networks_;

  // Map of monitored devices.
  NetworkWatcherMap monitored_devices_;

  DISALLOW_COPY_AND_ASSIGN(NetworkLibraryImplCros);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_NETWORK_LIBRARY_IMPL_CROS_H_
