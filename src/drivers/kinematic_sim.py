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
def simulate_freefall(h0, v0, a0, m, dt=0.01):

    v_table = (1000/3600)*np.array([5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100])
    Fd_table = np.array([0.000734, 0.00274, 0.006, 0.0103, 0.0160, 0.022, 0.031, 0.04, 0.05, 0.0621, 0.075, 0.083, 0.098, 0.114, 0.132, 0.151, 0.171, 0.192, 0.213, 0.237])

    g = 9.81

    r = np.array([0.0, 0.0, 0.0])
    v = np.array(v0)
    a = np.array(a0)

    while r[2] < h0:
        Fd_x = np.interp(v[0], v_table, Fd_table)
        Fd_y = np.interp(v[1], v_table, Fd_table)
        Fd_z = np.interp(v[2], v_table, Fd_table)

        a[0] = -Fd_x / m
        a[1] = -Fd_y / m
        a[2] = g - (Fd_z / m)

        v += a * dt
        r += v * dt

    return r  # Final position at impact

def forward_kinematics(theta1, theta2, d):
    """Forward kinematics exactly matching the MATLAB implementation."""
    # Helper functions to match MATLAB's trig functions

    #x = 1.16*sin(theta_1 + np.pi/2) - 15.3*cos(theta_1 + np.pi/2) + 63.06*cos(theta_1 + np.pi/2)*cos(theta_2 - np.pi/2) 
    # - 1.0*cos(theta_1 + np.pi/2)*sin(theta_2 - np.pi/2)*(dist + 42.0)
    x = (1.16*np.sin(theta1 + np.pi/2) - 15.3*np.cos(theta1+np.pi/2)
         +63.06*np.sin(theta1+np.pi/2)*np.cos(theta2-np.pi/2)
         -np.cos(theta1+np.pi/2)*np.sin(theta2-np.pi/2)*(d+42))
    
    #y = 63.06*cos(theta_2 - np.pi/2)*sin(theta_1 + np.pi/2) - 15.3*sin(theta_1 + np.pi/2) - 1.16*cos(theta_1 + np.pi/2) - 
    # 1.0*sin(theta_1 + np.pi/2)*sin(theta_2 - np.pi/2)*(dist + 42.0)
    y = (63.06*np.cos(theta2-np.pi/2)*np.sin(theta1+np.pi/2)-15.3*np.sin(theta1+np.pi/2)
         -1.16*np.cos(theta1+np.pi/2)-np.sin(theta1+np.pi/2)*np.sin(theta2-np.pi/2)*(d+42))
    
    #63.06*sin(theta_2 - np.pi/2) + 1.0*cos(theta_2 - np.pi/2)*(dist + 42.0) - 58.32
    z = 63.06*np.sin(theta2-np.pi/2)+np.cos(theta2-np.pi/2)*(d+42)-58.32
    
    return np.array([x, y, z])

def calculate_jacobian(theta1, theta2, d):
    """Compute the Jacobian matrix exactly as in the MATLAB code."""
    # Helper functions to match MATLAB's trig functions
    sind = lambda angle: np.sin(np.radians(angle))
    cosd = lambda angle: np.cos(np.radians(angle))
    
    # Create the Jacobian matrix with exact expressions from MATLAB
    J = np.zeros((3, 3))
    
    # First row (exact copy from MATLAB)
    J[0, 0] = 1.16 * cosd(theta1 + 90) + 15.3 * sind(theta1 + 90) - 63.06 * sind(theta1 + 90) * cosd(theta2 - 90) + sind(theta1 + 90) * sind(theta2 - 90) * (d + 42)
    J[0, 1] = -63.06 * cosd(theta1 + 90) * sind(theta2 - 90) - cosd(theta1 + 90) * cosd(theta2 - 90) * (d + 42)
    J[0, 2] = -cosd(theta1 + 90) * sind(theta2 - 90)
    
    # Second row (exact copy from MATLAB)
    J[1, 0] = 63.06 * cosd(theta2 - 90) * cosd(theta1 + 90) - 15.3 * cosd(theta1 + 90) - 1.16 * sind(theta1 + 90) - cosd(theta1 + 90) * sind(theta2 - 90) * (d + 42)
    J[1, 1] = -63.06 * sind(theta1 + 90) * sind(theta2 - 90) - sind(theta1 + 90) * cosd(theta2 - 90) * (d + 42)
    J[1, 2] = -sind(theta1 + 90) * sind(theta2 - 90)
    
    # Third row (exact copy from MATLAB)
    J[2, 0] = 0
    J[2, 1] = 63.06 * cosd(theta2 - 90) - sind(theta2 - 90) * (d + 42)
    J[2, 2] = cosd(theta2 - 90)
    
    return J

def inverse_kinematics_jacobian(x_target, y_target, z_target, initial_guess=None):
    """Solves for the joint variables using the Jacobian method with stability enhancements."""
    # Use the same initial guess as MATLAB for consistency
    if initial_guess is None:
        theta1 = 0.0  # Initial yaw in degrees
        theta2 = 0.0  # Initial pitch in degrees
        d = 50.0      # Initial prismatic displacement in mm
    else:
        theta1, theta2, d = initial_guess
    
    # Iteration parameters exactly matching MATLAB
    max_iters = 1000
    tol = 1e-4
    alpha = 0.1  # Step size
    
    # For debugging/comparison with MATLAB
    iteration_history = []
    
    for iter in range(max_iters):
        # Forward kinematics to get current position
        current_pos = forward_kinematics(theta1, theta2, d)
        x, y, z = current_pos
        
        # Compute error
        error = np.array([x_target - x, y_target - y, z_target - z])
        
        # Check for convergence - use same norm calculation as MATLAB
        error_norm = np.linalg.norm(error)
        # if error_norm < tol:
        #     print(f'Converged in {iter+1} iterations.')
        #     break
        
        # Compute Jacobian
        J = calculate_jacobian(theta1, theta2, d)
        
        # Improved numerical stability for pseudo-inverse calculation
        # Using SVD decomposition with conditioning similar to MATLAB's pinv
        U, S, Vh = np.linalg.svd(J, full_matrices=False)
        
        # Apply conditioning similar to MATLAB's pinv (handles small singular values)
        rcond = np.finfo(float).eps * max(J.shape) * np.max(S)
        S_inv = np.array([1/s if s > rcond else 0 for s in S])
        
        # Calculate pseudo-inverse
        J_pinv = (Vh.T * S_inv) @ U.T
        
        # Compute update
        delta = alpha * J_pinv @ error
        
        # Update joint variables
        theta1_new = theta1 + delta[0]
        theta2_new = theta2 + delta[1]
        d_new = d + delta[2]
        
        # Store for debugging
        iteration_history.append({
            'iter': iter+1,
            'theta1': theta1_new,
            'theta2': theta2_new,
            'd': d_new,
            'error': error_norm
        })
        
        # Update variables
        theta1 = theta1_new
        theta2 = theta2_new
        d = d_new
        
        # Print progress to match MATLAB output
        # print(f'Iteration {iter+1}: theta1 = {theta1:.2f}, theta2 = {theta2:.2f}, d = {d:.2f}')
    
    # # Final solution
    # print('\nFinal Solution:')
    # print(f'theta1 = {theta1:.2f} degrees')
    # print(f'theta2 = {theta2:.2f} degrees')
    # print(f'd = {d:.2f} mm')
    
    return theta1, theta2, d, iteration_history

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
