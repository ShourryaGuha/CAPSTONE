import numpy as np
import data_logger  # Import existing functions
from kinematic_sim import payload_lin_a_v, simulate_freefall, forward_kinematics, inverse_kinematics_jacobian  # Replace with actual module names

def check_payload_kinematics():
    """Sanity check for payload velocity and acceleration calculations."""
    a_imu_G = np.array([0.1, 0.2, -9.81])
    v_imu_G = np.array([1.0, 2.0, 0.0])
    omega_drone = np.array([0.1, 0.2, 0.3])
    alpha_drone = np.array([0.01, 0.02, 0.03])
    r_drone = (1/1000)*np.array([15.45, -162.19, 28.41])
    yaw, pitch, roll = 10.0, 5.0, -2.0

    a_payload_G, v_payload_G = payload_lin_a_v(a_imu_G, v_imu_G, omega_drone, alpha_drone, r_drone, yaw, pitch, roll)
    
    print("Payload Acceleration (Global):", a_payload_G)
    print("Payload Velocity (Global):", v_payload_G)
    if np.linalg.norm(a_payload_G) < 9.0 or np.linalg.norm(a_payload_G) > 10:
        print("WARNING: Payload acceleration magnitude is unrealistic!")


def check_freefall_simulation():
    """Sanity check for freefall physics."""
    h0 = 25.0  
    v0 = [8.3, 2.0, 1.0]  # Moving forward and slightly upward  
    a0 = [0.0, 0.0, -9.81]  # Gravity pulls it down  
    m, g = 0.1, 9.81
    v_table = (1000/3600)*np.array([5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100])
    Fd_table = np.array([0.000734, 0.00274, 0.006, 0.0103, 0.0160, 0.022, 0.031, 0.04, 0.05, 0.0621, 0.075, 0.083, 0.098, 0.114, 0.132, 0.151, 0.171, 0.192, 0.213, 0.237])
    final_position = simulate_freefall(h0, v0, a0, m, g, v_table, Fd_table)
    print("Final Payload Impact Position:", final_position)
    if final_position[2] > 0:
        print("ERROR: Payload should have hit the ground (z=0)!")


def check_gimbal_inverse_kinematics():
    """Sanity check for gimbal angle calculations."""
    target_pos = (20.2294892, 4.76572681, -0.05411896)
    solution = inverse_kinematics_jacobian(*target_pos)
    x_fk, y_fk, z_fk = forward_kinematics(solution['theta1'], solution['theta2'], solution['d'])
    error = np.linalg.norm(np.array(target_pos) - np.array([x_fk, y_fk, z_fk]))
    print("Gimbal IK Error:", error)
    if error > 0.01:
        print("WARNING: Gimbal inverse kinematics is inaccurate!")


def run_all_sanity_checks():
    print("Running sanity checks...")
    check_payload_kinematics()
    check_freefall_simulation()
    check_gimbal_inverse_kinematics()
    print("Sanity checks complete.")

if __name__ == "__main__":
    run_all_sanity_checks()
