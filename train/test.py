import os
import numpy as np
from PIL import Image
from skimage import morphology, filters
import matplotlib.pyplot as plt
from tensorflow.keras.preprocessing.image import ImageDataGenerator, array_to_img, img_to_array

def thin_image(image):
    # 将图像转换为二值图像
    binary_image = image > filters.threshold_otsu(image)
    # 进行骨架化处理
    skeleton = morphology.skeletonize(binary_image)
    # 将骨架化后的图像转换回0-255的范围
    thin_image = (skeleton * 255).astype(np.uint8)
    return thin_image

def binarize_image(image):
    threshold = filters.threshold_otsu(image)
    binary_image = image > threshold
    return (binary_image * 255).astype(np.uint8)


# 加载并处理图像
image_path = 'qmnist/all/8/test232.jpg'
image = Image.open(image_path).convert('L')  # 转换为灰度图
image = image.resize((28, 28))  # 调整大小
x = np.array(image)
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

# 生成并显示增强后的图片
custom_gen = custom_generator(datagen, x)
i = 0
fig, ax = plt.subplots(2, 3, figsize=(12, 8))  # 创建一个2x3的子图
for batch in custom_gen:
    ax[i // 3, i % 3].imshow(batch[0].squeeze(), cmap='gray')
    ax[i // 3, i % 3].axis('off')
    i += 1
    if i % 6 == 0:
        break  # 只显示6张增强后的图片

plt.tight_layout()
plt.show()
