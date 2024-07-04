# NTPRO test stock app

create dir and build all
```bash
mkdir build && cd build
cmake .. --target all
```

run db
```bash
docker compose up --build -d
```
run server
```bash
./build/server/server
```

run client
```bash
./build/client/client
```