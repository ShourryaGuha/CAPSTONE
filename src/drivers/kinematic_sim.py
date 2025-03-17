import numpy as np
import scipy.interpolate

### ✅ Transformation Matrix Function
def T_imu_to_G(yaw, pitch, roll):
    yaw, pitch, roll = np.radians([yaw, pitch, roll])

    R_z = np.array([
        [np.cos(yaw), -np.sin(yaw), 0],
        [np.sin(yaw), np.cos(yaw), 0],
        [0, 0, 1]
    ])
    R_y = np.array([
        [np.cos(pitch), 0, np.sin(pitch)],
        [0, 1, 0],
        [-np.sin(pitch), 0, np.cos(pitch)]
    ])
    R_x = np.array([
        [1, 0, 0],
        [0, np.cos(roll), -np.sin(roll)],
        [0, np.sin(roll), np.cos(roll)]
    ])

    return R_z @ R_y @ R_x


### ✅ Payload Kinematics Function
def payload_lin_a_v(a_imu_G, v_imu_G, omega_drone, alpha_drone, r_drone, yaw, pitch, roll):
    T_imu_G = T_imu_to_G(yaw, pitch, roll)

    a_payload_drone = alpha_drone @ np.cross(r_drone, alpha_drone) + np.cross(omega_drone, np.cross(omega_drone, r_drone))
    v_payload_drone = np.cross(omega_drone, r_drone)

    a_payload_G = a_imu_G + T_imu_G @ a_payload_drone
    v_payload_G = v_imu_G + T_imu_G @ v_payload_drone

    return a_payload_G, v_payload_G


### ✅ Freefall Simulation
def simulate_freefall(h0, v0, a0, m, g, v_table, Fd_table, dt=0.01):
    r = np.array([0.0, 0.0, h0])
    v = np.array(v0)
    a = np.array(a0)

    while r[2] > 0:
        Fd_x = np.interp(v[0], v_table, Fd_table)
        Fd_y = np.interp(v[1], v_table, Fd_table)
        Fd_z = np.interp(v[2], v_table, Fd_table)

        a[0] = Fd_x / m
        a[1] = Fd_y / m
        a[2] = -g + (Fd_z / m)

        v += a * dt
        r += v * dt

    return r  # Final position at impact


### ✅ Inverse Kinematics for Gimbal
def forward_kinematics(theta1, theta2, d):
    """Computes the position of the gimbal end-effector given joint angles and extension."""
    alpha = np.radians(theta1 + 90)
    beta = np.radians(theta2 - 90)

    x_FK = 1.16 * np.sin(alpha) - 15.3 * np.cos(alpha) + 63.06 * np.cos(alpha) * np.cos(beta) - np.cos(alpha) * np.sin(beta) * (d + 42)
    y_FK = 63.06 * np.cos(beta) * np.sin(alpha) - 15.3 * np.sin(alpha) - 1.16 * np.cos(alpha) - np.sin(alpha) * np.sin(beta) * (d + 42)
    z_FK = 63.06 * np.sin(beta) + np.cos(beta) * (d + 42) - 58.32

    return np.array([x_FK, y_FK, z_FK])


def calculate_jacobian(theta1, theta2, d):
    """Computes the Jacobian matrix using numerical differentiation."""
    delta = 1e-6
    J = np.zeros((3, 3))

    for i, param in enumerate([theta1, theta2, d]):
        params = [theta1, theta2, d]
        params[i] += delta
        pos1 = forward_kinematics(*params)
        params[i] -= 2 * delta
        pos2 = forward_kinematics(*params)
        J[:, i] = (pos1 - pos2) / (2 * delta)

    return J


def inverse_kinematics_jacobian(x0, y0, z0, initial_guess=(0, 0, 0)):
    """Solves for the gimbal angles and extension using the Jacobian method."""
    theta1, theta2, d = initial_guess
    max_iterations = 100
    tolerance = 1e-6
    damping_factor = 0.5

    for _ in range(max_iterations):
        pos_current = forward_kinematics(theta1, theta2, d)
        error = np.array([x0, y0, z0]) - pos_current

        if np.linalg.norm(error) < tolerance:
            break

        J = calculate_jacobian(theta1, theta2, d)
        J_pseudo_inv = np.linalg.pinv(J + damping_factor * np.eye(3))
        delta_q = J_pseudo_inv @ error

        theta1 += delta_q[0]
        theta2 += delta_q[1]
        d += delta_q[2]

    return theta1, theta2, d


### ✅ Example Usage (Commented Out)
"""
def main():
    # Example input data (replace with real sensor values)
    a_imu_G = np.array([0.1, 0.2, -9.81])
    v_imu_G = np.array([1.0, 2.0, 0.0])
    omega_drone = np.array([0.1, 0.2, 0.3])
    alpha_drone = np.array([0.01, 0.02, 0.03])
    yaw, pitch, roll = 10.0, 5.0, -2.0
    h0 = 100.0  # Initial drop height
    m = 0.1
    g = 9.81

    # Compute payload acceleration & velocity
    r_drone = np.array([15.45, -162.19, 28.41])
    a_payload_G, v_payload_G = payload_lin_a_v(a_imu_G, v_imu_G, omega_drone, alpha_drone, r_drone, yaw, pitch, roll)

    # Air resistance table
    v_table = np.array([0, 5, 10, 15, 20, 25, 30])
    Fd_table = np.array([0, 1.2, 4.8, 10.8, 19.2, 30.0, 43.2])

    # Run Freefall Simulation
    impact_position = simulate_freefall(h0, v_payload_G, a_payload_G, m, g, v_table, Fd_table)
    print(f"Payload Impact Position (Global): {impact_position}")

    # Solve inverse kinematics for gimbal aiming
    theta1, theta2, d = inverse_kinematics_jacobian(impact_position[0], impact_position[1], impact_position[2])
    print(f"Gimbal Angles: θ₁ = {theta1:.2f}°, θ₂ = {theta2:.2f}°, d = {d:.2f}")

# Uncomment this if you want to test:
# main()
"""
