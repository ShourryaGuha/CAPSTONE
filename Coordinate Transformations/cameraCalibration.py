import numpy as np
import cv2
import glob

# Set the dimensions of the checkerboard
# For a checkerboard with 10 squares x 7 squares, there are 9 inner corners horizontally and 6 vertically.
pattern_size = (9, 6)  # (number of inner corners per row, number of inner corners per column)
square_size = 24.0  # mm

objp = np.zeros((pattern_size[0] * pattern_size[1], 3), np.float32)
objp[:, :2] = np.mgrid[0:pattern_size[0], 0:pattern_size[1]].T.reshape(-1, 2)
objp = objp * square_size

objpoints = []  # 3D points in real-world space.
imgpoints = []  # 2D points in image plane.

images = glob.glob('calibrationImages/*.jpg')

# loop over the images and find the chessboard corners.
for fname in images:
    img = cv2.imread(fname)
    if img is None:
        print("Could not load image:", fname)
        continue
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # find the chessboard corners (inner corners)
    ret, corners = cv2.findChessboardCorners(gray, pattern_size, None)
    
    if ret:
        criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
        corners_refined = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
        
        objpoints.append(objp)
        imgpoints.append(corners_refined)
        
        cv2.drawChessboardCorners(img, pattern_size, corners_refined, ret)
        cv2.imshow('Chessboard Corners', img)
        cv2.waitKey(500)
    else:
        print("Chessboard corners not found in:", fname)

cv2.destroyAllWindows()

# obtain intrinsic parameters and distortion coefficients.
ret, camera_matrix, dist_coeffs, rvecs, tvecs = cv2.calibrateCamera(
    objpoints, imgpoints, gray.shape[::-1], None, None
)

print("Camera matrix (intrinsic parameters):")
print(camera_matrix)
print("\nDistortion coefficients:")
print(dist_coeffs)

# test the accuracy of the calibration
mean_error = 0
for i in range(len(objpoints)):
    imgpoints2, _ = cv2.projectPoints(objpoints[i], rvecs[i], tvecs[i], camera_matrix, dist_coeffs)
    error = cv2.norm(imgpoints[i], imgpoints2, cv2.NORM_L2) / len(imgpoints2)
    mean_error += error
print("Reprojection Error (<0.5 is great accuracy):", mean_error / len(objpoints))
