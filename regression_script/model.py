from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def predict(speed):
    coefficients = np.array([0.00000000e+00, 3.88192460e-03, -2.24144143e-05, -5.07314594e-07, 7.27527660e-09, 3.34496405e-11, -5.53052388e-13])
    powers = np.array([i for i in range(len(coefficients))])
    return np.sum(coefficients * speed ** powers)


df = pd.read_csv(filepath_or_buffer="all.csv",delimiter=';')

df = df[df['groundsteering'] != 0]
#remove outliers
df = df[~((df['groundsteering'] >= 0.26) & (df['speed'] >= -85) & (df['speed'] <= 49))]
df = df[~((df['groundsteering'] <= -0.26) & (df['speed'] >= -85) & (df['speed'] <= 49))]

speeds = df['speed']
ground_steerings = df['groundsteering']


coefficents = np.polyfit(speeds, ground_steerings, 6)
y_fit = np.polyval(coefficents, speeds)

poly_str = "y = "
for i, coeff in enumerate(reversed(coefficents)):
    if i == 0:
        poly_str += f"{coeff}"
    elif i == 1:
        poly_str += f" + {coeff}x"
    else:
        poly_str += f" + {coeff}x^{i}"

print(f"The function is {poly_str}")


plt.scatter(speeds, ground_steerings, color='blue')
plt.plot(speeds, y_fit, color='red')
plt.show()

#test accuracy
total = 0
total_correct=0
for speed,angle in zip(speeds,ground_steerings):
  total+=1
  prediction = predict(speed)
  upper_bound = (1.25*angle)
  lower_bound = (0.75*angle) 
  
  if angle<0:
    upper_bound = (0.75*angle)
    lower_bound = (1.25*angle)
  
  if prediction >= lower_bound and prediction <= upper_bound:
    total_correct+=1
  print(f"Speed: {speed} | Angle: {angle} | Prediction: {prediction} | accuracy {total_correct/total}")