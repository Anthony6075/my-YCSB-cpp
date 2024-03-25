#!/usr/bin/python3

import time
import string
import random

TIMES = 32
N = 1 * 1024 * 1024
data = ''.join(random.choices(string.ascii_uppercase + string.digits, k=N))
# data = 'test'

with open("test.txt", "w") as test_file:
    for i in range(TIMES):
        test_file.write(data)
        # test_file.flush()
        time.sleep(0.5)

print("write done")

with open("test.txt") as test_file:
    for i in range(TIMES):
        d = test_file.read(N)
        time.sleep(0.5)

print("read done")
    
while True:
    time.sleep(1)