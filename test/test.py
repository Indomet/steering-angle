import unittest
import numpy as np
import pandas as pd

class Test_accuracy(unittest.TestCase):
    
    def predict(self,speed):
        coefficients = np.array([0.00000000e+00, 3.96583242e-03, -1.05373477e-04, -3.17407267e-07, 4.68268257e-08, -1.49984238e-10, -5.54462195e-12, 3.47391935e-14])
        intercept = 0.05159669059756054
        powers = np.array([i for i in range(len(coefficients))])
        return intercept + np.sum(coefficients * speed ** powers)

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
    
    def test_accuracy(self):
        paths = ['./data/first_rec.csv','./data/second_rec.csv','./data/third_rec.csv','./data/fourth_rec.csv','./data/fifth_rec.csv']
        for path in paths:
            self.assertTrue(self.get_accuracy(path) >= 0.25)

if __name__ == '__main__':
    unittest.main()