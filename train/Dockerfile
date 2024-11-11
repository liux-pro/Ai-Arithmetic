# 使用 Python 3.9 镜像作为基础
FROM python:3.9

# 设置工作目录
WORKDIR /workspace

# 复制你的 requirements.txt 到容器内
COPY requirements.txt .

# 安装 Python 依赖
RUN pip install --no-cache-dir -r requirements.txt