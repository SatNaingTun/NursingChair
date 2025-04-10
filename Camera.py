import cv2
import numpy as np
import tflite_runtime.interpreter as tflite
from picamera2 import Picamera2
from PIL import Image
import time

# Load labels
with open("labels.txt", "r") as f:
    labels = [line.strip() for line in f.readlines()]

# Load TFLite model
interpreter = tflite.Interpreter(model_path="my_train_model.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
input_shape = input_details[0]['shape']
height, width = input_shape[1], input_shape[2]

# Initialize PiCamera2
picam2 = Picamera2()
picam2.preview_configuration.main.size = (640, 480)
picam2.preview_configuration.main.format = "RGB888"
picam2.configure("preview")
picam2.start()
time.sleep(2)

# Real-time loop
while True:
    frame = picam2.capture_array()
    img = Image.fromarray(frame).resize((width, height))
    input_data = np.expand_dims(np.asarray(img, dtype=np.float32) / 255.0, axis=0)

    # Set tensor and run inference
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    output_data = interpreter.get_tensor(output_details[0]['index'])

    predicted_index = np.argmax(output_data)
    predicted_label = labels[predicted_index]

    # Draw label on frame
    cv2.putText(frame, predicted_label, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    cv2.imshow("Real-Time Detection", frame)

    # Exit on keypress
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()