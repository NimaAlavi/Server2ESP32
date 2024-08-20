import socket
import struct
import numpy as np
from detectionProcess import DetectionProcessTransFormers
from audioCommandDetection import CommandDetection

s2t = DetectionProcessTransFormers()
c2d = CommandDetection()

SERVER_IP = '0.0.0.0'
SERVER_PORT = 13579
BUFFER_SIZE = 1024  # Increased buffer size for efficiency
NUM_BYTES_TO_RECEIVE = BUFFER_SIZE * 2
TARGET_SAMPLES = 48000

def handle_client_connection(conn):
    try:
        accumulated_data = np.zeros(TARGET_SAMPLES, dtype=np.uint16)
        current_position = 0

        while current_position < TARGET_SAMPLES:
            data_buffer = bytearray()
            while len(data_buffer) < NUM_BYTES_TO_RECEIVE:
                packet = conn.recv(NUM_BYTES_TO_RECEIVE - len(data_buffer))
                if not packet:
                    print("Connection lost")
                    return  # Exit if the connection is closed
                data_buffer.extend(packet)

            audio_data = np.frombuffer(data_buffer, dtype=np.uint16)
            remaining_space = TARGET_SAMPLES - current_position
            chunk_size = min(len(audio_data), remaining_space)

            accumulated_data[current_position:current_position + chunk_size] = audio_data[:chunk_size]
            current_position += chunk_size

        if current_position >= TARGET_SAMPLES:
            print(f"Received {current_position} samples.")
            result_text = s2t.doProcess(accumulated_data / 2**12)
            DataOutB = c2d.proceedCode(result_text)
            print("Code Out is: ", DataOutB)

            # Send the result text back to the ESP32
            conn.sendall(DataOutB.encode('utf-8'))  # Send the text as a UTF-8 encoded string

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
