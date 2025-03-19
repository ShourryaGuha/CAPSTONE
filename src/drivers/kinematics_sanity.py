import numpy as np
import data_logger  # Import existing functions
from kinematic_sim import payload_lin_a_v, simulate_freefall, forward_kinematics, inverse_kinematics_jacobian  # Replace with actual module names

def check_payload_kinematics():
    """Sanity check for payload velocity and acceleration calculations."""
    a_imu_G = np.array([0.0, 0.0, -9.81])
    v_imu_G = np.array([8.0, 0.0, 0.0])
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
    v0 = [8.0, 0.0, 0.0]  # Moving forward and slightly upward  
    a0 = [0.0, 0.0, 9.81]  # Gravity pulls it down  
    m, g = 0.1, 9.81
    final_position = simulate_freefall(h0=h0, v0=v0, a0=a0, m=m)
    print("Final Payload Impact Position:", final_position)
    if final_position[2] < h0*0.9:
        print("ERROR: Payload should have hit the ground (z=0)!")


def check_FK():
    theta1 = 0
    theta2 = 0
    d = 0
    [x,y,z] = forward_kinematics(theta1, theta2, d)

    print("x:", x, "y:", y, "z:", z)

    if not(1.15 < x < 1.17):
        print("x wrong", x)
    if not(26.6 < y < 27.0):
        print("y wrong", y)
    if not(-121 > z > -122):
        print("z wrong", z)


def check_gimbal_inverse_kinematics():
    """Sanity check for gimbal angle calculations."""
    target_pos = (100.00, 50.00, 150.00)
    solution = inverse_kinematics_jacobian(*target_pos, initial_guess=[0,0,0])
    print("theta_yaw:", solution[0], "theta_pitch:", solution[1], "dist:", solution[2])
    x_fk, y_fk, z_fk = forward_kinematics(solution[0], solution[1], solution[2])
    error = np.linalg.norm(np.array(target_pos) - np.array([x_fk, y_fk, z_fk]))
    print("Gimbal IK Error:", error)
    if error > 0.01:
        print("WARNING: Gimbal inverse kinematics is inaccurate!")


def run_all_sanity_checks():
    print("Running sanity checks...")
    check_payload_kinematics()
    check_freefall_simulation()
    check_FK()
    # check_gimbal_inverse_kinematics()
    print("Sanity checks complete.")

if __name__ == "__main__":
    run_all_sanity_checks()
