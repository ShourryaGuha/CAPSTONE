import pandas as pd
import numpy as np

df = pd.read_csv('Accelerometer.csv')
consecutive_deltas = df['seconds_elapsed'].diff()
avg_sampling_rate = 1 / np.mean(consecutive_deltas)
shortest_gap = np.min(consecutive_deltas)
maximum_gap = np.max(consecutive_deltas)
total_num_of_samples = len(df.index)
print(avg_sampling_rate)