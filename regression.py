import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


# Load the CSV data into a DataFrame

df = pd.read_csv("CID-140-recording-2020-03-18_144821-selection.rec.csv", delimiter=";")


x = df["right_x"]
y = df["groundsteering"]

fake_x = np.array(x)
fake_y = np.array(y)

x_total = []
y_total = []

for num in fake_x:
    x_total.append(float(num))

for num in fake_y:
    y_total.append(float(num))

# x_total = [x for x, y in zip(x_total, y_total) if x > 0 and y >= 0]

assert len(x_total) == len(y_total)

print(max(y_total))
print(min(y_total))
print(max(x_total))
print(min(x_total))
# get a polynomial function
# coefficents = np.polyfit(x_total, y_total, 100)

# y_fit = np.polyval(coefficents, x_total)

# string to store the coefficients
# poly_str = "y = "
# for i, coeff in enumerate(reversed(coefficents)):
#     if i == 0:
#         poly_str += f"{coeff}"
#     elif i == 1:
#         poly_str += f" + {coeff}x"
#     else:
#         poly_str += f" + {coeff}x^{i}"

# print(f"The function is {poly_str}")
# plt.plot(x_total, y_total, "o")
plt.plot(
    [x for x, y in zip(x_total, y_total) if x >= 0 and y != 0],
    [y for x, y in zip(x_total, y_total) if x >= 0 and y != 0],
    "o",
)
plt.show()
