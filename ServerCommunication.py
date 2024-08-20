import socket
import struct

SERVER_IP = '0.0.0.0'
SERVER_PORT = 12345
BUFFER_SIZE = 1024  # Number of uint16_t samples
NUM_BYTES_TO_RECEIVE = BUFFER_SIZE * 2  # Each uint16_t is 2 bytes

def handle_client_connection(conn):
    try:
        while True:
            # Create a buffer to store incoming data
            data_buffer = bytearray()
            
            # Loop until we receive the full buffer size
            while len(data_buffer) < NUM_BYTES_TO_RECEIVE:
                packet = conn.recv(NUM_BYTES_TO_RECEIVE - len(data_buffer))
                if not packet:
                    break
                data_buffer.extend(packet)
            
            if len(data_buffer) < NUM_BYTES_TO_RECEIVE:
                print(f"Received incomplete data: {len(data_buffer)} bytes")
                break

            # Unpack the received bytes into uint16_t values
            audio_data = struct.unpack(f'{BUFFER_SIZE}H', data_buffer)
            
            # Process the received audio_data
            print(f"Received {len(audio_data)} samples: {audio_data[:10]}...")  # Print first 10 samples as a preview

    except Exception as e:
        print(f"An error occurred: {e}")

    finally:
        conn.close()
        print("Client disconnected")


def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Reuse the address
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(1)
    print(f"Server listening on {SERVER_IP}:{SERVER_PORT}")

    while True:
        try:
            conn, addr = server_socket.accept()
            print(f"Connection from {addr}")
            handle_client_connection(conn)

        except KeyboardInterrupt:
            print("Server shutting down...")
            break

        except Exception as e:
            print(f"An error occurred: {e}")

    server_socket.close()

if __name__ == "__main__":
    start_server()
