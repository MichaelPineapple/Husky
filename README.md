# Husky
HTTP Server

Compile MacOS
```
gcc main.c \
    -I$(brew --prefix openssl)/include \
    -L$(brew --prefix openssl)/lib \
    -lssl -lcrypto -lpthread
```

Compile Linux
```
gcc main.c -lssl -lcrypto -lpthread
```

Run
```
./a.out 443 server.cert server.key
```

Install OpenSSL Dev Library On Linux
```
sudo yum install openssl-devel
```

Example Curl Request
```
curl -k -X POST https://localhost \
     -H "Content-Type: application/json" \
     -d '{"test":"hello"}'
```

Generate SSL Certificate
```
openssl req -x509 -newkey rsa:2048 \
    -keyout server.key \
    -out server.cert \
    -days 365 \
    -nodes
```
