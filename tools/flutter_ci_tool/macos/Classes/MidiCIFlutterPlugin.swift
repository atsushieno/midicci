import Cocoa
import FlutterMacOS

public class MidiCIFlutterPlugin: NSObject, FlutterPlugin {
    private var repositoryChannel: FlutterMethodChannel?
    private var deviceChannel: FlutterMethodChannel?
    private var bridgeChannel: FlutterMethodChannel?
    
    private var repository: OpaquePointer?
    private var deviceManager: OpaquePointer?
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let instance = MidiCIFlutterPlugin()
        
        let repositoryChannel = FlutterMethodChannel(name: "midi_ci_tool/repository", binaryMessenger: registrar.messenger)
        let deviceChannel = FlutterMethodChannel(name: "midi_ci_tool/device", binaryMessenger: registrar.messenger)
        let bridgeChannel = FlutterMethodChannel(name: "midi_ci_tool/bridge", binaryMessenger: registrar.messenger)
        
        instance.repositoryChannel = repositoryChannel
        instance.deviceChannel = deviceChannel
        instance.bridgeChannel = bridgeChannel
        
        registrar.addMethodCallDelegate(instance, channel: repositoryChannel)
        registrar.addMethodCallDelegate(instance, channel: deviceChannel)
        registrar.addMethodCallDelegate(instance, channel: bridgeChannel)
    }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "initialize":
            handleInitialize(result: result)
        case "shutdown":
            handleShutdown(result: result)
        case "loadConfig":
            handleLoadConfig(call: call, result: result)
        case "saveConfig":
            handleSaveConfig(call: call, result: result)
        case "loadConfigDefault":
            handleLoadConfigDefault(result: result)
        case "saveConfigDefault":
            handleSaveConfigDefault(result: result)
        case "sendDiscovery":
            handleSendDiscovery(result: result)
        case "getConnections":
            handleGetConnections(result: result)
        case "getInputDevices":
            handleGetInputDevices(result: result)
        case "getOutputDevices":
            handleGetOutputDevices(result: result)
        case "setInputDevice":
            handleSetInputDevice(call: call, result: result)
        case "setOutputDevice":
            handleSetOutputDevice(call: call, result: result)
        case "addLocalProfile":
            handleAddLocalProfile(call: call, result: result)
        case "removeLocalProfile":
            handleRemoveLocalProfile(call: call, result: result)
        case "addLocalProperty":
            handleAddLocalProperty(call: call, result: result)
        case "removeLocalProperty":
            handleRemoveLocalProperty(call: call, result: result)
        case "updatePropertyValue":
            handleUpdatePropertyValue(call: call, result: result)
        default:
            result(FlutterMethodNotImplemented)
        }
    }
    
    private func handleInitialize(result: @escaping FlutterResult) {
        if repository == nil {
            repository = ci_tool_repository_create()
            if repository != nil {
                ci_tool_repository_initialize(repository)
                deviceManager = ci_tool_device_manager_create(repository)
                if deviceManager != nil {
                    ci_tool_device_manager_initialize(deviceManager)
                    result(true)
                } else {
                    result(false)
                }
            } else {
                result(false)
            }
        } else {
            result(true)
        }
    }
    
    private func handleShutdown(result: @escaping FlutterResult) {
        if let deviceManager = deviceManager {
            ci_tool_device_manager_destroy(deviceManager)
            self.deviceManager = nil
        }
        if let repository = repository {
            ci_tool_repository_destroy(repository)
            self.repository = nil
        }
        result(nil)
    }
    
    private func handleLoadConfig(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let repository = repository,
              let args = call.arguments as? [String: Any],
              let path = args["path"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_repository_load_config(repository, path)
        result(success)
    }
    
    private func handleSaveConfig(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let repository = repository,
              let args = call.arguments as? [String: Any],
              let path = args["path"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_repository_save_config(repository, path)
        result(success)
    }
    
    private func handleLoadConfigDefault(result: @escaping FlutterResult) {
        guard let repository = repository else {
            result(nil)
            return
        }
        
        ci_tool_repository_load_config_default(repository)
        result(nil)
    }
    
    private func handleSaveConfigDefault(result: @escaping FlutterResult) {
        guard let repository = repository else {
            result(nil)
            return
        }
        
        ci_tool_repository_save_config_default(repository)
        result(nil)
    }
    
    private func handleSendDiscovery(result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager else {
            result(nil)
            return
        }
        
        ci_tool_device_manager_send_discovery(deviceManager)
        result(nil)
    }
    
    private func handleGetConnections(result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager else {
            result("")
            return
        }
        
        if let connectionsJson = ci_tool_device_manager_get_connections_json(deviceManager) {
            let jsonString = String(cString: connectionsJson)
            ci_tool_free_string(connectionsJson)
            result(jsonString)
        } else {
            result("")
        }
    }
    
    private func handleGetInputDevices(result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager else {
            result("")
            return
        }
        
        if let devicesJson = ci_tool_device_manager_get_input_devices_json(deviceManager) {
            let jsonString = String(cString: devicesJson)
            ci_tool_free_string(devicesJson)
            result(jsonString)
        } else {
            result("")
        }
    }
    
    private func handleGetOutputDevices(result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager else {
            result("")
            return
        }
        
        if let devicesJson = ci_tool_device_manager_get_output_devices_json(deviceManager) {
            let jsonString = String(cString: devicesJson)
            ci_tool_free_string(devicesJson)
            result(jsonString)
        } else {
            result("")
        }
    }
    
    private func handleSetInputDevice(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let deviceId = args["deviceId"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_device_manager_set_input_device(deviceManager, deviceId)
        result(success)
    }
    
    private func handleSetOutputDevice(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let deviceId = args["deviceId"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_device_manager_set_output_device(deviceManager, deviceId)
        result(success)
    }
    
    private func handleAddLocalProfile(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let profileData = args["profile"] as? [String: Any] else {
            result(false)
            return
        }
        
        do {
            let jsonData = try JSONSerialization.data(withJSONObject: profileData)
            let jsonString = String(data: jsonData, encoding: .utf8) ?? ""
            let success = ci_tool_device_manager_add_local_profile_json(deviceManager, jsonString)
            result(success)
        } catch {
            result(false)
        }
    }
    
    private func handleRemoveLocalProfile(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let group = args["group"] as? UInt8,
              let address = args["address"] as? UInt8,
              let profileId = args["profileId"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_device_manager_remove_local_profile(deviceManager, group, address, profileId)
        result(success)
    }
    
    private func handleAddLocalProperty(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let propertyData = args["property"] as? [String: Any] else {
            result(false)
            return
        }
        
        do {
            let jsonData = try JSONSerialization.data(withJSONObject: propertyData)
            let jsonString = String(data: jsonData, encoding: .utf8) ?? ""
            let success = ci_tool_device_manager_add_local_property_json(deviceManager, jsonString)
            result(success)
        } catch {
            result(false)
        }
    }
    
    private func handleRemoveLocalProperty(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let propertyId = args["propertyId"] as? String else {
            result(false)
            return
        }
        
        let success = ci_tool_device_manager_remove_local_property(deviceManager, propertyId)
        result(success)
    }
    
    private func handleUpdatePropertyValue(call: FlutterMethodCall, result: @escaping FlutterResult) {
        guard let deviceManager = deviceManager,
              let args = call.arguments as? [String: Any],
              let propertyId = args["propertyId"] as? String,
              let resourceId = args["resourceId"] as? String,
              let data = args["data"] as? [Int] else {
            result(false)
            return
        }
        
        let uint8Data = data.map { UInt8($0) }
        let success = ci_tool_device_manager_update_property_value(deviceManager, propertyId, resourceId, uint8Data, uint8Data.count)
        result(success)
    }
}
