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
        db.await("CREATE TABLE IF NOT EXISTS inventory (item_id SERIAL PRIMARY KEY, name TEXT);");

        console::log("Connected and table created.");

    } catch( except_t error ) {
        console::error("Connection or initialization failed:", error.get());
    }
}
```

## Asynchronous API Reference

The client offers three primary methods for executing SQL, designed for optimal concurrency in different scenarios.

**1. Asynchronous with Promise (.resolve())**

The recommended method for queries (SELECT). It runs in the background and returns a promise that resolves with the complete result set, making it perfect for structured asynchronous control.

```cpp
db.resolve("SELECT item_id, name FROM inventory WHERE item_id > 10;")

.then([]( array_t<sql_item_t> results ) {
    console::log("Fetched %d items.", results.size());
})

.fail([]( except_t error ) {
    console::error("Async query failed:", error.what());
});
```

**2. Synchronous/Blocking (.await())**

Use this when you need an immediate result in a context that can yield (block the current fiber). It acts as a synchronous wrapper around the .resolve() promise.

```cpp
try {
    auto results = db.await("SELECT COUNT(*) AS total FROM inventory;");
    console::log("Current item count:", results[0]["total"]);

} catch( except_t error ) {
    console::error("Synchronous read failed:", error.what());
}
```

**3. Asynchronous Streaming/Fire-and-Forget (.emit())**

Use emit() for non-query commands (INSERT, UPDATE, DELETE) or when processing very large result sets row-by-row.

```cpp
// Non-query command (returns PGRES_COMMAND_OK), executes successfully in the background.
db.emit("INSERT INTO inventory (name) VALUES ('Widget A');");
db.emit("INSERT INTO inventory (name) VALUES ('Gadget B');");
```
```cpp
// Example: Streaming results
db.emit("SELECT * FROM massive_table;", []( sql_item_t row ) {
    // Process the row here (e.g., write to disk, log)
    console::log("Processing row ID:", row["item_id"]);
});
```

## Compilation
```bash
g++ -o main main.cpp -I ./include -lpq -lssl -lcrypto ; ./main
```

## License
**Nodepp-Postgres** is distributed under the MIT License. See the LICENSE file for more details.