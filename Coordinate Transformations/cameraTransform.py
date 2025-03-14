import numpy as np
import cv2
import os

# point in camera coordinates (3D)
P_cam = np.array([.0, 0.0, 10.0])

# Intrinsic parameter matrix K
K = np.array([[477.00580736,   0,  511.86699298],
              [  0, 625.7461253,  371.52458483],
              [  0,   0,    1]])
              
# Distortion coefficients vector
# Format: [k1, k2, p1, p2, k3]
dist = np.array([-0.29936293, 0.10959204, 0.00269389, -0.00213927, -0.02094805])
k1, k2, p1, p2, k3 = dist

# ------------------------------
# Step 1: Normalize the 3D point
# In the pinhole camera model, the normalized image coordinates are:
x = P_cam[0] / P_cam[2]
y = P_cam[1] / P_cam[2]
# Normalized point (ideal, without distortion)
norm_point = np.array([x, y])
print("Ideal (undistorted) normalized coordinates:", norm_point)

# ------------------------------
# Step 2: Apply the distortion model to the normalized coordinates
r2 = x**2 + y**2  # squared radius from the optical axis

# Apply radial and tangential distortion
x_dist = x * (1 + k1 * r2 + k2 * (r2**2) + k3 * (r2**3)) + 2 * p1 * x * y + p2 * (r2 + 2 * x**2)
y_dist = y * (1 + k1 * r2 + k2 * (r2**2) + k3 * (r2**3)) + p1 * (r2 + 2 * y**2) + 2 * p2 * x * y

distorted_norm_point = np.array([x_dist, y_dist])
print("Distorted normalized coordinates:", distorted_norm_point)

# ------------------------------
# Step 3: Convert to homogeneous coordinates and apply the intrinsic matrix
# Create a homogeneous coordinate for the distorted normalized point
distorted_homog = np.array([x_dist, y_dist, 1.0])

# Map to pixel coordinates using the intrinsic matrix
projected = K @ distorted_homog
u = projected[0] / projected[2]
v = projected[1] / projected[2]

print("Pixel coordinates (u, v): ({:.2f}, {:.2f})".format(u, v))

# ------------------------------
# Step 4: Load a sample image and overlay the point
image_path = os.path.join(os.path.dirname(__file__), "target.jpg")  
image = cv2.imread(image_path)
cv2.circle(image, (int(u), int(v)), 8, (0, 0, 255), -1)
cv2.circle(image, (int(u), int(v)), 24, (0, 0, 255), 2)
cv2.imshow("Overlayed Point", image)
cv2.waitKey(0)
cv2.destroyAllWindows()