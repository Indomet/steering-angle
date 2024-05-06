import unittest
import numpy as np
import pandas as pd

class Test_accuracy(unittest.TestCase):
    
    def predict(self,speed):
        coefficients = np.array([0.00000000e+00, 3.88192460e-03, -2.24144143e-05, -5.07314594e-07, 7.27527660e-09, 3.34496405e-11, -5.53052388e-13])
        powers = np.array([i for i in range(len(coefficients))])
        return np.sum(coefficients * speed ** powers)
    
    def test_accuracy(self):
        paths = ['./data/data.csv','./data/fifth_rec.csv','./data/fourth_rec.csv','./data/third_rec.csv']
        for path in paths:
            self.assertTrue(self.get_accuracy(path) >= 0.25)
    
    def get_accuracy(self,path):
        df = pd.read_csv(filepath_or_buffer=path,delimiter=';')
        speeds = df['speed']
        ground_steerings = df['groundsteering']
        total = 0
        total_correct=0
        for speed,angle in zip(speeds,ground_steerings):
            if(angle==0 or angle==-0):
                continue
            
            total+=1
            prediction = self.predict(speed)
            upper_bound = (1.25*angle)
            lower_bound = (0.75*angle) 
            
            if angle<0:
                upper_bound = (0.75*angle)
                lower_bound = (1.25*angle)
            
            if prediction >= lower_bound and prediction <= upper_bound:
                total_correct+=1
        return total_correct/total


if __name__ == '__main__':
    unittest.main()