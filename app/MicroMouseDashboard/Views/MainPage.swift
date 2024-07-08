import SwiftUI

struct MainPage: View {
  @EnvironmentObject var btManager: BluetoothManager
  
  enum Task: UInt8, CaseIterable, Identifiable {
    case mazeSearch = 1
    case mazeFastSolve = 2
    case driveStraight = 3
    
    var id: Self { self }
  }
  
  private let taskNames: [Task: String] = [
    .mazeSearch: "Maze Search",
    .mazeFastSolve: "Maze Fast Solve",
    .driveStraight: "Drive Straight (10cm)",
  ]
  private let taskDescriptions: [Task: String] = [
    .mazeSearch: "Search to the center of the maze, then back to the start",
    .mazeFastSolve: "Solve the maze as fast as possible while using previous search data",
    .driveStraight: "Drive straight for 10cm at 500mm/s",
  ]
  
  @State private var selectedTask = Task.mazeSearch

  var body: some View {
    NavigationStack {
      List {
        Section("Current task") {
          let currentTask = btManager.mainService.currentTask
          if (currentTask == 0) {
            Text("None")
          }
          else {
            Text(taskNames[Task(rawValue: currentTask)!]!)
          }
        }
        
        Section("Error message") {
          Text("None")
        }
        
        Section(header: Text("User task Selection"), footer: Text(taskDescriptions[selectedTask]!)) {
          Picker("Task", selection: $selectedTask) {
            ForEach(Task.allCases, id: \.self) { task in
              Text(taskNames[task]!)
            }
          }
        }
        Section {
          Button("Run Task") {
            let setTaskChar = btManager.connectionState.mainService.setTaskChar!
            let setTaskData = Data([selectedTask.rawValue])
            btManager.writeValueToChar(setTaskChar, setTaskData)
          }
        }
        Section {
          Button("Stop") {
            let setTaskChar = btManager.connectionState.mainService.setTaskChar!
            let setTaskData = Data([0])
            btManager.writeValueToChar(setTaskChar, setTaskData)
          }
        }
      }
      .navigationTitle("MicroMouse")
    }
  }
}

#Preview {
  MainPage()
    .environmentObject(BluetoothManager())
}
