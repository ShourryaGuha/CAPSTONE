import os
import numpy as np
from typing import Tuple
import cv2

class SensorToVideoTransformer:
    def __init__(self, camera_matrix: np.array, dist_coeffs: np.array):
        self.camera_matrix = camera_matrix
        self.dist_coeffs = dist_coeffs
        
 
    def transform_sensor_to_image(self, distance: float, pitch: float, yaw: float):
        """
        Notes:
        Positive Pitch -> Tilts downwards
        Positive Yaw -> Tilts Counterclockwise about Z axis
        """
        sensor_point = np.array([
            [distance],
            [0],
            [0],
        ])

        rotation_matrix_yaw = np.array([
            [np.cos(yaw), -np.sin(yaw), 0],
            [np.sin(yaw), np.cos(yaw), 0],
            [0, 0, 1]
        ])

        rotation_matrix_pitch = np.array([
            [np.cos(pitch), 0, np.sin(pitch)],
            [0, 1, 0],
            [-np.sin(pitch), 0, np.cos(pitch)]
        ])

        # --- Determine distance to camera here ---
        camera_to_sensor_distance = np.array([
            [-0.05],
            [-0.03],
            [-0.05],
        ])

        sensor_to_camera_rotation_matrix = rotation_matrix_yaw @ rotation_matrix_pitch

        camera_point = sensor_to_camera_rotation_matrix @ sensor_point + camera_to_sensor_distance

        print(f"Camera Point: {camera_point}")

        return self.transfrom_to_image_plane(camera_point)
    
    def transfrom_to_image_plane(self, camera_point):

        # Adjust camera point to correspond to correct x and y on image plane
        adjusted_camera_point = np.array([
            camera_point[1],
            camera_point[2],
            camera_point[0]
        ])

        img_coords = (1/camera_point[0]) * self.camera_matrix @ adjusted_camera_point

        return img_coords

    def create_demo_image(self):
        # create a blank white image
        image = np.ones((720, 1280, 3), dtype=np.uint8) * 255

        sensor_distance = 25

        index = 0
        labels = ["-5 Deg", "-10 Deg", "-15 Deg", "-20 Deg", "5 Deg", "10 Deg", "15 Deg", "20 Deg"]
        for pitch in [np.pi/36, np.pi/18, np.pi/12, np.pi/9, -np.pi/36, -np.pi/18, -np.pi/12, -np.pi/9]: 
            img_coords = self.transform_sensor_to_image(sensor_distance, pitch, 0)
            u, v = img_coords[0], img_coords[1]
            cv2.line(image, (0, 720 - int(v)), (1280, 720 - int(v)), (0, 0, 0), 2) 
            label = f"Pitch: {labels[index]}"
            index = index + 1
            cv2.putText(image, label, (int(u) - 50, 720 - int(v) - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)


        index = 0
        labels = ["10 Deg", "20 Deg", "30 Deg", "-10 Deg", "-20 Deg", "-30 Deg"]
        for yaw in [np.pi/18, np.pi/9, np.pi/6, -np.pi/18, -np.pi/9, -np.pi/6]: 
            img_coords = self.transform_sensor_to_image(sensor_distance, 0, yaw)
            u, v = img_coords[0], img_coords[1]
            cv2.line(image, (1280 - int(u), 0), (1280 - int(u), 720), (0, 0, 255), 2) 
            label = f"Yaw: {labels[index]}"
            index = index + 1
            cv2.putText(image, label, ( (1280 - int(u)) - 120, 720 - int(v) - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)
        
        img_coords = self.transform_sensor_to_image(25, np.pi/18, np.pi/18)
        u, v = img_coords[0], img_coords[1]

        # save image to downloads
        # downloads_path = os.path.join(os.path.expanduser("~"), "Downloads")
        # output_path = os.path.join(downloads_path, "output_image.png")
        # cv2.imwrite(output_path, image)

        # display the image
        cv2.imshow('Labeled Image', image)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
        

        

if __name__ == "__main__":
    # assuming a 1280x720 resolution for simplicity
    f_x = 800  # focal length in x direction
    f_y = 800  # focal length in y direction
    c_x = 640  # principal point x (center of image)
    c_y = 360  # principal point y (center of image)

    # intrinsic matrix
    camera_matrix = np.array([[f_x, 0, c_x],
                [0, f_y, c_y],
                [0, 0, 1]])

    # distortion coefficients (k1, k2, p1, p2, k3)
    # NOT USED YET
    dist_coeffs = np.array([-0.2, 0.1, 0.0, 0.0, 0.01])

    # distance sensor readings
    sensor_distance = 25
    sensor_yaw = -np.pi/20
    sensor_pitch = -np.pi/20

    transformer = SensorToVideoTransformer(camera_matrix, dist_coeffs)

    # transformer.transform_sensor_to_image(sensor_distance, sensor_pitch, sensor_yaw)
    transformer.create_demo_image()
    # transformer.transform_sensor_to_image(25,0,np.pi/6)
