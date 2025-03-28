# Backend

## Build

### First Time Build

```
cd backend
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix)/Cellar/mysql-connector-c++/9.2.0" ..
make && ./drogon_backend
```

### After First Time Build

```
cd backend/build
make && ./drogon_backend
```


## Testing

From the backend/build directory

First configure the mysql login path

```bash
mysql_config_editor set --login-path=hansnap --host=127.0.0.1 --user=hansnap_user --password
```


Running the tests

```bash
cmake ..
cmake --build . --target database_tests
./database_tests
```