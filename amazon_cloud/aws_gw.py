# -*- coding:utf-8 -*-
import boto3
import os
import json


class aws_gw():

    def __init__(self, AWS_ACCESS, AWS_SECRET):
        self.s3c = boto3.client('s3', aws_access_key_id=AWS_ACCESS, aws_secret_access_key=AWS_SECRET,
                           region_name='cn-north-1')
        self.s3r = boto3.resource('s3', aws_access_key_id=AWS_ACCESS, aws_secret_access_key=AWS_SECRET,
                             region_name='cn-north-1')
        self.sqs = boto3.resource('sqs', aws_access_key_id=AWS_ACCESS, aws_secret_access_key=AWS_SECRET,
                             region_name='cn-north-1')
        self.ec2 = boto3.client('ec2', aws_access_key_id=AWS_ACCESS, aws_secret_access_key=AWS_SECRET,
                           region_name='cn-north-1')

    # 监听sqs
    def sqs_listen(self, queue_name):
        try:
            print("sqs listening......")
            sqs = self.sqs.get_queue_by_name(QueueName=queue_name)
            data = []
            while not data:
                response = sqs.receive_messages(WaitTimeSeconds=2)
                for msg in response:
                    data = eval(msg.body)
                    msg.delete()
            print("sqs listen end")
            return data
        except:
            print('sqs listen failed!')
            return None

    # 发送sqs
    def sqs_send(self, queue_name, data):
        try:
            print("sqs sending......")
            sqs = self.sqs.get_queue_by_name(QueueName=queue_name)
            sqs.send_message(MessageBody=json.dumps(data))
            print("sqs send end")
            return True
        except:
            print('sqs send failed!')
            return False

    # 上传文件到s3
    def s3_upload(self, bucket, key, folder_local, path_list):
        try:
            print("uploading......")
            path_len = len(path_list)
            for i in range(path_len):
                print('(' + str(i + 1) + '/' + str(path_len) + ')' + '\t' + path_list[i])
                path_local = folder_local + '/' + path_list[i]
                res = os.path.exists(path_local)
                if not res:
                    print('source files do not exist!')
                    print('upload files failed!')
                    return False
                self.s3c.put_object(Bucket=bucket, Key=key + '/' + path_list[i],
                               Body=open(path_local, 'rb').read())
                print("upload file to s3 success!")
                return True
        except:
            print('upload file to s3 failed!')
            return False

    # 从s3下载文件
    def s3_download(self, bucket, key, folder_local, path_list):
        try:
            print("downloading......")
            res = os.path.exists(folder_local)
            if not res:
                os.mkdir(folder_local)
            path_len = len(path_list)
            for i in range(path_len):
                print('(' + str(i + 1) + '/' + str(path_len) + ')' + '\t' + path_list[i])
                resp_pos = self.s3c.get_object(Bucket=bucket, Key=key + '/' + path_list[i])
                path_local = folder_local + '/' + path_list[i]
                with open(path_local, 'wb') as f:
                    f.write(resp_pos['Body'].read())
            print("download file from s3 success")
            return True
        except:
            print("download file from s3 failed!")
            return False


if __name__ == '__main__':
    AWS_ACCESS = "AKIAOVRACM2EDDYBIFUQ"
    AWS_SECRET = "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"
    aws = aws_gw(AWS_ACCESS, AWS_SECRET)
    queue_name = 'goldfarm-uav-start'
    data = {"project_id": 1, "status": 1, 'msg': "msg"}
    aws.sqs_send(queue_name, data)
    # data1 = aws.sqs_listen(queue_name)
    # print(str(data1))