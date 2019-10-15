from flask import Flask, request
from flask import Flask, request
from aws_confg import *
from aws_gw import aws_gw
from file_gw import file_gw
import os
import json
import math
import os

app = Flask(__name__)


@app.route('/')
def hello_world():
    return 'Hello World!'


@app.route('/wrg_vtk/', methods=['GET'])
def wrg_vtk():
    # 接受参数
    path_wrg = request.args.get('path_wrg')
    path_vtk = request.args.get('path_vtk')
    print(path_wrg)
    print(path_vtk)

    # 连接AWS
    aws = aws_gw(AWS_ACCESS, AWS_SECRET)

    # 从s3下载wrg
    bucket = 'goldfoam2'
    key, file_wrg = os.path.split(path_wrg)
    folder_local = 'C:/Users/Administrator/Desktop/wrg_vtk'
    path_list = []
    path_list.append(file_wrg)
    print("download wrg")
    res = aws.s3_download(bucket, key, folder_local, path_list)
    if res == 0:
        return str(1)

    # wrg to vtk
    print("wrg to vtk......")
    print(file_wrg)
    os.system("C:/Users/Administrator/Desktop/wrg_vtk/WRG2VTK.exe " + folder_local + '/' + file_wrg)

    name_wrg, ext = os.path.splitext(file_wrg)
    res = os.path.exists('C:/Users/Administrator/Desktop/wrg_vtk/' + name_wrg + '1.vtk')
    if res == 0:
        print("wrg to vtk failed!")
        return str(2)
    print("wrg to vtk success!")

    # 上传vtk到s3
    bucket = 'goldfarm2'
    key = path_vtk
    path_list = []
    for i in range(5):
        file_vtk = name_wrg + str(i + 1) + '.vtk'
        path_list.append(file_vtk)
    print('upload vtk to s3')
    res = aws.s3_upload(bucket, key, folder_local, path_list)
    if res == 0:
        return str(3)

    return str(0)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8002)
