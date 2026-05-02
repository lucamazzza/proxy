<p align='center'>

<img src="https://capsule-render.vercel.app/api?type=venom&color=009130&height=200&section=header&text=Proxy&fontSize=90&fontColor=ffffff&animation=fadeIn&fontAlignY=35&desc=A%20Proxy%20based%20on%20Appwrite&descAlignY=61&descAlign=50"/>

</p>

`proxy` is a Qt/C++ project centered on **appcomm**, a communication layer that lets clients exchange messages through Appwrite when direct network connectivity is restricted.

## What is appcomm

`appcomm` is the shared library in `appcomm/` used by both frontend and backend.  
Its purpose is to provide a clean, Qt-native abstraction over Appwrite REST + Realtime for:

- authentication (guest and email session flows)
- topic/membership-based messaging
- real-time message delivery
- message querying, recovery, and basic delivery protections (rate limiting / state handling)

It is designed so UI or service code can consume high-level signals and methods instead of dealing with raw Appwrite request/event plumbing.

## How does it work

Clients authenticate against Appwrite and resolve their membership/topic context. Messages are stored as Appwrite documents, while realtime subscriptions notify connected clients of new events.  
When events are received, appcomm extracts payloads, maps them to internal models, and emits Qt signals for application code. Recovery/query components are used to backfill missed data and keep message flow coherent after reconnects or transient issues.

On the server side, the backend app uses appcomm + bootstrap helpers to configure or reuse the Appwrite database/collections and expose operational commands for users, topics, sessions, members, and messages.

## How do i use it

### Build

Requirements: Qt 6 (Core, Network, WebSockets, Quick, QuickControls2, Qml), CMake >= 3.16, C++17 compiler.

```bash
git clone https://github.com/lucamazzza/proxy.git
cd proxy
cmake -B build -S . -DBUILD_TESTING=ON
cmake --build build --parallel
```

### Configure backend and run it

Backend writes config to `~/.proxy-backend-config.json` (or `PROXY_BACKEND_CONFIG_PATH` if set).

```bash
./build/backend/backend configure \
  https://<your-appwrite-endpoint>/v1 \
  <projectId> \
  <apiKey> \
  <databaseId> \
  messages members topics sessions pendingmessages \
  --guest-access false
```

```bash
./build/backend/backend start
```

### Run the frontend demo

Set your Appwrite values in `frontend/main.cpp` (`endpoint`, `projectId`, `databaseId`, and collection IDs), then run:

```bash
./build/frontend/appfrontend
```

### Run tests

```bash
ctest --test-dir build/test --output-on-failure
```

`tst_appwritesdk` needs `APPWRITE_ENDPOINT`, `APPWRITE_PROJECT_ID`, and `APPWRITE_API_KEY`; if missing, that integration suite is skipped.

## What if something does not work

- **Build errors (Qt not found):** provide `-DCMAKE_PREFIX_PATH=/path/to/Qt/...` when configuring CMake.
- **Backend config issues:** verify `~/.proxy-backend-config.json` exists and has valid values, or set `PROXY_BACKEND_CONFIG_PATH`.
- **Auth/permission failures:** check Appwrite project ID, API key scopes, and collection/document permissions.
- **No realtime messages:** verify WebSocket connectivity and that topic/membership documents are correctly set for the user.
- **Frontend cannot connect:** confirm `frontend/main.cpp` uses the same endpoint/project/database/collection IDs as backend setup.

## Credits

Developed as a SUPSI project by Luca Mazza and Manuela Mondini.

## License

Copyright (c) 2026 Luca Mazza, Manuela Mondini
