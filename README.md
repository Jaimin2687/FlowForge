# FlowForge ⚙️

A modular, extensible C++ workflow automation engine inspired by Zapier/n8n/IFTTT.

## Features

- Dynamic plugin loading (.so / .dylib)
- JSON-based workflow configuration
- Thread pool for concurrent workflow execution
- Advanced rule engine for conditional execution with logical operators
- Persistent state storage
- Thread-safe logging
- Example plugins: Compress (ZIP), Email (SMTP), Message (SMS)

## What's Included

- `src/` — Engine core (WorkflowManager, Workflow, PluginLoader, Logger, ThreadPool, RuleEngine, Storage, PathUtils)
- `plugins/` — Example plugins built as shared libraries: `PrintAction`, `CompressAction`, `EmailPlugin`, `MessagePlugin`
- `config/workflows.json` — Example workflows
- `data/` — Runtime data directory (backups, uploads, state)
- `logs/` — Log files

## Quick Start

### 1. Prerequisites

- CMake 3.14+
- C++17 compiler
- ZLIB (for compression)
- CURL (for email and SMS)
- nlohmann/json (single header included)

### 2. Build

```bash
git clone <your-repo-url>
cd workflow-engine
mkdir -p build
cd build
cmake ..
make -j4
```

The build produces `build/flowforge` and plugin shared libraries in `plugins/`.

### 3. Configure Workflows

Edit `config/workflows.json` to define your workflows. Example workflows are provided.

### 4. Environment Variables

Set credentials for messaging plugins:

- **EmailPlugin (Gmail SMTP):**
  - `SMTP_USER` — Your Gmail address
  - `SMTP_PASS` — Gmail app password (generate at https://myaccount.google.com/apppasswords)

- **MessagePlugin (Twilio SMS):**
  - `TWILIO_SID` — Twilio Account SID
  - `TWILIO_TOKEN` — Twilio Auth Token
  - `TWILIO_FROM` — Twilio phone number

Example:

```bash
export SMTP_USER="your-email@gmail.com"
export SMTP_PASS="your-app-password"
export TWILIO_SID="AC..."
export TWILIO_TOKEN="your-token"
export TWILIO_FROM="+1234567890"
```

### 5. Run the Engine

```bash
./build/flowforge
```

## CLI Features

- Lists available workflows with numbers
- `a`: Run all workflows (with confirmation)
- `m`: Run multiple workflows by number/range (e.g., `1,3-4`)
- `r`: Run single workflow by number or name
- `d`: Dry-run: show actions without executing
- `s`: Schedule workflow to run after N minutes (background thread)
- `q`: Quit

Output uses ANSI colors. Scheduling is in-memory; persists across restarts via `data/state.json`.

## Rule Engine

The advanced rule engine allows workflows to run conditionally based on system states and logical combinations:

### Conditions
- **System Metrics:**
  - `disk`: Disk usage percentage (e.g., `"> 80%"`)
  - `cpu`: CPU usage percentage (e.g., `"> 50%"`)
  - `memory`: Memory usage percentage (e.g., `"> 75%"`)

- **File System:**
  - `file`: File existence checks (e.g., `"exists /path/to/file"`)

- **Time-Based:**
  - `time`: Time range checks (e.g., `"between 09:00 and 17:00"`)

### Logical Operators
- `and`: All conditions must be true
- `or`: At least one condition must be true
- `not`: Negate a condition

### Examples
```json
{
  "rule": {
    "if": {
      "and": [
        { "disk": "> 80%" },
        { "memory": "> 75%" }
      ]
    }
  }
}
```

```json
{
  "rule": {
    "if": {
      "or": [
        { "file": "exists /tmp/alert.txt" },
        { "cpu": "> 90%" }
      ]
    }
  }
}
```

## Plugins

- **CompressAction** — Compresses specified path to timestamped ZIP in `data/backups/` using ZLIB
- **EmailPlugin** — Sends email via Gmail SMTP
- **MessagePlugin** — Sends SMS via Twilio API

To add a plugin: Create `.cpp` in `plugins/` implementing `IAction` with `extern "C" IAction* create_action()`. Rebuild with CMake.

## Logs

Logs to `logs/engine.log`. Override with `FLOWFORGE_LOG_DIR`.

## Troubleshooting

- Plugin load failures: Check paths in error messages
- Email/SMS failures: Verify credentials and enable verbose logging if available

## License

MIT
