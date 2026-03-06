# Nodepp PostgreSQL Client
A modern, high-performance PostgreSQL database driver built for the Nodepp framework. This client leverages Nodepp's coroutines, promises, and events to execute SQL commands non-blockingly, ensuring scalable I/O operations.

## Dependencies & CMake Integration
```bash
#postgres-dev
  🪟: pacman -S mingw-w64-x86_64-postgresql
  🐧: sudo apt install libpq-dev
#Openssl
  🪟: pacman -S mingw-w64-ucrt-x86_64-openssl
  🐧: sudo apt install libssl-dev
```
```bash
include(FetchContent)

FetchContent_Declare(
	nodepp
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp)

FetchContent_Declare(
	nodepp-postgres
	GIT_REPOSITORY   https://github.com/NodeppOfficial/nodepp-postgres
	GIT_TAG          origin/main
	GIT_PROGRESS     ON
)
FetchContent_MakeAvailable(nodepp-postgres)

#[...]

target_link_libraries( #[...]
	PUBLIC nodepp nodepp-postgres #[...]
)
```

## Connection and Initialization
The postgres_t object is constructed using a connection URI and the target database name.

```cpp
#include <nodepp/nodepp.h>
#include <postgres/postgres.h>

using namespace nodepp;

void main() {
    // Note: The postgres_t constructor blocks the current fiber until connection is established.
    try {
        // Connect to 'mydatabase' using details from the URI (e.g., 'postgres://user:pass@host:5432')
        auto db = postgres_t("postgres://localhost:5432", "mydatabase");

        // Example: Execute a non-query command synchronously
        db.emit("CREATE TABLE IF NOT EXISTS inventory (item_id SERIAL PRIMARY KEY, name TEXT);");

        console::log("Connected and table created.");

    } catch( except_t error ) {
        console::error("Connection or initialization failed:", error.get());
    }
}
```

## Compilation
```bash
g++ -o main main.cpp -I ./include -lpq -lssl -lcrypto ; ./main
```

## License
**Nodepp-Postgres** is distributed under the MIT License. See the LICENSE file for more details.