#!/usr/bin/env node

const FS = require('fs')
var requestRaw = process.argv[2] || "{}"
if (parseInt(process.env.CONTENT_LENGTH || "0") > 0) requestRaw = FS.readFileSync(0, 'utf8')
const request = JSON.parse(requestRaw)
if (request.error == true) throw new Error("An error occured!");
const response = JSON.stringify(request)
process.stdout.write("HTTP/1.1 200 OK\r\nStatus: 200 OK\r\nContent-Type: application/json; charset=utf-8\r\n\r\n")
process.stdout.write(response)
