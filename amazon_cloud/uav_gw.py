# -*- coding: utf-8 -*-
import os
import shutil
from aws_confg import *
from aws_gw import aws_gw
from smb_gw import smb_gw
from file_gw import file_gw


# 将pos_pix.txt分解为pos.txt和photo.txt
def file_split(path_list):
    try:
        path_pos = 'C:/Users/Administrator/Desktop/uav_mapper/pos.txt'
        path_photo = 'C:/Users/Administrator/Desktop/uav_mapper/photo.txt'
        path_pospix = 'C:/Users/Administrator/Desktop/uav_mapper/pos_pix.txt'
        folder_photos = 'C:/Users/Administrator/Desktop/uav_mapper/photo'
        file_pos = open(path_pos, 'w')
        file_photo = open(path_photo, 'w')
        with open(path_pospix) as f:
            line = f.readline()
            while line:
                str_list = line.split('\n')
                str_list = str_list[0].split('\t')
                if len(str_list) >= 4:
                    path_list.append(str_list[0])
                    file_pos.write(str_list[2] + ' ' + str_list[1] + ' ' + str_list[3] + '\n')
                    file_photo.write(folder_photos + '/' + str_list[0] + '\n')
                line = f.readline()
        file_pos.close()
        file_photo.close()
        return True
    except:
        return False


# 影像拼接
def image_fusion():
    try:
        print('image fusion......')
        os.system('C:/Users/Administrator/Desktop/uav_mapper/CDEMO/CDemo.exe'
                  ' -f C:/Users/Administrator/Desktop/uav_mapper/'
                  ' -p demo'
                  ' -i C:/Users/Administrator/Desktop/uav_mapper/photo.txt'
                  ' -g C:/Users/Administrator/Desktop/uav_mapper/pos.txt'
                  ' -s 4326'
                  ' -d 4326'
                  ' -mode 0')
        res = os.path.exists('C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products/demo_DSM_Fast.tif')
        if not res:
            print('image fusion failed!')
            return False
        print('image fusion success!')
        return True
    except:
        print('image fusion failed!')
        return False


# dsm重采样
def dsm_resample():
    try:
        print('image resample......')
        os.system('C:/Users/Administrator/Desktop/dsm_resample/GDAL_TIF.exe'
                  ' -s C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products/demo_DSM_Fast.tif'
                  ' -d C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products/demo_DSM_resample.tif'
                  ' -dr 2')
        res = os.path.exists('C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products/demo_DSM_resample.tif')
        if not res:
            print('dsm resample failed!')
            return False
        print('image resample success!')
        return True
    except:
        print('image resample failed!')
        return False


def uav_gw():
    try:
        bucket = 'goldfarm2'
        queue_name = 'goldfarm-uav-stop'
        
        # 连接AWS
        aws = aws_gw(AWS_ACCESS, AWS_SECRET)

        # 监听sqs
        data = aws.sqs_listen('goldfarm-uav-start')
        project_id = str(data['project_id'])
        sprint = 'project_id:' + project_id
        print(sprint)

        # # 从s3下载pos_pix.txt
        # print('download pos_pix.txt')
        # key = project_id + '/UAV/input'
        # folder_local = 'C:/Users/Administrator/Desktop/uav_mapper'
        # path_list = []
        # path_list.append('pos_pix.txt')
        # res = aws.s3_download(bucket, key, folder_local, path_list)
        # # 将pos_pix.txt分解为pos.txt和photo.txt
        # photos_path_list = []
        # file_split(photos_path_list)
        # if not res:
        #     data = {'project_id': project_id, 'status': 1, 'msg': 'download pos_pix.txt failed!'}
        #     aws.sqs_send(queue_name, data)
        #     return False
        #
        # # 从s3下载photos
        # print('download photos')
        # folder_photos = 'C:/Users/Administrator/Desktop/uav_mapper/photo'
        # key = project_id + '/UAV/input/photo'
        # res = aws.s3_download(bucket, key, folder_photos, photos_path_list)
        # if not res:
        #     data = {'project_id': project_id, 'status': 2, 'msg': 'download photos failed!'}
        #     aws.sqs_send(queue_name, data)
        #     return False
        #
        # # 影像拼接
        # res = image_fusion()
        # if not res:
        #     data = {'project_id': project_id, 'status': 3, 'msg': 'image fusion failed!'}
        #     aws.sqs_send(queue_name, data)
        #     return False

        # 上传dsm和dom到地图服务共享文件夹
        print('upload dsm and dom to map server')
        path_list = []
        path_list.append('demo_DSM_Fast.tif')
        path_list.append('demo_DSM_Fast.tif.ovr')
        path_list.append('demo_OrthoMosaic_Fast.tif')
        path_list.append('demo_OrthoMosaic_Fast.tif.ovr')
        folder_shared = 'gisdata/published/goldfarm/' + project_id
        folder_local = 'C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products'
        smb = smb_gw('datauser', 'gwdatauser', '10.32.23.252', 139)
        smb.createDir(folder_shared)
        res = smb.upload(folder_shared, folder_local, path_list)
        if not res:
            data = {'project_id': project_id, 'status': 5, 'msg': 'upload dsm and dom to map server failed!'}
            aws.sqs_send(queue_name, data)
            return False

        # 给共享文件夹发送sqs
        data = {'project_id': project_id, 'status': 0, 'msg': 'upload dsm and dom to map server success!'}
        aws.sqs_send('goldfarm-uav-MapService-upload', data)

        # 上传demo_DSM_Fast.tif到道路设计共享文件夹
        print('upload dsm to road-design server')
        path_list = []
        path_list.append('demo_DSM_Fast.tif')
        folder_local = 'C:/Users/Administrator/Desktop/uav_mapper/demo/5_Products'
        folder_dst = '//10.32.23.105/folder_shared/' + project_id
        res = os.path.isfile(folder_dst)
        if res:
            os.remove(folder_dst)
        res = os.path.isdir(folder_dst)
        if not res:
            os.mkdir(folder_dst)
        res = file_gw.copy(folder_local, folder_dst, path_list)
        if not res:
            data = {'project_id': project_id, 'status': 6, 'msg': 'upload dsm to road-design server failed!'}
            aws.sqs_send(queue_name, data)
            return False

        # 返回执行成功状态
        data = {'project_id': project_id, 'status': 0, 'msg': 'uav_gw seccess!'}
        aws.sqs_send(queue_name, data)
        print('uav_gw seccess!')
        return True
    except:
        print('uav_gw failed!')
        return False


if __name__ == '__main__':
    uav_gw()
