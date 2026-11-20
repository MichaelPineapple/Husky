#!/usr/bin/env python3

import os
import sys
import json

request_raw = "{}"
if len(sys.argv) > 1:
    request_raw = sys.argv[1]
content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
if content_length > 0: request_raw = sys.stdin.read(content_length)
request = json.loads(request_raw)

if "error" in request and request["error"] == True:
    raise Exception("An error occured!")

response = json.dumps(request)
sys.stdout.write("HTTP/1.1 200 OK\r\nStatus: 200 OK\r\nContent-Type: application/json; charset=utf-8\r\n\r\n")
sys.stdout.write(response)
