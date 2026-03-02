<div align="center">

# V-Null

### Say no to invasiveness and yes to privacy.

A lightweight Windows service that monitors and blocks unwanted processes in real time.

![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-0078D6?logo=windows&logoColor=white)
![C++](https://img.shields.io/badge/language-C%2B%2B-00599C?logo=cplusplus&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green)

</div>

---

## Overview

V-Null is a native Windows service that silently monitors process creation events and automatically blocks executables that match a configurable blacklist. When a blocked process is detected, V-Null terminates it and displays a system-level error dialog — making the termination appear as a natural application crash.

## Features

- 🔍 **Real-Time Monitoring** — Uses WMI event subscriptions to detect new processes within ~100ms of launch
- 💥 **Stealth Termination** — Injects minimal shellcode to trigger a genuine access violation, avoiding the typical `TerminateProcess` fingerprint
- 🪟 **Native Error Dialog** — Displays a Windows-style "Application Error" message box via `WTSSendMessage`, making the block look like a natural crash
- 📋 **Flexible Blacklist** — Supports exact name matching and prefix matching for broader coverage
- 🚀 **Runs as a Service** — Starts automatically with Windows, operates silently in the background
- 🎨 **Splash Screen** — Displays a styled console banner on the user's desktop when the service starts

## How It Works

```
Process Created → WMI Event Fires → Name Checked Against Blacklist
                                              │
                                    ┌─────────┴──────────┐
                                    │                     │
                                No Match              Match Found
                                (ignore)                  │
                                              ┌───────────┴───────────┐
                                              │                       │
                                     Inject Shellcode          Show Error Dialog
                                     (null-ptr deref)          (WTSSendMessage)
                                              │
                                     Process Crashes
                                     (0xC0000005)
```

### Termination Method

Instead of calling `TerminateProcess` (which is easily detected and logged), V-Null:

1. Opens the target process with `PROCESS_ALL_ACCESS`
2. Allocates executable memory via `VirtualAllocEx`
3. Writes a 6-byte shellcode payload that dereferences a null pointer:
   ```asm
   xor rcx, rcx      ; rcx = 0
   mov [rcx], rcx     ; write to address 0x0 → ACCESS_VIOLATION
   ```
4. Executes the shellcode via `CreateRemoteThread`
5. The process crashes with exception code `0xC0000005` — indistinguishable from a natural bug

## Installation

### Build from Source

**Requirements:**
- Visual Studio 2022 (or later) with the **Desktop development with C++** workload
- Windows SDK 10.0+
- x64 target platform

**Steps:**

1. Clone the repository:
   ```sh
   git clone https://github.com/yourusername/V-Null.git
   ```
2. Open `V-Null.slnx` in Visual Studio
3. Build the solution in **Release | x64**

### Install the Service

Run the built executable from an **Administrator** command prompt:

```sh
V-Null.exe -install
```

The service will be registered as **V-Null Service** and configured to start automatically with Windows.

### Remove the Service

```sh
V-Null.exe -remove
```

## Configuration

### Blacklist

Edit `Blacklist.h` to define which processes to block. Each entry specifies an executable name, a category label, and a match mode:

```cpp
inline std::vector<BlacklistEntry> get_blacklist ( )
{
    return
    {
        { L"malware.exe"   , L"Malware"   , MatchMode::Exact  },
        { L"SuspiciousApp-", L"Suspicious", MatchMode::Prefix },
    };
}
```

| Match Mode | Behavior | Example |
|---|---|---|
| `Exact` | Matches the full process name (case-insensitive) | `malware.exe` matches only `malware.exe` |
| `Prefix` | Matches any process starting with the string | `Ocean-` matches `Ocean-Client.exe`, `Ocean-Loader.exe`, etc. |

> [!NOTE]
> Changes to the blacklist require a rebuild and service reinstallation.

## Project Structure

```
V-Null/
├── V-Null.cpp              # Entry point — service install/remove/splash
├── VNullService.cpp/.h     # Service lifecycle (OnStart/OnStop)
├── ProcessWatcher.cpp/.h   # WMI monitoring + process termination
├── Blacklist.h             # Configurable blocked process list
├── ServiceBase.cpp/.h      # Generic Windows service base class
├── ServiceInstaller.cpp/.h # Service registration/unregistration
└── ServiceConfig.h         # Service name and startup config
```

## Requirements

- Windows 10 / 11 (x64)
- Administrator privileges (required for service installation and process access)
- The service must run under the **Local System** account for full process access

## Disclaimer

This tool is intended for **personal use** — managing which software runs on your own machine. Use responsibly and in accordance with applicable laws. The authors are not responsible for any misuse.
