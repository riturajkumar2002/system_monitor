#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#define _WIN32_IE 0x0700

#include <windows.h>
#include <winver.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <tchar.h>

// Rest of your includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <conio.h>
#include <ctime>

// ...existing code...
using namespace std;

// Structure to hold process information
struct ProcessInfo
{
    DWORD pid;
    string name;
    string state;
    double cpu_usage;
    double memory_usage;
    string user;
};

// Function to get CPU usage
double getCPUUsage()
{
    static ULARGE_INTEGER prev_idle = {0}, prev_kernel = {0}, prev_user = {0};

    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);

    ULARGE_INTEGER curr_idle, curr_kernel, curr_user;
    curr_idle.LowPart = idle.dwLowDateTime;
    curr_idle.HighPart = idle.dwHighDateTime;
    curr_kernel.LowPart = kernel.dwLowDateTime;
    curr_kernel.HighPart = kernel.dwHighDateTime;
    curr_user.LowPart = user.dwLowDateTime;
    curr_user.HighPart = user.dwHighDateTime;

    ULARGE_INTEGER diff_idle, diff_kernel, diff_user;
    diff_idle.QuadPart = curr_idle.QuadPart - prev_idle.QuadPart;
    diff_kernel.QuadPart = curr_kernel.QuadPart - prev_kernel.QuadPart;
    diff_user.QuadPart = curr_user.QuadPart - prev_user.QuadPart;

    ULARGE_INTEGER total;
    total.QuadPart = diff_kernel.QuadPart + diff_user.QuadPart;

    double cpu_percent = 0.0;
    if (total.QuadPart > 0)
    {
        cpu_percent = 100.0 * (1.0 - (double)diff_idle.QuadPart / total.QuadPart);
    }

    prev_idle = curr_idle;
    prev_kernel = curr_kernel;
    prev_user = curr_user;

    return cpu_percent;
}

// Function to get memory information
void getMemoryInfo(double &total_mem, double &used_mem, double &mem_percent)
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    total_mem = memInfo.ullTotalPhys / (1024.0 * 1024.0); // Convert to MB
    used_mem = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0 * 1024.0);
    mem_percent = memInfo.dwMemoryLoad;
}

ProcessInfo getProcessInfo(DWORD pid)
{
    ProcessInfo proc;
    proc.pid = pid;
    proc.state = "R"; // Default to Running
    proc.cpu_usage = 0.0;
    proc.memory_usage = 0.0;
    proc.user = "";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess)
    {
        char processPath[MAX_PATH] = {0};
        DWORD size = sizeof(processPath);

        // Try GetModuleFileNameEx first
        if (GetModuleFileNameExA(hProcess, NULL, processPath, size))
        {
            string fullPath = processPath;
            size_t lastSlash = fullPath.find_last_of("\\/");
            if (lastSlash != string::npos)
            {
                proc.name = fullPath.substr(lastSlash + 1);
            }
            else
            {
                proc.name = fullPath;
            }
        }
        else
        {
            // Fallback to GetModuleBaseName
            char processName[MAX_PATH] = {0};
            if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName)))
            {
                proc.name = processName;
            }
            else
            {
                proc.name = "[System]";
            }
        }

        // Get memory usage
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        {
            proc.memory_usage = pmc.WorkingSetSize / (1024.0 * 1024.0);
        }

        // Get CPU usage (total time)
        FILETIME ftCreation, ftExit, ftKernel, ftUser;
        if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser))
        {
            ULARGE_INTEGER kernel, user;
            kernel.LowPart = ftKernel.dwLowDateTime;
            kernel.HighPart = ftKernel.dwHighDateTime;
            user.LowPart = ftUser.dwLowDateTime;
            user.HighPart = ftUser.dwHighDateTime;
            proc.cpu_usage = (kernel.QuadPart + user.QuadPart) / 10000000.0;
        }

        // Check exit code for process state
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode))
        {
            if (exitCode != STILL_ACTIVE)
            {
                proc.state = "Z"; // Zombie (exited)
            }
        }

        CloseHandle(hProcess);
    }
    else
    {
        proc.state = "S"; // System/Protected
    }

    return proc;
}
// Function to get all processes
vector<ProcessInfo> getAllProcesses()
{
    vector<ProcessInfo> processes;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return processes;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);
        return processes;
    }

    do
    {
        processes.push_back(getProcessInfo(pe32.th32ProcessID));
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return processes;
}

// Sort by CPU usage
bool sortByCPU(const ProcessInfo &a, const ProcessInfo &b)
{
    return a.cpu_usage > b.cpu_usage;
}

// Sort by Memory usage
bool sortByMemory(const ProcessInfo &a, const ProcessInfo &b)
{
    return a.memory_usage > b.memory_usage;
}

// Function to kill a process
bool killProcess(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess)
    {
        bool result = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return result;
    }
    return false;
}

