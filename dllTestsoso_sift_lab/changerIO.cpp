
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


#include"changerIO.hpp"

CHANGE_RGN* SceneChangeDetector(char* inputPath1,char* inputPath2,char* outputPath,DETECT_PARAM inputParam1,PRESET_POS_INFO inputParam2,bool flag)
{
       
	char fileName1[256];
	char fileName2[256];
	char fileName3[256];

        strcpy(fileName1,inputPath1);
	strcpy(fileName2,inputPath2);
	strcpy(fileName3,outputPath);

    	double scale=1; //设置缩放倍数
	DETECT_PARAM params ;//= {3,7,8,400,0.01,200,1.6,3,0.3};
	params.nScale = inputParam1.nScale;//检测尺度
	params.nSzKernel = inputParam1.nSzKernel;//滤波核大小
	params.nFilters = inputParam1.nFilters;//滤波器个数
	params.nChangeRnSz = (inputParam1.nChangeRnSz)*scale;//变化区域最小尺寸
	params.fSensitivity = inputParam1.fSensitivity;//检测灵敏度，分割阈值
	params.nChangeRnSzKH = (inputParam1.nChangeRnSzKH)*scale;//KH变化区域最小尺寸
	params.fSensitivityKH = inputParam1.fSensitivityKH;//检测灵敏度，分割阈值
	params.whratio1 = inputParam1.whratio1;
        params.whratio2 = inputParam1.whratio2;  //矩形长宽比

	PRESET_POS_INFO presetPosInfo = {0,0,0,0};
	//memcpy(&presetPosInfo,(PRESET_POS_INFO&)inputParam2,sizeof(params));

        presetPosInfo.fCarrierAz = inputParam2.fCarrierAz;// 预置位方位角
        presetPosInfo.fCarrierEl = inputParam2.fCarrierEl;//预置位俯仰角
        presetPosInfo.fCarrierHt = inputParam2.fCarrierHt;//云台高度
        presetPosInfo.fFieldViewPP = inputParam2.fFieldViewPP;//预置位视场角

        int i;
        Mat image_A,image_B; 
        Mat image_A_Color,image_B_Color;

        CHANGE_RGN rtRgn;// = NULL;
        int rtRgnNum = 0;
  	int rtRgnNum1 = 0;
        Rect rcTemp;
	Mat srcImgA; 
	Mat srcImgB;
	vector<Rect> output;//save the changed region
        vector<float> outputCorrelationCf;//save the corresponding correlation coefficient
        image_A = imread(fileName1, 1); // Read the file  
        image_B = imread(fileName2,1);


        if(! image_A.data ) // Check for invalid input  
        {  
            cout << "Could not open or find the image" << std::endl ;  
            return NULL;  
        } 
        if(! image_B.data ) // Check for invalid input  
        {   
            cout << "Could not open or find the image" << std::endl ;  
            return NULL;  
        } 
	if(image_B.size!=image_A.size)
		 return NULL;

	
	Size dsize_A = Size(image_A.cols*scale,image_A.rows*scale);
	Mat image_At = Mat(dsize_A,CV_32S);
	resize(image_A, image_At,dsize_A);

	Size dsize_B = Size(image_B.cols*scale,image_B.rows*scale);
	Mat image_Bt = Mat(dsize_B,CV_32S);
	resize(image_B, image_Bt,dsize_B);	

        int maxSize = 0;
	int maxSize1 = 0;
	int maxSize2 = 0;
	int maxSize3 = 0;
	int maxID = -1;
	int maxID1 = -1;
	int maxID2 = -1;
	int maxID3 = -1;
	int leftLmt = 10*scale;
	int rightLmt = (image_B.cols-10)*scale;
	int topLmt = 150*scale;//字符裁剪位置，垂直方向
	int bottomLmt = (image_B.rows-10)*scale;
	int rightX1,bottomY1;
	int rightX2,bottomY2;
	int rightX3,bottomY3;
	int sz,sz1,sz2,sz3;
	CvPoint origin;//配准后交点坐标
	origin.x = 0;
	origin.y = 0;
 	int Hflag=1;

       if (flag==true)
	{ 
		//convert Mat to IplImage
		Mat mat1,mat2;
		IplImage *image_AA;
		IplImage *image_BB;
		mat1=image_At.clone();
		mat2=image_Bt.clone();
	 	image_AA=cvCreateImage(cvSize(image_At.cols,image_At.rows),8,3);
	 	image_BB=cvCreateImage(cvSize(image_Bt.cols,image_Bt.rows),8,3);
		image_AA->imageData=(char*)mat1.data;
		image_BB->imageData=(char*)mat2.data;	

		// 参数定义	
		int param_WH[4] = {0,0,0,0};//processWidth=param_WH[0]; processHeight=param_WH[1]; start=param_WH[2]; start2=param_WH[3];

		CvPoint param_pt[4]={{0,0},{0,0},{0,0},{0,0}};//leftTop=param_pt[0];leftBottom=param_pt[1];rightTop=param_pt[2];rightBottom=param_pt[3];
		CvMat * H = NULL;//RANSAC算法求出的变换矩阵
 
		// 计算校正参数
		jisuanwh(image_AA,image_BB,param_WH,param_pt,H,&Hflag);	

		if(Hflag==1)
        	{
			//初始化校正图像
		 	IplImage* corr_img1 = NULL;
			corr_img1 = cvCreateImage(cvSize(param_WH[0] ,param_WH[1]),IPL_DEPTH_8U,3);
		 	IplImage* corr_img2 = NULL;
			corr_img2 = cvCreateImage(cvSize(param_WH[0] ,param_WH[1]),IPL_DEPTH_8U,3);

			//校正图像
			correct(image_AA,image_BB,corr_img1,corr_img2,param_WH, param_pt,origin,H);

			Mat image_Ar(corr_img1,true);
			Mat image_Br(corr_img2,true);
		 
			cvtColor(image_Ar, srcImgA, CV_RGB2GRAY);
			cvtColor(image_Br, srcImgB, CV_RGB2GRAY);
			image_A_Color = image_Ar;
			image_B_Color = image_Br;


			if (NULL != corr_img1)
			{
			cvReleaseImage(&corr_img1);
			}
			if (NULL != corr_img2)
			{
			cvReleaseImage(&corr_img2);
			}
			if(H)
			{
			 cvReleaseMat(&H);//释放变换矩阵H  
			}
		}	       
	}

	if((flag==false)||(Hflag==0))
	{
		cvtColor(image_At, srcImgA, CV_RGB2GRAY);
		cvtColor(image_Bt, srcImgB, CV_RGB2GRAY);
		image_A_Color = image_At;
		image_B_Color = image_Bt;
	}
	// DETECT_PARAM params1={3,7,8,200,1.6};

//Mat colorCorrectImgA,colorCorrectImgB;
//colorCorrect(image_A_Color,image_B_Color,colorCorrectImgA,colorCorrectImgB);
//return NULL;

   vector<Rect> output1;
   vector<Rect> output2;
   vector<Rect> output3;
   vector<Rect> outputtemp;
   vector<float> outputCf1,outputCf2,outputCf3;

// ChangeDetectorKH(image_A_Color,image_B_Color,params,output1,outputCf);//detect the changes from KH space
// ChangeDetector(srcImgA,srcImgB, params,output,outputCorrelationCf,"null");//detect the changes
 
   vector<Mat> rImgA(image_A_Color.channels());
   vector<Mat> rImgB(image_B_Color.channels());
   Mat grayImgA,grayImgB,labImgA,labImgB;

// cvtColor(srcImgA, grayImgA, CV_BGR2GRAY);
// cvtColor(srcImgB, grayImgB, CV_BGR2GRAY);
 
   cvtColor(image_A_Color, labImgA, CV_BGR2Lab);      
   cvtColor(image_B_Color, labImgB, CV_BGR2Lab);

   split(labImgA,rImgA);
   split(labImgB,rImgB);
//  ChangeDetector(grayImgA,grayImgB,params,output,outputCorrelationCf,"/home/guangdian/zhangtao/ftp/DebugResult/cf_bw_gray.jpg");
//cout<<"save cf_bw_gray ok"<<endl;
   ChangeDetector(rImgA[0],rImgB[0],params,output1,outputCorrelationCf,"/home/tuxiang/liquan/Pictures/DebugResult/cf_bw_L.jpg");
//cout<<"save cf_bw_L ok"<<endl;
   ChangeDetector(rImgA[1],rImgB[1],params,output2,outputCorrelationCf,"/home/tuxiang/liquan/Pictures/DebugResult/cf_bw_a.jpg");
//cout<<"save cf_bw_a ok"<<endl;
   ChangeDetector(rImgA[2],rImgB[2],params,output3,outputCorrelationCf,"/home/tuxiang/liquan/Pictures/DebugResult/cf_bw_b.jpg");
  
   
 	 for(i = 0;i<output1.size();i++)
	 {		
		rightX1 = output1.at(i).x+output1.at(i).width;
		bottomY1= output1.at(i).y+output1.at(i).height;
		double wh1=double(output1.at(i).width)/double(output1.at(i).height);	
		if(rightX1<leftLmt||rightX1>rightLmt||bottomY1<topLmt||bottomY1>bottomLmt||wh1>params.whratio1||wh1<params.whratio2)
		{	  
			continue;
		}		
		sz1 = output1.at(i).width*output1.at(i).height;	
                if(maxSize1<sz1)
                {
                   maxSize1 = sz1;
                   maxID1 = i;
               }  
	      output.push_back(output1.at(i));
         }
 	 for(i = 0;i<output2.size();i++)
	 {		
		rightX2 = output2.at(i).x+output2.at(i).width;
		bottomY2= output2.at(i).y+output2.at(i).height;
		double wh1=double(output2.at(i).width)/double(output2.at(i).height);	
		if(rightX2<leftLmt||rightX2>rightLmt||bottomY2<topLmt||bottomY2>bottomLmt||wh1>params.whratio1||wh1<params.whratio2)
		{	  
			continue;
		}		
		sz1 = output2.at(i).width*output2.at(i).height;	
                if(maxSize2<sz1)
                {
                   maxSize2 = sz1;
                   maxID2 = i;
               }  
	      output.push_back(output2.at(i));
         } 	 
         for(i = 0;i<output3.size();i++)
	 {		
		rightX1 = output3.at(i).x+output3.at(i).width;
		bottomY1= output3.at(i).y+output3.at(i).height;
		double wh1=double(output3.at(i).width)/double(output3.at(i).height);	
		if(rightX1<leftLmt||rightX1>rightLmt||bottomY1<topLmt||bottomY1>bottomLmt||wh1>params.whratio1||wh1<params.whratio2)
		{	  
			continue;
		}		
		sz1 = output3.at(i).width*output3.at(i).height;	
                if(maxSize1<sz1)
                {
                   maxSize3 = sz1;
                   maxID3 = i;
               }  
	      output.push_back(output3.at(i));
         }


	//  output.insert(output.end(), output1.begin(),output1.end);
  	//  output.insert(output.end(), output2.begin(),output2.end);
	//  output.insert(output.end(), output3.begin(),output3.end);
       
	 for(i = 0;i<output.size();i++)
	 {		
		rightX3 = output.at(i).x+output.at(i).width;
		bottomY3 = output.at(i).y+output.at(i).height;

		double wh2=double(output.at(i).width)/double(output.at(i).height);
		if(rightX3<leftLmt||rightX3>rightLmt||bottomY3<topLmt||bottomY3>bottomLmt||wh2>params.whratio1||wh2<params.whratio2)
		{	  
			continue;
		}		
		sz3 = output.at(i).width*output.at(i).height;
cout<<"Rect size"<<sz3<<endl;
     	        if(maxSize3<sz3)
                {
                   maxSize = sz3;
                   maxID = i;
                } 
                /* 
             //all changes start
		rcTemp = output.at(i);
		rcTemp.x = rcTemp.x-origin.x;
		rcTemp.y = rcTemp.y-origin.y;
		rcTemp.x = rcTemp.x<0?0:rcTemp.x;
		rcTemp.y = rcTemp.y<0?0:rcTemp.y;
                rectangle(image_Bt,rcTemp,Scalar(255,0,0));//label

		rcTemp.x = (rcTemp.x-1)<0?0:(rcTemp.x-1);
		rcTemp.y = (rcTemp.y-1)<0?0:(rcTemp.y-1);
		rcTemp.width = rcTemp.width+2;
		rcTemp.height = rcTemp.height+2;
		rectangle(image_Bt,rcTemp,Scalar(255,255,255));//label
             //   rectangle(image_Bt,output.at(j),Scalar(0,0,255));//label  
		// all changes finsh
		*/
         }
        

	if(maxID!=-1&&maxSize!=0)
	{
		rcTemp = output.at(maxID);
		rcTemp.x = rcTemp.x-origin.x;
		rcTemp.y = rcTemp.y-origin.y;
		rcTemp.x = rcTemp.x<0?0:rcTemp.x;
		rcTemp.y = rcTemp.y<0?0:rcTemp.y;
                rectangle(image_Bt,rcTemp,Scalar(0,0,255));//label

		rcTemp.x = (rcTemp.x-1)<0?0:(rcTemp.x-1);
		rcTemp.y = (rcTemp.y-1)<0?0:(rcTemp.y-1);
		rcTemp.width = rcTemp.width+2;
		rcTemp.height = rcTemp.height+2;
		rectangle(image_Bt,rcTemp,Scalar(0,0,0));//label

                 rtRgn.fAz = presetPosInfo.fCarrierAz+arc2deg*
2*atan(H_RESOLUTION_mm/(2*f_MIN_mm*presetPosInfo.fFieldViewPP))*(rcTemp.x+rcTemp.width/2-image_At.cols/2)/H_RESOLUTION_PIXEL;
		rtRgn.fEl = presetPosInfo.fCarrierEl+arc2deg*
2*atan(V_RESOLUTION_mm/(2*f_MIN_mm*presetPosInfo.fFieldViewPP))*(rcTemp.y+rcTemp.height/2-image_At.rows/2)/V_RESOLUTION_PIXEL;
		rtRgn.fDist = presetPosInfo.fCarrierHt/(0.00001+sin(deg2arc*rtRgn.fEl)); 
		rtRgn.fP = 1;
		if(rtRgn.fAz<0)
			rtRgn.fAz=rtRgn.fAz+360;
		if(rtRgn.fAz>360)
			rtRgn.fAz=rtRgn.fAz-360;
               
	}
	else
	{
		cout<<"Two images have not changes"<<endl;
		rtRgn.fP = 0;	
	}


	Size dsize_Bt = Size(image_B.cols,image_B.rows);
	Mat image_Btr = Mat(dsize_Bt,CV_32S);
	resize(image_Bt, image_Btr,dsize_Bt);


        imwrite(fileName3,image_Btr);//save the labeled image
        return &rtRgn;

}
//参数计算
void jisuanwh(IplImage* img1,IplImage* img2, int *param_wh,CvPoint* param_pt,CvMat * &H ,int *Hflag)
{    
    int n1,n2;//n1:图1中的特征点个数，n2：图2中的特征点个数
    struct feature* feat1, * feat2;//feat1：图1的特征点数组，feat2：图2的特征点数组
    struct feature *feat;//每个特征点
    struct feature* featForH = NULL;
    featForH = (feature*)calloc( MAX_MATCHPOINTS, sizeof(struct feature) );//分配空间 

    struct feature** nbrs;//当前特征点的最近邻点数组
    struct kd_node* kd_root;//k-d树的树根
    struct feature **inliers;//精RANSAC筛选后的内点数组
    int n_inliers;//经RANSAC算法筛选后的内点个数,即feat2中具有符合要求的特征点的个数
    IplImage *xformed = NULL;//xformed临时拼接图，即只将图2变换后的图

    n1 = detectionFeature( img1,feat1 );//检测图1中的SIFT特征点,n1是图1的特征点个数
    n2 = detectionFeature( img2, feat2 );//检测图2中的SIFT特征点，n2是图2的特征点个数
   
    kd_root = kdtree_build( feat1, n1 );//根据图1的特征点集feat1建立k-d树，返回k-d树根给kd_root

    double d0,d1;//feat2中每个特征点到最近邻和次近邻的距离
    int matchNum = 0;//经距离比值法筛选后的匹配点对的个数

    bool* matchPointSlectFlag = (bool*)calloc( n2, sizeof(bool) );//分配空间

    //遍历特征点集feat2，针对feat2中每个特征点feat，选取符合距离比值条件的匹配点，放到feat的fwd_match域中
   for(int i = 0; i < n2; i++ )
    {
        feat = feat2+i;//第i个特征点的指针
        //在kd_root中搜索目标点feat的2个最近邻点，存放在nbrs中，返回实际找到的近邻点个数
        int k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
        if( k == 2 )
        {
            d0 = descr_dist_sq( feat, nbrs[0] );//feat与最近邻点的距离的平方
            d1 = descr_dist_sq( feat, nbrs[1] );//feat与次近邻点的距离的平方
            //若d0和d1的比值小于阈值NN_SQ_DIST_RATIO_THR，则接受此匹配，否则剔除
            if( d0 < d1 * NN_SQ_DIST_RATIO_THR )  
            {   
                matchNum++;//统计匹配点对的个数
                feat2[i].fwd_match = nbrs[0];//使点feat的fwd_match域指向其对应的匹配点
	        matchPointSlectFlag[i] = 1;
            }
            else
               matchPointSlectFlag[i] = 0;    //feat2[i].fwd_match = NULL;

        }
	else
              matchPointSlectFlag[i] = 0;    //feat2[i].fwd_match = NULL;
              free( nbrs );//释放近邻数组
   }
 
 	int matchPointSelctStep = 0;
 	int rtMatchPoints = 0;
	matchPointSelctStep=matchNum/MAX_MATCHPOINTS;
	
	int j=0;
	if(matchPointSelctStep>=1)
	{
	  for(int i=0;i<n2;i++)
	  {
		if((matchPointSlectFlag[i] == 1)&&(j<matchNum))
		{
			j++;
			if((j%matchPointSelctStep)==0&&(rtMatchPoints<MAX_MATCHPOINTS))
			{
			  featForH[rtMatchPoints] = feat2[i];
			  rtMatchPoints++;
			}
		}
	  }
	}
	
/*
	int j=0;
	if(matchPointSelctStep>=1)
	{
	  for(int i=0;i<n2;i++)
	  {

		if((feat2[i].fwd_match != NULL)&&(j<matchNum)&&((j%matchPointSelctStep)==0)&&(rtMatchPoints<MAX_MATCHPOINTS))
		{		
			  j++;
			  featForH[rtMatchPoints] = feat2[i];
			  rtMatchPoints++;		
		}
	  }
	}
*/
	else
	{
		rtMatchPoints = MAX_MATCHPOINTS>matchNum?matchNum:MAX_MATCHPOINTS;
		featForH = feat2;	
	}
	free(matchPointSlectFlag);	

     //无论img1和img2的左右顺序，H永远是将feat2中的特征点变换为其匹配点，即将img2中的点变换为img1中的对应点
	H = ransac_xform(featForH,rtMatchPoints,FEATURE_FWD_MATCH,lsq_homog,4,0.01,homog_xfer_err,3.0,&inliers,&n_inliers,Hflag);

   	if(*Hflag==0)
    		{return;}
	free(featForH);
    if( H )   //若能成功计算出变换矩阵，即两幅图中有共同区域 
    {    
        CalcFourCorner(H,param_pt[0],param_pt[1],param_pt[2],param_pt[3],img2); //计算图2的四个角经变换后的坐标
       //为拼接结果图xformed分配空间,高度为图1图2高度的较小者，根据图2右上角和右下角变换后的点的位置决定拼接图的宽度
	 xformed = cvCreateImage(cvSize(abs(MIN(param_pt[0].x,param_pt[1].x))+img2->width,MIN(img1->height,img2->height)),IPL_DEPTH_8U,3);
        //用变换矩阵H对右图img2做投影变换(变换后会有坐标右移)，结果放到xformed中
        cvWarpPerspective(img2,xformed,H,CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS,cvScalarAll(0));

        //创建重叠区域大小的图像
	 param_wh[2] = MIN(param_pt[0].x,param_pt[1].x) ;//开始位置，即重叠区域的左边界 start
         param_wh[0] = img1->width-abs(param_wh[2]);//重叠区域的宽度  processWidth

         param_wh[3] = MIN(param_pt[2].y,param_pt[3].y);  //重叠区域的上边界  start2
	 param_wh[1] = xformed->height-abs(param_wh[3]);   //重叠区域的高度   processHeight 
	
   }
	else //无法计算出变换矩阵，即两幅图中没有重合区域
    {
        return ;
    }

    kdtree_release(kd_root);//释放kd树
    if(H)
    {
        free(inliers);//释放内点数组
     }
   if (NULL != xformed)
   {
       cvReleaseImage(&xformed);
   }
}

