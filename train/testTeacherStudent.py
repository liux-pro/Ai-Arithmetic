import tensorflow as tf
from tensorflow.keras import layers, models
from tensorflow.keras.datasets import mnist
import numpy as np
from tensorflow import keras
import numpy as np
from keras.datasets import mnist
from tensorflow.python.keras.backend import set_session
from tensorflow.python.keras.models import load_model
from tensorflow.keras.models import Model, load_model, Sequential
from tensorflow.keras.layers import Input,Conv2D, Dense, MaxPooling2D, Softmax, Activation, BatchNormalization, Flatten, Dropout, DepthwiseConv2D
from tensorflow.keras.layers import MaxPool2D, AvgPool2D, AveragePooling2D, GlobalAveragePooling2D,ZeroPadding2D,Input,Embedding,PReLU,Reshape
from tensorflow import keras
from keras.callbacks import ModelCheckpoint
from keras.callbacks import TensorBoard
from keras.utils import to_categorical
import keras.backend as K
import tensorflow as tf
import time
from keras.utils import to_categorical

import os
import numpy as np
from sklearn.model_selection import train_test_split
from PIL import Image
from tensorflow.keras.preprocessing.image import ImageDataGenerator
import random

# # 定义数据集路径
# data_folder = 'qmnist/all'
# x_data = []
# y_data = []
#
# # 加载图像和标签
# for label in range(10):
#     label_folder = os.path.join(data_folder, str(label))
#     images = []
#
#     for img_name in os.listdir(label_folder):
#         img_path = os.path.join(label_folder, img_name)
#
#         # 加载并处理图像
#         image = Image.open(img_path).convert('L')  # 转换为灰度图
#         image = image.resize((28, 28))  # 调整大小
#         image_array = np.array(image)
#         images.append(image_array)
#
#     x_data.extend(images)
#     y_data.extend([label] * len(images))
#
# # 转换为NumPy数组
# x_data = np.array(x_data)
# y_data = np.array(y_data)
#
# # 直接使用所有样本
# x_balanced = x_data
# y_balanced = y_data
#
# # 按比例拆分数据集（例如，80% 训练集，20% 测试集）
# x_train, x_test, y_train, y_test = train_test_split(x_balanced, y_balanced, test_size=0.2, random_state=42)
#
# # 确保数据集的形状符合TensorFlow的要求
# # x_train = x_train.reshape(-1, 28, 28, 1).astype('float32')
# # x_test = x_test.reshape(-1, 28, 28, 1).astype('float32')
#
# # # CREATE MORE IMAGES VIA DATA AUGMENTATION
# # datagen = ImageDataGenerator(
# #     # rotation_range=10,
# #     zoom_range=(0.9, 1.1),
# #     width_shift_range=0.1,
# #     height_shift_range=0.1)
# # # 适配数据生成器
# # datagen.fit(x_train)
#
# # 打印每种图片的数量
# unique, counts = np.unique(y_balanced, return_counts=True)
# print("每种图片数量:")
# for label, count in zip(unique, counts):
#     print(f'类别 {label}: {count} 张')
#
# # x_train = x_train.reshape(x_train.shape[0],x_train.shape[1],x_train.shape[2],1)/255
# # x_test = x_test.reshape(x_test.shape[0],x_test.shape[1],x_test.shape[2],1)/255
# #
# # y_train = to_categorical(y_train,num_classes=10)
# # y_test = to_categorical(y_test,num_classes=10)


# 加载 MNIST 数据集
(x_train, y_train), (x_test, y_test) = mnist.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0

# 添加通道维度
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)

# 将标签转换为 one-hot 编码
y_train_one_hot = tf.keras.utils.to_categorical(y_train, 10)
y_test_one_hot = tf.keras.utils.to_categorical(y_test, 10)

# # 定义教师模型
# teacher_model = models.Sequential([
#     layers.Conv2D(32, (3, 3), activation='relu', input_shape=(28, 28, 1)),
#     layers.MaxPooling2D((2, 2)),
#     layers.Conv2D(64, (3, 3), activation='relu'),
#     layers.MaxPooling2D((2, 2)),
#     layers.Conv2D(64, (3, 3), activation='relu'),
#     layers.Flatten(),
#     layers.Dense(64, activation='relu'),
#     layers.Dense(10)
# ])
def init_model(dim0):
    inputs = Input(shape=(28, 28, 1))
    x = Conv2D(dim0, (3, 3), padding='same', strides=(2, 2), name='ftr0')(inputs)
    x = BatchNormalization(name="bn0")(x)
    x = Activation('relu', name="relu0")(x)

    x = Conv2D(dim0 * 2, (3, 3), padding='same', strides=(2, 2), name='ftr1')(x)
    x = BatchNormalization(name="bn1")(x)
    x = Activation('relu', name="relu1")(x)
    res = x

    x = Conv2D(dim0 * 2, (3, 3), padding='same', name='ftr2')(x)
    x = BatchNormalization(name="bn2")(x)

    x = res + x

    x = Conv2D(dim0 * 4, (3, 3), padding='valid', strides=(2, 2), name='ftr3')(x)
    x = Flatten(name='reshape')(x)
    x = Dense(10, name="fc1")(x)

    x = Activation('softmax', name="sm")(x)
    model = Model(inputs=inputs, outputs=x)

    return model

# 设置参数并初始化模型
teacher_model = init_model(10)
teacher_model.summary()



# 编译和训练教师模型
teacher_model.compile(optimizer='adam',
                      loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True),
                      metrics=['accuracy'])
teacher_history = teacher_model.fit(x_train, y_train_one_hot, epochs=10, batch_size=64, validation_split=0.2)

# 教师模型的预测（软标签）
teacher_logits = teacher_model.predict(x_train)
soft_labels = tf.nn.softmax(teacher_logits / 10.0).numpy()  # 温度参数设置为10

# # 定义学生模型
# student_model = models.Sequential([
#     layers.Conv2D(8, (3, 3), activation='relu', input_shape=(28, 28, 1)),
#     layers.MaxPooling2D((2, 2)),
#     layers.Conv2D(16, (3, 3), activation='relu'),
#     layers.MaxPooling2D((2, 2)),
#     layers.Flatten(),
#     layers.Dense(16, activation='relu'),
#     layers.Dense(10)
# ])
student_model = init_model(8)
student_model.summary()

# 编译学生模型
student_model.compile(optimizer='adam',
                      loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True),
                      metrics=['accuracy'])

# 训练学生模型（使用软标签）
student_history = student_model.fit(x_train, soft_labels, epochs=10, batch_size=64, validation_split=0.2)

# 评估教师模型和学生模型在测试集上的表现
teacher_loss, teacher_acc = teacher_model.evaluate(x_test, y_test_one_hot)
student_loss, student_acc = student_model.evaluate(x_test, y_test_one_hot)

# 输出教师模型和学生模型的准确度
print(f'Teacher model accuracy on validation set: {teacher_history.history["val_accuracy"][-1]}')
print(f'Teacher model accuracy on test set: {teacher_acc}')
print(f'Student model accuracy on validation set: {student_history.history["val_accuracy"][-1]}')
print(f'Student model accuracy on test set: {student_acc}')
