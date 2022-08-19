# Probabilistic SQL

PostgreSQL extension for probabilistic SQL queries.

## Prerequisites
1. `CMake 3.22.1`
2. `PostgreSQL 14`
3. `direnv` for automatic configuring of `PGPATH`

## Installation steps
1. `mkdir build && cd build`
2. `cmake ..`
3. `cmake --build .`
4. `sudo cmake --install .`
5. To run tests, run `cd build && ctest .`

## Credits
@mkindahl for the `pg_extension` project,
@maxim2266 for the `str` library.