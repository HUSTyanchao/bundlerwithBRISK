#ifndef PATTERN_H
#define PATTERN_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2\opencv.hpp>
//#include "opencv2/nonfree/nonfree.hpp"  //opencv2 sift
#include <opencv2\xfeatures2d.hpp> //opencv301

#include "globalSetting.h"

//Ϊ��ǰ��Pattern�洢
struct MatchesInvertedIndex{
	uint matchedImgNumber;
	uint matchedImgMatchesNumber;
};

//Ϊ��ǰ��Pattern�洢ƥ��ɹ���ͼ����ź�matches
struct MatchesImage{
	//ƥ��ɹ���ͼƬ�����
	uint									matchesNumber;
	std::vector<cv::DMatch>					matches;
};

//�洢ÿ����ͼ������
struct Pattern {
	cv::Mat									img;
	
	std::vector<MatchesImage>				matchedImg;
	std::vector<MatchesInvertedIndex>		matchedImgInvertedIndex;

	std::vector<cv::KeyPoint>				keypoints;
	cv::Mat									descriptors;
	
	std::vector<cv::Point2f>				points2d;
	std::vector<cv::Point3f>				points3d;
	
	//���浱ǰͼƬ����̬����bundle.out�ļ��ж�ȡ��̬
	//cv::Matx34f			  cameraPose;
};

class PatternMatch
{
public:
	//���캯��
	PatternMatch();
	
	//��������
	~PatternMatch();

	//������ͼƬ����pattern
	void BuildPatternFromImage(const std::vector<std::string>& imgList);

	//��ָ����ͼƬ��������ͼƬ����ƥ��
	void MatchPatternTwoInAll(std::vector<uint> dstNumberVector);

	//��һ��ͼƬ����������ͼƬƥ��,�õ�ƥ���ƥ���б�
	//ƥ���б��б�����ƥ��ɹ���ͼƬ�����
	void MatchPatternOneToAll(const std::vector<uint>& srcNumberVector, const uint& dstNumber);

protected:

	//ƥ������ģʽ��ͼ��,���������Ż�ƥ�䣬ƥ��ɹ�����1�����򷵻�0
	bool MatchPatternOneToOne(const Pattern& src, const Pattern& dst, std::vector<cv::DMatch>& matches);

	//ͨ����Ӧ�Ż�ƥ�䣬�޳����
	static bool RefineMatchesWithHomography(
		const std::vector<cv::KeyPoint>& queryKeypoints,
		const std::vector<cv::KeyPoint>& trainKeypoints,
		const float homographyReproThres,
		std::vector<cv::DMatch>& matches,
		cv::Mat& homography);

	//ͨ����������Լ���Ż�ƥ�䣬�޳����
	static bool RefineMatchesWithFundamental(
		const std::vector<cv::KeyPoint>& queryKeypoints,
		const std::vector<cv::KeyPoint>& trainKeypoints,
		const float reprojectionThreshold,
		std::vector<cv::DMatch>& matches,
		cv::Mat& fundamental);
	//�ж�һ����srcNumber�Ƿ���Ŀ��������,�ڷ���1�� ���򷵻�0
	static bool findElement(uint& srcNumber, std::vector<uint>& dstNumberVector);

private:
	const float						homographyReproThres;
	std::vector<Pattern>			mPattern;
	cv::Ptr<cv::Feature2D>			mPtrFeature;
	cv::Ptr<cv::DescriptorMatcher>	mPtrMatcher;
	cv::Ptr<cv::Feature2D>			mPtrXfeature;

};


bool WriteKeyPointsWithDescriptors(std::string imgName,
	const std::vector<cv::KeyPoint>& keypoints,
	const cv::Mat& descriptor);

bool WriteMatches(const int imgIndex1,
	const int imgIndex2,
	const std::vector<cv::DMatch>& matches);

#endif