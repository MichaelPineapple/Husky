import requests
import json
import time
from enum import Enum

def run_test(id, endpoint, payload=None, expected_status=200, expected_body=None):

    #time.sleep(0.01)
    url = "http://localhost" + endpoint
    response = None
    grade = "\nTest #" + str(id) + ": "
    succ = True
    
    try:
        if payload: response = requests.post(url, json=payload)
        else: response = requests.get(url)
    except Exception as ex:
        grade += "\nAn error occured: " + str(ex)
        succ = False
        
    actual_body = None
    try: actual_body = response.json()
    except: pass
    
    actual_status = None
    try: actual_status = response.status_code
    except: pass
        
    if actual_status != expected_status:
        grade += "\nIncorect Status Code! Expected: " + str(expected_status) + ", Actual: " + str(actual_status)
        succ = False
    
    if expected_body:
        if str(actual_body) != str(expected_body):
            grade += "\nIncorect Response! Expected: " + str(expected_body) + ", Actual: " + str(actual_body)
            succ = False
    
    if not succ: print(grade + "\n")

dummy_payload = { "test": "abc" }

# *** HTML ***

# OK
run_test(0, "")
run_test(1, "", "abc")
run_test(2, "/index.html")

# TOO LARGE
# run_test(3, "", payload=("X" * 200000), expected_status=413)

# NOT FOUND
run_test(4, "/not_found.html", expected_status=404)
run_test(5, "/not_found", expected_status=404)

# *** BIN ***

# OK
run_test(6, "/bin/program.py", payload=dummy_payload, expected_body=dummy_payload)
run_test(7, "/bin/program.py", expected_body="{}")

# TOO LARGE
run_test(8, "/bin/program.py", payload=("X" * 20000), expected_status=413)

# ERROR
run_test(9, "/bin/program.py", payload=json.dumps({ "error": True }), expected_status=500)

# NOT FOUND
#run_test(10, "/bin/not_found.py", expected_status=404)
#run_test(11, "/bin/not_found", expected_status=404)
#run_test(12, "/bin", expected_status=404)
