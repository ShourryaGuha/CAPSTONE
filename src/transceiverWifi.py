import socket

# Set up the server socket
server_ip = "192.168.223.92"  # Listen on all available network interfaces
server_port = 5000      # Choose an appropriate port

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((server_ip, server_port))
server_socket.listen(1)

print(f"Waiting for connection on {server_ip}:{server_port}...")

# Accept the connection from the client (laptop)
client_socket, client_address = server_socket.accept()
print(f"Connection established with {client_address}")

# Send and receive data
while True:
    data_to_send = "Hello from Raspberry Pi!"
    client_socket.sendall(data_to_send.encode())  # Send data

    data_received = client_socket.recv(1024).decode()  # Receive data from laptop
    print(f"Received from laptop: {data_received}")

    # Break the loop if needed (e.g., a termination condition)
    if data_received == "exit":
        print("Closing connection")
        break

# Close the socket
client_socket.close()
server_socket.close()
