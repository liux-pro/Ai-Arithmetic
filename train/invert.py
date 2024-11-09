import cv2
import os


def invert_images_in_directory(src_directory, dst_directory):
    # 确保目标目录存在
    if not os.path.exists(dst_directory):
        os.makedirs(dst_directory)

    # 遍历源目录及其子目录
    for root, dirs, files in os.walk(src_directory):
        for file in files:
            # 检查文件扩展名
            if file.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.tiff')):
                # 构造完整文件路径
                file_path = os.path.join(root, file)

                # 构造目标文件路径，保持目录结构
                relative_path = os.path.relpath(root, src_directory)
                dst_dir = os.path.join(dst_directory, relative_path)
                if not os.path.exists(dst_dir):
                    os.makedirs(dst_dir)
                inverted_file_path = os.path.join(dst_dir, 'inverted_' + file)

                # 读取图像
                image = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)

                if image is not None:
                    # 反转颜色
                    inverted_image = 255 - image

                    # 保存反转后的图像
                    cv2.imwrite(inverted_file_path, inverted_image)
                    print(f'Saved inverted image to: {inverted_file_path}')
                else:
                    print(f'Failed to read image: {file_path}')


# 使用示例
src_directory_path = 'extend'
dst_directory_path = 'inverted_images'
invert_images_in_directory(src_directory_path, dst_directory_path)
