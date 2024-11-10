import os
import numpy as np
from PIL import Image
from skimage import morphology, filters
import matplotlib.pyplot as plt
from tensorflow.keras.preprocessing.image import ImageDataGenerator, array_to_img, img_to_array
from tqdm import tqdm  # 导入 tqdm

def thin_image(image):
    """图像骨架化处理"""
    binary_image = image > filters.threshold_otsu(image)
    skeleton = morphology.skeletonize(binary_image)
    return (skeleton * 255).astype(np.uint8)


def binarize_image(image):
    """图像二值化处理"""
    threshold = filters.threshold_otsu(image)
    binary_image = image > threshold
    return (binary_image * 255).astype(np.uint8)


def save_augmentations(image_path, save_dir, num_images=6, prefix="aug"):
    """处理并保存增强后的图像到指定目录"""

    # 创建保存文件夹（如果不存在）
    os.makedirs(save_dir, exist_ok=True)

    # 加载并处理图像
    image = Image.open(image_path).convert('L')  # 转换为灰度图
    image = image.resize((28, 28))  # 调整大小
    x = np.array(image)

    # 图像骨架化处理
    x = thin_image(x)
    x = np.expand_dims(x, axis=-1)  # 增加一个维度，因为ImageDataGenerator期望输入为批次
    x = np.expand_dims(x, axis=0)  # 增加批次维度

    # 创建一个ImageDataGenerator实例并设置增强参数
    datagen = ImageDataGenerator(
        rotation_range=1,
        zoom_range=(0.8, 1.5),
        width_shift_range=0.1,
        height_shift_range=0.1
    )

    # 自定义生成器，应用二值化处理
    def custom_generator(generator, data):
        for batch in generator.flow(data, batch_size=1):
            binarized_batch = np.array([binarize_image(img.squeeze()) for img in batch])
            binarized_batch = np.expand_dims(binarized_batch, axis=-1)  # 增加单通道维度
            yield binarized_batch

    # 生成并保存增强后的图片
    custom_gen = custom_generator(datagen, x)
    i = 0
    while i < num_images:
        batch = next(custom_gen)
        img = Image.fromarray(batch[0].squeeze())  # 转换回PIL图像对象
        # 获取原始文件名，添加前缀，保存为BMP格式
        base_filename = os.path.basename(image_path)
        new_filename = f"{prefix}_{i + 1}_{base_filename}"
        img.save(os.path.join(save_dir, new_filename))  # 保存为BMP格式
        i += 1

    # print(f"{num_images} images have been saved to {save_dir}")


if __name__ == '__main__':
    """批量处理所有图像并保存到新目录"""

    for i in tqdm(range(14), desc="Processing directories"):  # 添加进度条，遍历目录时显示
        input_dir = f"qmnist/all/{i}"
        output_dir = f"qmnist/mnist14/{i}"

        # 确保输出目录存在
        os.makedirs(output_dir, exist_ok=True)

        # 获取目录中的所有图片
        image_files = [f for f in os.listdir(input_dir) if f.endswith('.jpg') or f.endswith('.png')]

        # 处理并保存每一张图片
        for image_file in tqdm(image_files, desc=f"Processing images in {i}"):  # 在处理图片时显示进度
            image_path = os.path.join(input_dir, image_file)
            save_augmentations(image_path, output_dir, num_images=10, prefix=f"aug_{i}")
