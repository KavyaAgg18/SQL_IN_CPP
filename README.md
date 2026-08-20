# DBMS in C++

A file-based mini DBMS in C++ with an interactive CLI and a lightweight SQL parser.
The system stores metadata and table data as CSV files inside the Databases folder.

## Project Layout

```text
SQL_IN_CPP/
  include/                # Header files
  src/
    caller/               # CLI entrypoint / callers
    core/                 # Database + table core logic
    handlers/             # SQL/query handlers (parser, joins)
  tests/                  # Query test input files
  docs/                   # Feature and SQL notes
  tools/                  # Standalone experiments/utilities
  Databases/              # Runtime storage (generated/updated at runtime)
  output/                 # Build output artifacts
```

## What Is Implemented

- Multi-database management (create, select, list)
- Table lifecycle operations (create, rename, truncate, drop, list)
- Row insertion in two modes:
  - positional insert
  - insert with explicit column list (missing columns are stored as NULL)
- Data type validation at insert time
- SELECT queries with column projection
- JOIN queries:
  - INNER JOIN
  - LEFT JOIN
  - RIGHT JOIN
  - FULL JOIN
- SHOW commands:
  - SHOW TABLES
  - SHOW DATABASES
- Automatic bootstrapping of required storage files/folders at startup

## Supported SQL Commands

The parser in src/handlers/sqlparser.cpp currently supports the following commands:

### DDL

- CREATE TABLE
- RENAME TABLE ... TO ...
- DROP TABLE
- TRUNCATE TABLE

### DML

- INSERT INTO table VALUES (...)
- INSERT INTO table (col1, col2, ...) VALUES (...)

### DQL

- SELECT * FROM table
- SELECT col1, col2 FROM table
- SELECT ... FROM table1 <JOIN_TYPE> JOIN table2 ON colA = colB

### Utility

- SHOW TABLES
- SHOW DATABASES

## Data Types

The engine validates and stores these logical types:

| SQL Type | Internal ID | Notes |
|----------|-------------|-------|
| INT      | 0           | Parsed with stoi |
| FLOAT    | 1           | Parsed with stof |
| BOOL     | 2           | Accepts TRUE/FALSE/1/0 |
| BOOLEAN  | 2           | Alias of BOOL |
| STRING   | 3           | Single-quoted strings supported |
| DATE     | 4           | Expected format: YYYY-MM-DD |

## CLI Flow

The executable starts with:

1. Initialization of Databases/information_schema.csv and Databases/baseDb
2. Main menu:
   - Create Database
   - Select Database
   - Show Databases
   - Exit
3. Table operations menu (after selecting a database):
   - Create Table (via SQL)
   - Show Tables
   - Run SQL Query loop

## Build and Run

```bash
# Build
make

# Optional: include mongo-c-driver libs if you need that integration
make USE_MONGO=1

# Run
./main

# Clean object files and binary
make clean

# Reset persisted database folder
make reset
```

On Windows, run the produced executable directly (for example main.exe).

## Run in Browser via GitHub Pages

This repo can be deployed as an interactive WebAssembly app on GitHub Pages.

### What is included

- A workflow at `.github/workflows/deploy-pages.yml`
- Automatic build on push to `main`/`master` and manual run support
- C++ sources compiled with Emscripten to `index.html + .js + .wasm`
- `Databases/` preloaded into the browser filesystem at startup

### First-time setup in your GitHub repo

1. Push this repository (including the workflow) to GitHub.
2. Open your repository on GitHub.
3. Go to **Settings -> Pages**.
4. Under **Build and deployment**, set **Source** to **Deploy from a branch**.
5. Set **Branch** to `gh-pages` and folder to `/(root)`.
6. Go to the **Actions** tab and run (or wait for) **Build and Publish GitHub Pages Site**.

After deployment, your app will be available at:

`https://<your-username>.github.io/<your-repo-name>/`

### Interaction notes

- The app runs fully in-browser as WebAssembly.
- The deployed page uses a minimal terminal-like UI (output pane + single input line).
- Input is captured asynchronously from the on-page terminal box (no browser prompt popups).
- Press Enter in the input box to send one full line to the running program.
- Runtime file changes are in browser memory for that session and are not pushed back to GitHub.

## Example Queries

```sql
CREATE TABLE users (id INT, name STRING, age INT, is_active BOOL);
CREATE TABLE departments (id INT, department_name STRING);

INSERT INTO users (1, 'Alice', 30, TRUE);
INSERT INTO users (id, name) VALUES (2, 'Bob');

SELECT * FROM users;
SELECT id, name FROM users;

SELECT * FROM users INNER JOIN departments ON id = id;
SELECT * FROM users LEFT JOIN departments ON id = id;

SHOW TABLES;
SHOW DATABASES;

RENAME TABLE users TO app_users;
TRUNCATE TABLE app_users;
DROP TABLE app_users;
```

## Storage Layout

```text
Databases/
  information_schema.csv            # database_name,date_created
  <db_name>/
    tables.csv                      # table_name,date_created
    <table_name>/
      columns.csv                   # col_name,data_type
      data.csv                      # raw rows
```

## Current Limitations

These are present in helper docs or placeholders but not implemented in active execution flow yet:

- WHERE, GROUP BY, HAVING, ORDER BY, LIMIT, OFFSET, DISTINCT
- UPDATE and DELETE
- ALTER TABLE
- CROSS JOIN and SELF JOIN (commented placeholders)
- PK/FK/constraint enforcement

Notes:

- Table and column identifiers are normalized to lowercase by the parser.
- JOIN queries print a temporary joined table result and then drop that temp table.

## Key Source Files

- src/caller/main.cpp: menu-driven CLI
- src/handlers/sqlparser.cpp: query parsing and dispatch
- src/core/database.cpp: database creation/selection/listing
- src/core/table.cpp: table creation, insert, display, rename, drop, truncate
- src/handlers/join.cpp: inner/left/right/full join execution
- src/core/globals.cpp: datatype mappings and timestamp helper

