<p align='center'>

<img src="https://capsule-render.vercel.app/api?type=venom&color=009130&height=200&section=header&text=Proxy&fontSize=90&fontColor=ffffff&animation=fadeIn&fontAlignY=35&desc=A%20Proxy%20based%20on%20Appwrite&descAlignY=61&descAlign=50"/>

</p>

A real-time communication library built with Qt/C++ that enables secure messaging between applications behind firewalls
using Appwrite as a cloud intermediary.

## Introduction

Proxy addresses the challenge of communication between isolated networks (e.g., corporate firewalls, private networks)
by leveraging Appwrite's cloud infrastructure as a secure relay point. The library provides a Qt-based abstraction layer
over Appwrite's REST and Realtime APIs, enabling bidirectional messaging without requiring direct network connectivity
between endpoints.

### Key Features

- Asynchronous REST API wrapper for Appwrite
- Qt signal/slot architecture for event-driven communication
- Cross-platform compatibility (Linux, macOS, Windows)
- WebAssembly support for browser deployment
- Modular architecture with clear separation of concerns

### Use Cases

- Cross-network communication between isolated environments
- IoT device management behind NAT/firewalls
- Remote administration of air-gapped systems
- Multi-site application coordination

## Architecture

The system consists of three primary layers:

### Communication Flow

1. Both endpoints authenticate with Appwrite independently
2. Clients subscribe to shared document collections
3. Messages are persisted as Appwrite documents
4. Appwrite broadcasts changes via WebSocket (Realtime API)
5. Clients receive notifications and retrieve message payloads

## Project Structure

```
proxy/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── appcomm/                    # Core communication library
│   ├── CMakeLists.txt
│   ├── appcomm.h               # Main facade interface
│   ├── appcomm.cpp
│   ├── appwritesdk.h           # Appwrite API wrappers
│   └── appwritesdk.cpp
├── frontend/                   # Client application (Qt/QML)
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── Main.qml
├── backend/                    # Server application
│   ├── CMakeLists.txt
│   └── main.cpp
└── docs/                       # Documentation
    └── docs.pdf                # Report
```

### Module Descriptions

#### appcomm

The core communication library providing:

- **appwritesdk.h/cpp**: Low-level wrappers around Appwrite REST APIs
  - `BaseSDK`: Abstract base class for HTTP requests
  - `Client`: User-facing operations (sessions, documents)
  - `Server`: Administrative operations (database, collections, users)
  - `ConnectionConfig`: Appwrite connection parameters

- **appcomm.h/cpp**: High-level facade interface (currently minimal)
  - Entry point for library consumers
  - Will coordinate SDK components
  - Planned: unified API for messaging operations

#### frontend

Qt/QML application demonstrating client-side usage:

- QML-based user interface
- Integration with appcomm library
- Designed for desktop and WebAssembly deployment

#### backend

Qt Core application for server-side operations:

- Headless Qt application
- Administrative functions
- Echo server capabilities (planned)

## Getting Started

### Prerequisites

- Qt 6.x or Qt 5.15+ (Core, Network, WebSockets modules)
- CMake 3.16 or higher
- C++17 compatible compiler
- Appwrite instance (cloud or self-hosted)

### Building the Project

#### Native Build

```bash
# Clone repository
git clone https://github.com/yourusername/proxy.git
cd proxy

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Outputs:
# - build/appcomm/libappcomm.a
# - build/frontend/appfrontend
# - build/backend/backend
```

#### WebAssembly Build

```bash
# Ensure Emscripten is installed and activated
source /path/to/emsdk/emsdk_env.sh

# Configure with Qt for WebAssembly
mkdir build-wasm && cd build-wasm
/path/to/qt-wasm/bin/qt-cmake ..

# Build
cmake --build .

# Output: build-wasm/frontend/appfrontend.{html,js,wasm}
```

### Build Options

The CMake configuration supports the following options:

- `BUILD_FRONTEND`: Build frontend application (default: ON)
- `BUILD_BACKEND`: Build backend application (default: ON)

Example:
```bash
cmake .. -DBUILD_FRONTEND=OFF -DBUILD_BACKEND=ON
```

## Integration with Appwrite

### Appwrite Setup

