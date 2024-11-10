import numpy as np
from skimage import morphology
from skimage import filters
from skimage import io
from skimage.util import invert
from tensorflow.keras.datasets import mnist
import matplotlib.pyplot as plt

# 加载MNIST数据集
(train_images, train_labels), (test_images, test_labels) = mnist.load_data()


def thin_image(image):
    # 将图像转换为二值图像
    binary_image = image > filters.threshold_otsu(image)

    # 进行骨架化处理
    skeleton = morphology.skeletonize(binary_image)

    # 将骨架化后的图像转换回0-255的范围
    thin_image = (skeleton * 255).astype(np.uint8)

    return thin_image


# 选择一个MNIST图像进行处理
sample_image = train_images[0]

# 对图像进行细化处理
thinned_image = thin_image(sample_image)

# 显示原始图像和细化后的图像
fig, axes = plt.subplots(1, 2, figsize=(8, 4))
ax = axes.ravel()

ax[0].imshow(sample_image, cmap=plt.cm.gray)
ax[0].set_title('Original Image')

ax[1].imshow(thinned_image, cmap=plt.cm.gray)
ax[1].set_title('Thinned Image')

for a in ax:
    a.axis('off')

plt.tight_layout()
plt.show()