//计算图2的四个角经矩阵H变换后的坐标
void CalcFourCorner(CvMat* &H,CvPoint& leftTop,CvPoint& leftBottom, CvPoint& rightTop,CvPoint& rightBottom,IplImage* img2)
{
    //计算图2的四个角经矩阵H变换后的坐标
    double v2[]={0,0,1};//左上角
    double v1[3];//变换后的坐标值
    CvMat V2 = cvMat(3,1,CV_64FC1,v2);
    CvMat V1 = cvMat(3,1,CV_64FC1,v1);
    cvGEMM(H,&V2,1,0,1,&V1);//矩阵乘法
    leftTop.x = cvRound(v1[0]/v1[2]);
    leftTop.y = cvRound(v1[1]/v1[2]);

    //将v2中数据设为左下角坐标
    v2[0] = 0;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
    cvGEMM(H,&V2,1,0,1,&V1);
    leftBottom.x = cvRound(v1[0]/v1[2]);
    leftBottom.y = cvRound(v1[1]/v1[2]);

    //将v2中数据设为右上角坐标
    v2[0] = img2->width;
    v2[1] = 0;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
    cvGEMM(H,&V2,1,0,1,&V1);
    rightTop.x = cvRound(v1[0]/v1[2]);
    rightTop.y = cvRound(v1[1]/v1[2]);

    //将v2中数据设为右下角坐标
    v2[0] = img2->width;
    v2[1] = img2->height;
    V2 = cvMat(3,1,CV_64FC1,v2);
    V1 = cvMat(3,1,CV_64FC1,v1);
    cvGEMM(H,&V2,1,0,1,&V1);
    rightBottom.x = cvRound(v1[0]/v1[2]);
    rightBottom.y = cvRound(v1[1]/v1[2]);
}


