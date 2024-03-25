import matplotlib.pyplot as plt

data = []

with open("output.txt") as input:
    for line in input:
        d = line.split(' ')[2]
        data.append(float(d) / 1024)

x = [i for i in range(len(data))]
y = data

plt.plot(x, y)
plt.xlabel("no.")
plt.ylabel("mem usage (MB)")
plt.title("mem usage")
plt.savefig("output.png")