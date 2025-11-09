# System Monitor Tool for Windows

A comprehensive system monitoring tool for Windows that provides real-time information about CPU usage, memory usage, and running processes. Built in C++ using Windows API.

## Features

- **Real-time CPU Usage Monitoring**: Displays overall CPU usage percentage
- **Memory Information**: Shows total, used, and available memory with percentage
- **Process Management**: Lists all running processes with details including:
  - Process ID (PID)
  - Process Name
  - Process State (Running, Zombie, System/Protected)
  - CPU Time (in seconds)
  - Memory Usage (in MB)
- **Interactive Interface**: Console-based UI with keyboard controls
- **Process Sorting**: Sort processes by CPU time or memory usage
- **Process Termination**: Ability to kill selected processes (with confirmation)
- **Administrator Detection**: Warns if not running with administrator privileges

## Controls

- `[c]` - Sort processes by CPU Time
- `[m]` - Sort processes by Memory Usage
- `[k]` - Kill selected process (with confirmation dialog)
- `[r]` - Refresh data
- `[Up/Down]` - Navigate through process list
- `[q]` - Quit the application

## Requirements

- Windows operating system
- C++ compiler (e.g., Visual Studio, MinGW)
- Administrator privileges recommended for full process information access

## Building

1. Ensure you have a C++ compiler installed
2. Compile the `system_monitor_windows.cpp` file:

```bash
g++ system_monitor_windows.cpp -o system_monitor.exe -lpsapi
```

Or using Visual Studio:

```bash
cl system_monitor_windows.cpp /link psapi.lib
```

## Usage

1. Run the executable as administrator for full access:

   ```
   system_monitor.exe
   ```

2. The tool will display system information and process list
3. Use keyboard controls to navigate and manage processes
4. Press 'q' to exit

## Notes

- Some system processes may show as "[System]" if not running with administrator privileges
- CPU usage is calculated as overall system usage
- Memory usage shows working set size for each process
- Process states: R=Running, Z=Zombie, S=System/Protected

## License

This project is open source. Feel free to modify and distribute.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
