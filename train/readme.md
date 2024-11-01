# mnist数字识别卷积神经网络
tensorflow训练，然后TinyMaix提供了脚本，自动把tensorflow转tflite，再转TinyMaix的tmdl格式，最终生成c数组头头文件。
# 数据集
数据集为qmnist，它是mnist的超集，包含12万张手写数字图片。  
图片为28x28，其为黑底白字，8位灰度图，数字居中，四边留有4像素空白，实际有效的数据在中间20x20内。  
[exp_qmnist.py](exp_qmnist.py)能把这些图片导出为jpg格式文件。
# 训练
网络较小，训练很快，不需要GPU，CPU也能在10分钟内完成。
## 使用Windows训练
使用 python3.9，使用venv避免干扰
```bash
pip install -r requirements.txt #安装依赖
python exp_qmnist.py #生成数据集
jupyter nbconvert --to notebook --execute good-mnist.ipynb #训练并转换
```
## 使用docker训练
创建容器
```bash
docker run -it --rm -v .:/workspace python:3.9 bash
```

```bash
cd workspace
pip install -i https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple -r requirements.txt
python exp_qmnist.py #生成数据集
jupyter nbconvert --to notebook --execute good-mnist.ipynb
```
# 参考资料
https://github.com/sipeed/TinyMaix
https://www.ccom.ucsd.edu/~cdeotte/programs/MNIST.html
https://www.kaggle.com/cdeotte/mnist-neural-network-coded-in-c-0-985
https://www.kaggle.com/cdeotte/25-million-images-0-99757-mnist

