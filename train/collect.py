import serial
import numpy as np
from PIL import Image
import os

# 设置串口
ser = serial.Serial('COM9', 9600, timeout=1)  # 根据实际串口号修改

# 设置图像的大小和通道
height, width = 28, 28

# 设置文件保存路径
save_path = './dataset/mymnist14/4/'

# 获取最大文件索引
def get_next_file_index():
    existing_files = os.listdir(save_path)
    existing_numbers = []
    for file in existing_files:
        if file.endswith('.png'):
            try:
                number = int(file.split('_')[1].split('.')[0])
                existing_numbers.append(number)
            except ValueError:
                pass
    return max(existing_numbers, default=-1) + 1

def receive_image():
    pixels = []

    # 读取图像数据
    while True:
        data = ser.readline().decode('utf-8').strip()
        print(f"Received data: {data}")

        # 读取像素数据，直到收到 '@'
        if data == '@':
            print("End of image transmission")
            break

        if data != '':
            split = data.split(',')
            print(f"Split data: {split}")
            split = [x for x in split if x]  # 移除空元素
            pixels.extend(map(int, split))

    # 确保接收到正确数量的像素
    if len(pixels) == height * width:
        image_array = np.array(pixels, dtype=np.uint8).reshape((height, width))

        # 获取下一个文件索引
        file_index = get_next_file_index()

        # 生成文件名并保存图片
        file_name = f"{save_path}image_{file_index}.png"
        img = Image.fromarray(image_array)
        img.save(file_name)
        print(f"Image saved as '{file_name}'")
    else:
        print(f"Error: Received {len(pixels)} pixels, expected {height * width}.")


while True:
    receive_image()
