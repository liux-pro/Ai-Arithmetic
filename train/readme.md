# mnist数字识别卷积神经网络
tensorflow训练，然后TinyMaix提供了脚本，自动把tensorflow转tflite，再转TinyMaix的tmdl格式，最终生成c数组头头文件。
# 数据集
数据集包括qmnist，它是mnist的超集，包含12万张手写数字图片。  
图片为28x28，其为黑底白字，8位灰度图，数字居中，四边留有4像素空白，实际有效的数据在中间20x20内。  
[exp_qmnist.py](script/exp_qmnist.py)能把这些图片导出为jpg格式文件。
除此之外，加减乘除四种图片，从[MNIST-Calculator-Handwriting-recognition](https://github.com/YoujiaZhang/MNIST-Calculator-Handwriting-recognition/blob/master/models/cfs.tar.xz)
项目下载，使用[invert.py](script%2Finvert.py)将其黑白翻转，并入数据集。  
最终打包这些图片，可以从 https://file.liux.pro/mnist14.zip 下载
# 训练
网络较小，训练很快，不需要GPU，CPU也能在10分钟内完成。
## 使用Windows训练
使用 python3.9，使用venv避免干扰
```bash
pip install -r requirements.txt #安装依赖
papermill -k python --log-output good-mnist.ipynb output.ipynb
```
## 使用docker训练
构建镜像
```bash
docker build train -t tensorflow
```

```bash
docker run --rm -v $(pwd):/workspace -w /workspace tensorflow \
          bash -c "papermill -k python --log-output good-mnist.ipynb output.ipynb"
```
# 参考资料
https://github.com/sipeed/TinyMaix
https://www.ccom.ucsd.edu/~cdeotte/programs/MNIST.html
https://www.kaggle.com/cdeotte/mnist-neural-network-coded-in-c-0-985
https://www.kaggle.com/cdeotte/25-million-images-0-99757-mnist
https://github.com/YoujiaZhang/MNIST-Calculator-Handwriting-recognition/