1. **Create an Appwrite Project**

   Visit [Appwrite Console](https://cloud.appwrite.io) or your self-hosted instance and create a new project.

2. **Configure API Keys**

   - For client applications: Use the project ID (no API key required)
   - For server applications: Generate an API key with appropriate scopes

3. **Database Structure**

   The library expects the following Appwrite database structure:

   **Collection: channels**
   ```
   Attributes:
   - name (string, 256, required)
   - adminUserId (string, 36, required)
   - createdAt (datetime, required)
   - members (string[], required)

   Permissions:
   - Read: Members only
   - Create/Delete: Admin only
   ```

   **Collection: messages**
   ```
   Attributes:
   - channelId (string, 36, required, indexed)
   - senderId (string, 36, required)
   - timestamp (datetime, required, indexed)
   - payload (json, required)

   Permissions:
   - Read: Channel members
   - Create: Authenticated users
   ```

> [NOTE!]
> The library has is designed to provide commands to bootstrap the required Appwrite database and collection structure.

### Configuration

Configure the library with your Appwrite credentials:

```cpp
#include "appwritesdk.h"

AppwriteSDK::ConnectionConfig config;
config.endpoint = "https://cloud.appwrite.io/v1";
config.projectId = "your-project-id";
config.apiKey = "your-api-key";        // Server-side only
config.dbId = "your-database-id";
config.collectionId = "messages";
```

### Authentication

#### Client-Side (User Sessions)

```cpp
#include "appwritesdk.h"
#include <QNetworkAccessManager>

QNetworkAccessManager networkManager;
AppwriteSDK::Client client(&networkManager);

// Anonymous session
client.createAnonymousSession(config);

// Email/password session
client.createEmailSession(config, "user@example.com", "password");

// Handle response
QObject::connect(&client, &AppwriteSDK::BaseSDK::requestSuccess,
                [](const QJsonObject &data) {
    QString sessionId = data["$id"].toString();
    QString userId = data["userId"].toString();
    // Store session information
});
```

#### Server-Side (API Key)

```cpp
AppwriteSDK::Server server(&networkManager);

// Create user
server.createUser(config, "newuser@example.com", "password");

// Create database
server.createDatabase(config, "ProxyDB");
```

### Sending and Receiving Messages

```cpp
// Send a message
QJsonObject messageData;
messageData["channelId"] = "channel-uuid";
messageData["senderId"] = "user-uuid";
messageData["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
messageData["payload"] = QJsonObject{{"text", "Hello"}};

client.createDocument(config, messageData);

// Receive messages via Realtime API (WebSocket)
// Implementation details in appcomm/_TODO.md
```

## Usage

### Basic Example

This section will be expanded as the library implementation progresses. The intended usage pattern:

```cpp
#include <QCoreApplication>
#include "appcomm.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Initialize appcomm with configuration
    // Connect to Appwrite
    // Authenticate user
    // Join communication channel
    // Send/receive messages

    return app.exec();
}
```

### Current Functionality

The library currently provides:

1. **Low-level Appwrite SDK**
   - HTTP request abstraction
   - Session management
   - Document CRUD operations
   - User management (admin)

2. **Planned Components**
   - Channel abstraction
   - Message validation and serialization
   - Realtime WebSocket integration
   - Automatic reconnection
   - Message recovery

Refer to `appcomm/_TODO.md` for detailed implementation specifications.

## Troubleshooting

### Build Issues

**CMake cannot find Qt**

```bash
# Specify Qt installation path
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

**WebAssembly build fails**

Ensure Emscripten SDK is properly activated:
```bash
source /path/to/emsdk/emsdk_env.sh
which emcc  # Should return path to emcc
```

### Connection Issues

**HTTP 401 Unauthorized**

- Verify `projectId` matches your Appwrite project
- For server operations, ensure API key has correct scopes
- Check session token is valid and not expired

**HTTP 404 Not Found**

- Verify `endpoint` URL is correct
- Ensure database and collection IDs exist
- Check Appwrite service is running

**Network timeout**

- Verify network connectivity to Appwrite instance
- Check firewall rules allow HTTPS (port 443)
- For WebSocket: Ensure port 80/443 allows WebSocket upgrade

### Appwrite Configuration

**Documents not visible**

- Review collection permissions
- Verify user has read access to collection
- Check document permissions are set correctly

**Cannot create documents**

- User must have authenticated session
- Collection must allow create permission for user role
- Validate JSON payload matches collection schema

### Runtime Issues

**Signal not emitted**

- Ensure QObject connections are established before operation
- Verify connection type (Qt::AutoConnection, Qt::QueuedConnection)
- Check network request completes (use QNetworkReply debugging)

**Application hangs**

- All network operations are asynchronous
- Do not use QEventLoop::exec() to wait for signals
- Use signal/slot connections for async operations

## License

Copyright (c) 2026 Luca Mazza, Manuela Mondini
