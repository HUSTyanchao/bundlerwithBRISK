#ifndef IMAGEPREPARE_H
#define IMAGEPREPARE_H

#include "iostream"
#include "fstream"
#include "string"
#include "io.h"  
#include <opencv2/opencv.hpp>
#include "globalSetting.h"


class Image{
public:

	//Ĭ���ǲ�ѯͼ��·��
	Image(
		std::string imgPath = gl_ImgPathQuery,
		std::string imgListPath = gl_ImgListPathQuery
		);

	~Image();

	//��ȡimage·���µ�ͼƬ���ƣ����洢��list_**.txt��
	//ÿһ����һ��ͼƬ��·��
	void MakeImgListTxt() const;

	//��ȡͼ���ļ��б�,������imageList��
	bool MakeImgListVector();

	//��ʾͼƬ�б��е�ͼƬ
	void ShowImage(uint numberMax = 5)const;
	
	//����ͼ���ļ�������
	std::vector<std::string> ReturnImgListVector()const;

private:
	//ͼƬ����·��
	std::string imgPath;
	
	//imgListPath - ͼ���б��ļ�list_**.txt����·��
	std::string imgListPath;
	
	//�洢ͼƬ���ļ�����������·����
	std::vector<std::string> imgListVector;
};


#endif