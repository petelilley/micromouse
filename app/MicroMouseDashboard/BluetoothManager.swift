import CoreBluetooth

class BluetoothManager: NSObject, ObservableObject,
                        CBCentralManagerDelegate, CBPeripheralDelegate {
  
  @Published var isBluetoothEnabled: Bool = false
  
  private var centralManager: CBCentralManager!
  private var microMouse: CBPeripheral?
  
  //
  // Connection state
  //
  
  struct ConnectionState {
    var deviceFound = false
    var deviceConnected = false
    
    //
    // Music service
    //
    
    struct MusicService {
      var serviceFound = false
      var playSongChar: CBCharacteristic?
      var isPlayingChar: CBCharacteristic?
      
      var isReady: Bool {
        get {
          return serviceFound && playSongChar != nil && isPlayingChar != nil
        }
      }
    }
    var musicService = MusicService()
    
    //
    // Vision service
    //
    
    struct VisionService {
      var serviceFound = false
      var dataChar: CBCharacteristic?
      
      var isReady: Bool {
        get {
          return serviceFound && dataChar != nil
        }
      }
    }
    
    var visionService = VisionService()
    
    //
    // Main service
    //
    
    struct MainService {
      var serviceFound = false
      var setTaskChar: CBCharacteristic?
      var currentTaskChar: CBCharacteristic?
      var appReadyChar: CBCharacteristic?
      
      var isReady: Bool {
        get {
          return serviceFound && setTaskChar != nil && currentTaskChar != nil && appReadyChar != nil
        }
      }
    }
    
    var mainService = MainService()
    
    //
    // Drive service
    //
    
    struct DriveService {
      var serviceFound = false
      var dataChar: CBCharacteristic?
      
      var isReady: Bool {
        get {
          return serviceFound && dataChar != nil
        }
      }
    }
    
    var driveService = DriveService()
    
    var isReady : Bool {
      get {
        return deviceFound && deviceConnected &&
               musicService.isReady && visionService.isReady &&
               mainService.isReady && driveService.isReady
      }
    }
  }
  
  @Published var connectionState = ConnectionState()
  
  @Published var rssi = 0
  
  //
  // Music serivce
  //
  
  struct MusicService {
    var isPlaying = false
  }
  
  @Published var musicService = MusicService()
  
  //
  // Vision service
  //
  
  struct VisionService {
    var sensorData = Data([0, 0, 0, 0])
    
    var farRightReading: UInt8 {
      get { return sensorData[0] }
    }
    
    var midRightReading: UInt8 {
      get { return sensorData[1] }
    }
    
    var midLeftReading: UInt8 {
      get { return sensorData[2] }
    }
    
    var farLeftReading: UInt8 {
      get { return sensorData[3] }
    }
  }
  
  @Published var visionService = VisionService()
  
  //
  // Main service
  //
  
  struct MainService {
    var currentTask: UInt8 = 0
  }
  
  @Published var mainService = MainService()
  
  //
  // Drive service
  //
  
  struct DriveService {
    var motorData = Data([0, 0, 0, 0,
                          0, 0, 0, 0,
                          0, 0, 0, 0,
                          0, 0, 0, 0])
    
    private func getValue(at index: Int) -> Float32 {
      let data = Data(motorData[index..<index+4])
      let value = data.withUnsafeBytes { $0.load(as: Float32.self) }
      return value
    }
    
    var motorLeftPosition: Float32 {
      return getValue(at: 0)
    }
    
    var motorRightPosition: Float32 {
      return getValue(at: 4)
    }
    
    var motorLeftVelocity: Float32 {
      return getValue(at: 8)
    }
    
    var motorRightVelocity: Float32 {
      return getValue(at: 12)
    }
  }
  
  @Published var driveService = DriveService()
  
  //
  // Basic stuff
  //
  
  override init() {
    super.init()
    self.centralManager = CBCentralManager(delegate: self, queue: nil)
  }
  
  private func resetState() {
    connectionState = ConnectionState()
  }
  
  //
  // CBCentralManagerDelegate stuff
  //
  
  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    
    if central.state == .poweredOn {
      isBluetoothEnabled = true
      centralManager.scanForPeripherals(withServices: nil)
    }
    else {
      isBluetoothEnabled = false
    }
  }
  
  func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                      advertisementData: [String: Any], rssi RSSI: NSNumber) {
    
    if peripheral.name == AppConstants.Bluetooth.MicroMouseName {
      print("MicroMouse Found!");
      microMouse = peripheral
      microMouse!.delegate = self
      centralManager.stopScan()
      centralManager.connect(peripheral)
      
      connectionState.deviceFound = true
    }
  }
  
  func centralManager(_ central: CBCentralManager,
                      didConnect peripheral: CBPeripheral) {
    
    print("MicroMouse Connected!")
    microMouse!.discoverServices(nil)
    
    connectionState.deviceConnected = true
  }
  
  func centralManager(_ central: CBCentralManager,
                      didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
    
    print("MicroMouse Disconnected!")
    resetState()
    microMouse = nil
    centralManager.scanForPeripherals(withServices: nil)
  }
  
  
  //
  // CBPeripheralDelegate stuff
  //
  
  func peripheral(_ peripheral: CBPeripheral,
                  didDiscoverServices error: Error?) {

    if let services = peripheral.services {
      for service in services {
        peripheral.discoverCharacteristics(nil, for: service)

        switch service.uuid {
        case AppConstants.Bluetooth.MusicService.ServiceUUID:
          connectionState.musicService.serviceFound = true
        case AppConstants.Bluetooth.VisionService.ServiceUUID:
          connectionState.visionService.serviceFound = true
        case AppConstants.Bluetooth.MainService.ServiceUUID:
          connectionState.mainService.serviceFound = true
        case AppConstants.Bluetooth.DriveService.ServiceUUID:
          connectionState.driveService.serviceFound = true
        default:
          print("Unknown Service Discovered: \(service.uuid.uuidString)")
        }
      }
    }
    
  }

  func peripheral(_ peripheral: CBPeripheral,
                  didDiscoverCharacteristicsFor service: CBService, error: Error?) {
    
    if let characteristics = service.characteristics {
      for ch in characteristics {
        
        func setNotify() {
          microMouse!.setNotifyValue(true, for: ch)
        }
        
        switch ch.uuid {
        // Music service
        case AppConstants.Bluetooth.MusicService.PlaySongUUID: // Write
          connectionState.musicService.playSongChar = ch
        case AppConstants.Bluetooth.MusicService.IsPlayingUUID: // Notify
          connectionState.musicService.isPlayingChar = ch
          setNotify()
          
        // Vision service
        case AppConstants.Bluetooth.VisionService.DataUUID: // Notify
          connectionState.visionService.dataChar = ch
          setNotify()
          
        // Main service
        case AppConstants.Bluetooth.MainService.SetTaskUUID: // Write
          connectionState.mainService.setTaskChar = ch
        case AppConstants.Bluetooth.MainService.CurrentTaskUUID: // Notify
          connectionState.mainService.currentTaskChar = ch
          setNotify()
        case AppConstants.Bluetooth.MainService.AppReadyUUID: // Write
          connectionState.mainService.appReadyChar = ch
          
        // Drive service
        case AppConstants.Bluetooth.DriveService.DataUUID: // Notify
          connectionState.driveService.dataChar = ch
          setNotify()
          
        default:
          print("Unknown Characteristic Discovered: \(ch.uuid)")
        }
      }
    }
       
  }
  
  func peripheral(_ peripheral: CBPeripheral,
                  didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
    
    switch characteristic.uuid {
    // Music service
    case AppConstants.Bluetooth.MusicService.IsPlayingUUID:
      musicService.isPlaying = characteristic.value![0] == 1
      
    // Vision service
    case AppConstants.Bluetooth.VisionService.DataUUID:
      visionService.sensorData = characteristic.value![0..<4]
      
    // Main service
    case AppConstants.Bluetooth.MainService.CurrentTaskUUID:
      mainService.currentTask = characteristic.value![0]
      
    // Drive service
    case AppConstants.Bluetooth.DriveService.DataUUID:
      driveService.motorData = characteristic.value![0..<16]
      
    default:
      print("Unknown Characteristic Update: \(characteristic.uuid)")
    }
  }
  
  func peripheral(_ peripheral: CBPeripheral, didReadRSSI RSSI: NSNumber, error: Error?) {
    rssi = RSSI.intValue
  }
  
  func readRSSI() {
    microMouse?.readRSSI()
  }
  
  func writeValueToChar(_ characteristic: CBCharacteristic, _ value: Data) {
    microMouse?.writeValue(value, for: characteristic, type: .withResponse)
  }
}
