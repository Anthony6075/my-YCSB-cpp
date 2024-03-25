#!/usr/bin/python3

import matplotlib.pyplot as plt

begin_time = ''
interval = 1.0

write_data = []
read_data = []

with open("write_output.txt") as write_input:
    line_no = 0
    for line in write_input:
        line_no += 1

        if line_no == 1:
            begin_time = line.strip()
            continue
        elif line_no == 2:
            interval = float(line.strip())
            continue

        d = line.split(' ')[1].strip()
        write_data.append(float(d) / (1024 * 1024))

with open("read_output.txt") as read_input:
    line_no = 0
    for line in read_input:
        line_no += 1

        if line_no == 1:
            if begin_time != line.strip():
                print("begin_time differ")
                exit(1)
            continue
        elif line_no == 2:
            if interval != float(line.strip()):
                print("interval differ")
                exit(1)
            continue

        d = line.split(' ')[1].strip()
        read_data.append(float(d) / (1024 * 1024))

write_x = [i * interval for i in range(len(write_data))]
write_y = write_data

read_x = [i * interval for i in range(len(read_data))]
read_y = read_data

plt.plot(write_x, write_y, 'b', label='write')
plt.plot(read_x, read_y, 'r', label='read')
plt.legend()
plt.xlabel("time (s)")
plt.ylabel("io (MB)")
plt.title(f"io, time: {begin_time}, interval: {interval}s")
plt.savefig("output.png")
plt.show()