int detectionFeature(IplImage* img,struct feature*& feat)
{
    int n  = sift_features( img, &feat);//检测图img中的SIFT特征点,n是图的特征点个数
    return n;
}

//校正实现
IplImage* correct(IplImage *img1,IplImage *img2,IplImage *corr_img1,IplImage *corr_img2,int *param_WH,CvPoint *param_pt,CvPoint& origin,CvMat * H)
{
	IplImage *xformed=NULL;
       //为拼接结果图xformed分配空间,高度为图1图2高度的较小者，根据图2右上角和右下角变换后的点的位置决定拼接图的宽度
        xformed = cvCreateImage(cvSize(abs(MIN(param_pt[0].x,param_pt[1].x))+img2->width,MIN(img1->height,img2->height)),IPL_DEPTH_8U,3);
        //用变换矩阵H对右图img2做投影变换(变换后会有坐标右移)，结果放到xformed中
        cvWarpPerspective(img2,xformed,H,CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS,cvScalarAll(0));

	if(param_WH[2]<=0&&param_WH[3]>0)
	{
	      cvSetImageROI(xformed,cvRect(0,param_WH[3],param_WH[0] ,param_WH[1] )); //设置ROI，是包含重叠区域的矩形
	      cvCopy(xformed,corr_img2,0);//复制图像
	      cvResetImageROI(corr_img2);

	      cvSetImageROI(img1,cvRect(0,param_WH[3],param_WH[0] ,param_WH[1] ));
	      cvCopy(img1,corr_img1,0);
	      cvResetImageROI(corr_img1);
	      origin.x=0;
	      origin.y=param_WH[3];
       }

      if(param_WH[2]<=0&&param_WH[3]<=0)
     {
	      cvSetImageROI(xformed,cvRect(0,0,param_WH[0] ,param_WH[1] )); //设置ROI，是包含重叠区域的矩形
	      cvCopy(xformed,corr_img2,0);//复制图像
	      cvResetImageROI(corr_img2);

	      cvSetImageROI(img1,cvRect(0,0,param_WH[0] ,param_WH[1] ));
	      cvCopy(img1,corr_img1,0);
	      cvResetImageROI(corr_img1);
	      origin.x=0;
	      origin.y=0;
     }

     if(param_WH[2]>0&&param_WH[3]<=0)
     {
	      cvSetImageROI(xformed,cvRect(param_WH[2],0,param_WH[0] ,param_WH[1] )); //设置ROI，是包含重叠区域的矩形
	      cvCopy(xformed,corr_img2,0);//复制图像
	      cvResetImageROI(corr_img2);    

	      cvSetImageROI(img1,cvRect(param_WH[2],0,param_WH[0] ,param_WH[1]));
	      cvCopy(img1,corr_img1,0);
	      cvResetImageROI(corr_img1);
	      origin.x = param_WH[2];
	      origin.y = 0;
      }

    if(param_WH[2]>0&&param_WH[3]>0)
    {
	      cvSetImageROI(xformed,cvRect(param_WH[2],param_WH[3],param_WH[0] ,param_WH[1] )); //设置ROI，是包含重叠区域的矩形
	      cvCopy(xformed,corr_img2,0);//复制图像
	      cvResetImageROI(corr_img2);

	      cvSetImageROI(img1,cvRect(param_WH[2],param_WH[3],param_WH[0] ,param_WH[1]));
	      cvCopy(img1,corr_img1,0);
	      cvResetImageROI(corr_img1);
	      origin.x=param_WH[2];
	      origin.y=param_WH[3];
    }

    if(NULL != img1)
    {
        cvReleaseImage(&img1);
    }

    if(NULL != img2)
    {
        cvReleaseImage(&img2);
    }
    if (NULL != xformed)
    {
        cvReleaseImage(&xformed);
     }

    return 0;
}
