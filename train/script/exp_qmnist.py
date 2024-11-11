import os
from torchvision.datasets import QMNIST
from PIL import Image
from tqdm import tqdm

# 导出qmnist所有12万张图片

# 下载并加载训练集和测试集
qmnist_train = QMNIST("qmnist", download=True, what='train')
qmnist_test = QMNIST("qmnist", download=True, what='test')

# 创建训练集和测试集的类别文件夹
for i in range(10):
    os.makedirs(f'qmnist/all/{i}', exist_ok=True)

# 保存训练集图像
for idx in tqdm(range(len(qmnist_train)), desc='Saving Training Images'):
    image, label = qmnist_train[idx]
    image.save(f'qmnist/all/{label}/train{idx}.jpg')

# 保存测试集图像
for idx in tqdm(range(len(qmnist_test)), desc='Saving Test Images'):
    image, label = qmnist_test[idx]
    image.save(f'qmnist/all/{label}/test{idx}.jpg')
