# FlowForge ⚙️

A modular, extensible C++ workflow automation engine inspired by Zapier/n8n/IFTTT.

## Features

- Dynamic plugin loading (.so / .dylib)
- JSON-based workflow configuration
- Thread pool for concurrent workflow execution
- Advanced rule engine for conditional execution with logical operators
- Persistent state storage
- Thread-safe logging
- Menu-driven CLI with per-action parameter overrides
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

`config/workflows.json` ships with three ready-to-run examples:

- `CompressFiles` → Archives `project_folder/` into `data/backups/`
- `SendEmailReminder` → Sends a reminder email via `EmailPlugin`
- `SendSMSAlert` → Sends an SMS alert via `MessagePlugin`

Edit this file to add or tweak workflows. The engine reloads configurations each time it starts.

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
# Optional: turn on verbose SMTP logging
export SMTP_DEBUG=1
```

### 5. Run the Engine

```bash
./build/flowforge
```

From the repository root you can also run a single workflow non-interactively:

```bash
./build/flowforge run SendEmailReminder
```

## CLI Features

When you launch `flowforge` you get a simple menu:

1. **List available workflows** — shows the names loaded from `config/workflows.json`.
2. **Run a workflow** — choose one or more workflows by number (comma-separated) to execute immediately.
3. **Run with parameter overrides** — walk through each action in a workflow and optionally supply JSON overrides at runtime.
4. **Exit** — quit the CLI.

All required runtime directories (`data/backups`, `data/uploads`, `logs`) are created automatically when the process starts.

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

- **CompressAction** — Compresses a target path into a timestamped ZIP inside `data/backups/` using ZLIB.
- **EmailPlugin** — Sends mail via Gmail SMTP over SMTPS. It honours `SMTP_USER`/`SMTP_PASS` environment variables and falls back to the bundled demo credentials for local testing. Set `SMTP_DEBUG=1` to capture the full SMTP transcript in `logs/email_plugin.log` when troubleshooting.
- **MessagePlugin** — Sends SMS via Twilio REST API using `TWILIO_SID`, `TWILIO_TOKEN`, and `TWILIO_FROM`. Activity is recorded in `logs/message_plugin.log`.

To add a plugin: create a `.cpp` file in `plugins/` that implements `IAction` and exposes `extern "C" IAction* create_action()`, then rebuild with CMake.

## Logs

- Core engine events → `logs/engine.log` (override the directory with `FLOWFORGE_LOG_DIR`).
- Email plugin activity → `logs/email_plugin.log`; enable extended curl tracing with `SMTP_DEBUG=1`.
- Message plugin activity → `logs/message_plugin.log`.

## Troubleshooting

- Plugin load failures: Check paths in error messages
- Email/SMS failures: Verify credentials, then inspect `logs/email_plugin.log` or `logs/message_plugin.log`; set `SMTP_DEBUG=1` for detailed SMTP transcripts

## License

MIT
