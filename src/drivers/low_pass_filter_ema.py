class LowPassFilterEMA:
    def __init__(self, alpha=0.05, initial_value=0.0):
        """
        Exponential Moving Average (EMA) Low-Pass Filter.

        Formula:
        y_t = alpha * x_t + (1 - alpha) * y_{t-1}

        where:
        - x_t is the new raw input value (gyroscope reading).
        - y_t is the filtered output at time t.
        - alpha is the smoothing factor (0 < alpha < 1).
        - y_{t-1} is the previous filtered output.

        :param alpha: Smoothing factor (0 < alpha < 1).
        :param initial_value: Initial filtered value.
        """
        self.alpha = alpha
        self.filtered_value = initial_value  # Initialize with first value
    
    def filter(self, new_value):
        """
        Updates the filter with a new value and returns the filtered output.

        :param new_value: New raw input value (gyroscope reading).
        :return: Filtered output.
        """
        self.filtered_value = self.alpha * new_value + (1 - self.alpha) * self.filtered_value
        return self.filtered_value