// Display UI using console
void displayUI(vector<ProcessInfo> &processes, int sort_mode, int selected_row)
{
    system("cls");

    // Display header with color
    cout << "========================================" << endl;
    cout << "     SYSTEM MONITOR TOOL - Windows     " << endl;
    cout << "========================================" << endl;

    // Get and display system information
    double cpu_usage = getCPUUsage();
    double total_mem, used_mem, mem_percent;
    getMemoryInfo(total_mem, used_mem, mem_percent);

    time_t now = time(0);
    string date_time = ctime(&now);
    date_time.pop_back(); // Remove newline

    cout << "\nTime: " << date_time << endl;
    cout << "CPU Usage: " << fixed << setprecision(2) << cpu_usage << "%" << endl;
    cout << "Memory: " << fixed << setprecision(2) << used_mem << " MB / " << total_mem << " MB (" << mem_percent << "%)" << endl;

    // Display process count
    cout << "Total Processes: " << processes.size() << endl;

    // Display sorting mode
    string sort_text = sort_mode == 1 ? "CPU Time" : "Memory Usage";
    cout << "Sorted by: " << sort_text << endl;

    // Display controls
    cout << "\n[CONTROLS]" << endl;
    cout << "  [c] Sort by CPU    [m] Sort by Memory" << endl;
    cout << "  [k] Kill Process   [r] Refresh" << endl;
    cout << "  [Up/Down] Navigate [q] Quit" << endl;
    cout << "\n"
         << string(75, '=') << endl;

    // Display process table header
    cout << left
         << setw(8) << "PID"
         << setw(25) << "NAME"
         << setw(8) << "STATE"
         << setw(15) << "CPU TIME(s)"
         << setw(15) << "MEMORY(MB)"
         << endl;
    cout << string(75, '-') << endl;

    // Display processes (limit to screen size)
    int max_rows = 20;
    int start_row = max(0, selected_row - max_rows + 1);

    for (int i = 0; i < min((int)processes.size(), max_rows); i++)
    {
        int idx = start_row + i;
        if (idx >= processes.size())
            break;

        // Highlight selected row
        if (idx == selected_row)
        {
            cout << ">> ";
        }
        else
        {
            cout << "   ";
        }

        cout << left
             << setw(8) << processes[idx].pid
             << setw(25) << processes[idx].name.substr(0, 24)
             << setw(8) << processes[idx].state
             << setw(15) << fixed << setprecision(2) << processes[idx].cpu_usage
             << setw(15) << processes[idx].memory_usage
             << endl;
    }

    cout << string(75, '=') << endl;
    cout << "\nNote: [System] = Protected system process" << endl;
}

int main()
{
    int sort_mode = 1; // 1 = CPU, 2 = Memory
    int selected_row = 0;

    // Display startup message
    system("cls");
    cout << "========================================" << endl;
    cout << "     SYSTEM MONITOR TOOL - Windows     " << endl;
    cout << "========================================" << endl;
    cout << "\nStarting System Monitor..." << endl;
    cout << "Loading process information..." << endl;

    // Check if running as admin
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    if (!isAdmin)
    {
        cout << "\nWARNING: Not running as Administrator!" << endl;
        cout << "Some process names may show as [System]" << endl;
        cout << "For full access, run PowerShell as Administrator" << endl;
    }
    else
    {
        cout << "\nRunning with Administrator privileges" << endl;
    }

    cout << "\nPress any key to continue..." << endl;
    _getch();

    while (true)
    {
        // Get all processes
        vector<ProcessInfo> processes = getAllProcesses();

        // Sort processes based on mode
        if (sort_mode == 1)
        {
            sort(processes.begin(), processes.end(), sortByCPU);
        }
        else
        {
            sort(processes.begin(), processes.end(), sortByMemory);
        }

        // Display UI
        displayUI(processes, sort_mode, selected_row);

        // Handle input
        if (_kbhit())
        {
            int ch = _getch();

            if (ch == 'q' || ch == 'Q')
            {
                break;
            }
            else if (ch == 'c' || ch == 'C')
            {
                sort_mode = 1;
                selected_row = 0;
            }
            else if (ch == 'm' || ch == 'M')
            {
                sort_mode = 2;
                selected_row = 0;
            }
            else if (ch == 'r' || ch == 'R')
            {
                // Refresh - do nothing, will refresh automatically
                continue;
            }
            else if (ch == 224)
            { // Arrow key prefix
                ch = _getch();
                if (ch == 72)
                { // Up arrow
                    if (selected_row > 0)
                        selected_row--;
                }
                else if (ch == 80)
                { // Down arrow
                    if (selected_row < (int)processes.size() - 1)
                        selected_row++;
                }
            }
            else if (ch == 'k' || ch == 'K')
            {
                if (selected_row < processes.size())
                {
                    DWORD pid = processes[selected_row].pid;
                    string pname = processes[selected_row].name;

                    system("cls");
                    cout << "\n========================================" << endl;
                    cout << "           KILL PROCESS                " << endl;
                    cout << "========================================" << endl;
                    cout << "\nProcess ID: " << pid << endl;
                    cout << "Process Name: " << pname << endl;
                    cout << "\nAre you sure you want to kill this process?" << endl;
                    cout << "Press [Y] to confirm, any other key to cancel: ";

                    int confirm = _getch();

                    if (confirm == 'y' || confirm == 'Y')
                    {
                        if (killProcess(pid))
                        {
                            cout << "\n\nProcess killed successfully!" << endl;
                        }
                        else
                        {
                            cout << "\n\nFailed to kill process!" << endl;
                            cout << "Possible reasons:" << endl;
                            cout << "- System protected process" << endl;
                            cout << "- Insufficient permissions" << endl;
                            cout << "- Process already terminated" << endl;
                        }
                    }
                    else
                    {
                        cout << "\n\nKill operation cancelled." << endl;
                    }

                    cout << "\nPress any key to continue...";
                    _getch();
                }
            }
        }

        Sleep(1000); // Refresh every 1 second
    }

    system("cls");
    cout << "\n========================================" << endl;
    cout << "  System Monitor Tool - Shutting Down  " << endl;
    cout << "========================================" << endl;
    cout << "\nThank you for using System Monitor!" << endl;
    cout << "Exiting..." << endl;
    Sleep(1000);

    return 0;
}
