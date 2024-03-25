#!/usr/bin/python3

import matplotlib.pyplot as plt

begin_time = ''
interval = 1.0

data = []

with open("output.txt") as input:
    line_no = 0
    for line in input:
        line_no += 1

        if line_no == 1:
            begin_time = line.strip()
            continue
        elif line_no == 2:
            interval = float(line.strip())
            continue

        d = line.split(' ')[2]
        data.append(float(d) / 1024)

x = [i * interval for i in range(len(data))]
y = data

plt.plot(x, y)
plt.xlabel("time (s)")
plt.ylabel("mem (MB)")
plt.title(f"mem, time: {begin_time}, interval: {interval}s")
plt.savefig("output.png")